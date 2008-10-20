/** \file TimeCorrectorApp.cxx
    \brief Implementation of the TimeCorrectorApp class.
    \authors Masaharu Hirayama, GSSC,
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/TimeCorrectorApp.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <stdexcept>

#include "hoops/hoops.h"

#include "st_app/AppParGroup.h"
#include "st_app/StAppFactory.h"

#include "timeSystem/AbsoluteTime.h"
#include "timeSystem/EventTimeHandler.h"
#include "timeSystem/GlastTimeHandler.h"

#include "tip/FileSummary.h"
#include "tip/Header.h"
#include "tip/IFileSvc.h"
#include "tip/TipException.h"
#include "tip/TipFile.h"

static const std::string s_cvs_id = "$Name:  $";

namespace {

  using namespace timeSystem;

  class IHandlerPairFactory {
    public:
      virtual ~IHandlerPairFactory() {}

      std::pair<EventTimeHandler *, EventTimeHandler *> create(const std::string & first_file_name,
        const std::string & second_file_name, int extension_number) const;

      virtual EventTimeHandler * create(const std::string & file_name, int extension_number, bool as_first_file = true) const = 0;

    protected:
      IHandlerPairFactory() {}
  };

  template <typename FirstHandlerType, typename SecondHandlerType>
  class HandlerPairFactory : public IHandlerPairFactory {
    public:
      HandlerPairFactory(): IHandlerPairFactory() {}

      virtual ~HandlerPairFactory() {}

      virtual EventTimeHandler * create(const std::string & file_name, int extension_number, bool as_first_file = true) const;
  };

  std::pair<EventTimeHandler *, EventTimeHandler *> IHandlerPairFactory::create(const std::string & first_file_name,
    const std::string & second_file_name, int extension_number) const {
    // Prepare variables to hold the return value.
    EventTimeHandler * first_handler(0);
    EventTimeHandler * second_handler(0);

    // Try to create an event time handler for the first file.
    first_handler = create(first_file_name, extension_number, true);
    if (0 != first_handler) {
      // Try to create an event time handler for the second file.
      second_handler = create(second_file_name, extension_number, false);

      // Destroy the handler for the first file if no handler is created for the second.
      if (0 == second_handler) {
        delete first_handler;
        first_handler = 0;
      }
    }

    // Return the pair of handlers.
    return std::make_pair(first_handler, second_handler);
  }

  template <typename FirstHandlerType, typename SecondHandlerType>
  EventTimeHandler * HandlerPairFactory<FirstHandlerType, SecondHandlerType>::create(const std::string & file_name,
    int extension_number, bool as_first_file) const {
    // Prepare a variable to hold the return value.
    EventTimeHandler * handler(0);

    // Create an extension name string.
    std::ostringstream oss;
    oss << extension_number;
    std::string ext_name = oss.str();

    try {
      // Try to create an event time handler.
      if (as_first_file) handler = FirstHandlerType::createInstance(file_name, ext_name, true);
      else handler = SecondHandlerType::createInstance(file_name, ext_name, false);

    } catch (...) {
      // Return 0 (null pointer) for error(s) of any kind.
      handler = 0;
    }

    // Return the pointer.
    return handler;
  }

}

namespace timeSystem {

  TimeCorrectorApp::TimeCorrectorApp() {
    setName("gtbary");
    setVersion(s_cvs_id);
  }

  TimeCorrectorApp::~TimeCorrectorApp() throw() {}

  void TimeCorrectorApp::run() {
    st_app::AppParGroup & pars = getParGroup();
    pars.Prompt();
    pars.Save();

    // Prepare for event file reading/writing, based on given time correction mode.
    std::string t_correct = pars["tcorrect"];
    std::string t_correct_uc(t_correct);
    for (std::string::iterator itor = t_correct_uc.begin(); itor != t_correct_uc.end(); ++itor) *itor = std::toupper(*itor);
    typedef std::list<IHandlerPairFactory *> factory_cont_type;
    factory_cont_type factory_cont;
    std::string target_time_ref;
    std::string target_time_sys;
    if ("BARY" == t_correct_uc) {
      target_time_ref = "SOLARSYSTEM";
      target_time_sys = "TDB";
      factory_cont.push_back(new HandlerPairFactory<GlastScTimeHandler, GlastBaryTimeHandler>());
    } else if ("GEO" == t_correct_uc) {
      target_time_ref = "GEOCENTRIC";
      target_time_sys = "TT";
      factory_cont.push_back(new HandlerPairFactory<GlastScTimeHandler, GlastGeoTimeHandler>());
    } else {
      throw std::runtime_error("Unsupported arrival time correction: " + t_correct);
    }

    // Check existence of the input FITS file.
    std::string inFile_s = pars["evfile"];
    if (!tip::IFileSvc::instance().fileExists(inFile_s)) {
      throw std::runtime_error("File not found: " + inFile_s);
    }

    // Get file summary of the input FITS file.
    tip::FileSummary file_summary;
    tip::IFileSvc::instance().getFileSummary(inFile_s, file_summary);

    // Loop over all extensions of the input file, including primary HDU, to check whether input file is supported or not.
    int ext_number = 0;
    for (tip::FileSummary::const_iterator ext_itor = file_summary.begin(); ext_itor != file_summary.end(); ++ext_itor, ++ext_number) {
      bool supported = false;
      for (factory_cont_type::const_iterator fact_itor = factory_cont.begin(); fact_itor != factory_cont.end(); ++fact_itor) {
        std::auto_ptr<EventTimeHandler> input_handler((*fact_itor)->create(inFile_s, ext_number));
        if (0 != input_handler.get()) supported = true;
      }
      if (!supported) {
        std::ostringstream oss;
        oss << "Unsupported timing extension: HDU "  << ext_number;
        if (0 == ext_number) oss << " (primary HDU)";
        else oss << " (EXTNAME=" << ext_itor->getExtId() << ")";
        oss << " of input file \"" << inFile_s << "\"";
        throw std::runtime_error(oss.str());
      }
    }

    // Check whether output file name already exists or not, if clobber parameter is set to no.
    std::string outFile_s = pars["outfile"];
    bool clobber = pars["clobber"];
    if (!clobber) {
      bool file_readable = false;
      try {
        std::ifstream is(outFile_s.c_str());
        if (is.good()) file_readable = true;
      } catch (const std::exception &) {}
      if (file_readable) throw std::runtime_error("File " + outFile_s + " exists, but clobber not set");
    }

    // Confirm that outfile is writable.
    bool file_writable = false;
    try {
      std::ofstream os(outFile_s.c_str(), std::ios::out | std::ios::app);
      if (os.good()) file_writable = true;
    } catch (const std::exception &) {}
    if (!file_writable)
      throw std::runtime_error("Cannot open file " + outFile_s + " for writing");

    // Create temporary output file name.
    std::string tmpOutFile_s = tmpFileName(outFile_s);

    // Open the input file.
    tip::TipFile inTipFile = tip::IFileSvc::instance().openFile(inFile_s);
  
    // Copy the input to the temporary output file.
    inTipFile.copyFile(tmpOutFile_s, true);

    // Set reference frame for the given solar system ephemeris.
    std::string solar_eph = pars["solareph"];
    std::string solar_eph_uc = solar_eph;
    for (std::string::iterator itor = solar_eph_uc.begin(); itor != solar_eph_uc.end(); ++itor) *itor = std::toupper(*itor);
    std::string pl_ephem;
    std::string refFrame;
    if (solar_eph_uc == "JPL DE200") {
      pl_ephem = "JPL-DE200";
      refFrame = "FK5";
    } else if (solar_eph_uc == "JPL DE405") {
      pl_ephem = "JPL-DE405";
      refFrame = "ICRS";
    } else {
      throw std::runtime_error("Solar system ephemeris \"" + solar_eph + "\" not supported");
    }

    // Handle leap seconds.
    std::string leap_sec_file = pars["leapsecfile"];
    timeSystem::TimeSystem::setDefaultLeapSecFileName(leap_sec_file);

    // Get RA and Dec.
    double ra = pars["ra"];
    double dec = pars["dec"];

    // Modify the output file so that an appropriate EventTimeHandler object will be created from it.
    for (tip::FileSummary::size_type ext_index = 0; ext_index < file_summary.size(); ++ext_index) {
      // Open the extension of the output file.
      std::ostringstream oss;
      oss << ext_index;
      std::string ext_name = oss.str();
      std::auto_ptr<tip::Extension> output_extension(tip::IFileSvc::instance().editExtension(tmpOutFile_s, ext_name));

      // Change the header keywords of the output file that determine how to interpret event times.
      tip::Header & output_header = output_extension->getHeader();
      output_header["TIMESYS"].set(target_time_sys);
      output_header["TIMESYS"].setComment("type of time system that is used");
      output_header["TIMEREF"].set(target_time_ref);
      output_header["TIMEREF"].setComment("reference frame used for times");

      // Update header keywords with parameters of arrival time corrections.
      output_header["RA_NOM"].set(ra);
      output_header["RA_NOM"].setComment("Right Ascension used for arrival time corrections");
      output_header["DEC_NOM"].set(dec);
      output_header["DEC_NOM"].setComment("Declination used for arrival time corrections");
      output_header["RADECSYS"].set(refFrame);
      output_header["RADECSYS"].setComment("coordinate reference system");
      output_header["PLEPHEM"].set(pl_ephem);
      output_header["PLEPHEM"].setComment("solar system ephemeris used for arrival time corrections");
      output_header["TIMEZERO"].set(0.);
      output_header["TIMEZERO"].setComment("clock correction");
      output_header["CREATOR"].set(getName() + " " + getVersion());
      output_header["CREATOR"].setComment("software and version creating file");
      output_header["DATE"].set(output_header.formatTime(time(0)));
      output_header["DATE"].setComment("file creation date (YYYY-MM-DDThh:mm:ss UT)");

      // Determine TIERRELA value, in the same manner as in axBary.c by Arnold Rots, and set it to the header.
      double tierrela = -1.;
      try {
        output_header["TIERRELA"].get(tierrela);
      } catch (const tip::TipException &) {
        tierrela = 1.e-9;
      }
      if (tierrela > 0.) {
        output_header["TIERRELA"].set(tierrela);
        output_header["TIERRELA"].setComment("short-term clock stability");
      }

      // Determine TIERABSO value, and set it to the header if necessary.
      // Note: A value of TIERABSO is dependent on a mission and an instrument, and TIERABSO was added only for XTE cases
      //       in axBary.c by Arnold Rots. Leave this part commented out until the keyword becomes necessary.
      //double tierabso = 0.0;
      //output_header["TIERABSO"].set(tierabso);
      //output_header["TIERABSO"].setComment("absolute precision of clock correction");
    }

    // Get spacecraft file name, spacecraft data extension name, and angular tolerance.
    std::string orbitFile_s = pars["scfile"];
    std::string sc_extension = pars["sctable"];
    double ang_tolerance = pars["angtol"];

    // List header keyword names to convert.
    std::list<std::string> keyword_list;
    keyword_list.push_back("TSTART");
    keyword_list.push_back("TSTOP");
    keyword_list.push_back("DATE-OBS");
    keyword_list.push_back("DATE-END");

    // List column names to convert.
    std::list<std::string> column_gti;
    column_gti.push_back("START");
    column_gti.push_back("STOP");
    std::list<std::string> column_other;
    std::string time_field = pars["timefield"];
    column_other.push_back(time_field);

    // Loop over all extensions in input and output files, including primary HDU.
    ext_number = 0;
    for (tip::FileSummary::const_iterator ext_itor = file_summary.begin(); ext_itor != file_summary.end(); ++ext_itor, ++ext_number) {
      // Open this extension of the input file, and the corresponding extension of the output file.
      std::auto_ptr<EventTimeHandler> input_handler(0);
      std::auto_ptr<EventTimeHandler> output_handler(0);
      for (factory_cont_type::const_iterator fact_itor = factory_cont.begin();
        fact_itor != factory_cont.end() && (0 == input_handler.get() || (0 == output_handler.get())); ++fact_itor) {
        std::pair<EventTimeHandler *, EventTimeHandler *> handler_pair = (*fact_itor)->create(inFile_s, tmpOutFile_s, ext_number);
        input_handler.reset(handler_pair.first);
        output_handler.reset(handler_pair.second);
      }

      // Check whether both of the input and the output files were successfully opened or not.
      if (0 == input_handler.get() || 0 == output_handler.get()) {
        std::ostringstream oss;
        oss << "Arrival time correction \"" << t_correct << "\" not supported for HDU " << ext_number;
        if (0 == ext_number) oss << " (primary HDU)";
        else oss << " (EXTNAME=" << ext_itor->getExtId() << ")";
        oss << " of input file \"" << inFile_s << "\"";
        throw std::runtime_error(oss.str());
      }

      // Write out all the parameters into HISTORY keywords.
      tip::Header & output_header = output_handler->getHeader();
      std::string creator_value;
      output_header["CREATOR"].get(creator_value);
      std::string date_value;
      output_header["DATE"].get(date_value);
      output_header.addHistory("File created or modified by " + creator_value + " on " + date_value);
      const st_app::AppParGroup & const_pars(pars);
      for (hoops::ConstGenParItor par_itor = const_pars.begin(); par_itor != const_pars.end(); ++par_itor) {
        std::ostringstream oss_par;
        oss_par << getName() << ".par: " << **par_itor;
        output_header.addHistory(oss_par.str());
      }

      // Initialize arrival time corrections.
      // Note: Always require for solar system ephemeris to match between successive arrival time conversions.
      static const bool match_solar_eph = true;
      input_handler->initTimeCorrection(orbitFile_s, sc_extension, solar_eph, match_solar_eph, ang_tolerance);
      input_handler->setSourcePosition(ra, dec);

      // Apply arrival time correction to header keyword values.
      for (std::list<std::string>::const_iterator name_itor = keyword_list.begin(); name_itor != keyword_list.end(); ++name_itor) {
        const std::string & keyword_name = *name_itor;
        try {
          if ("BARY" == t_correct_uc) {
            output_handler->writeTime(keyword_name, input_handler->getBaryTime(keyword_name, true), true);
          } else if ("GEO" == t_correct_uc) {
            output_handler->writeTime(keyword_name, input_handler->getGeoTime(keyword_name, true), true);
          }
        } catch (const tip::TipException &) {
          // Skip if this keyword does not exist.
        }
      }

      // Select columns to convert.
      const std::list<std::string> & column_list = ("GTI" == ext_itor->getExtId() ? column_gti : column_other);

      // Loop over all FITS rows.
      output_handler->setFirstRecord();
      for (input_handler->setFirstRecord(); !(input_handler->isEndOfTable() || output_handler->isEndOfTable());
        input_handler->setNextRecord(), output_handler->setNextRecord()) {

        // Apply arrival time correction to the specified columns.
        for (std::list<std::string>::const_iterator name_itor = column_list.begin(); name_itor != column_list.end(); ++name_itor) {
          const std::string & column_name = *name_itor;
          try {
            if ("BARY" == t_correct_uc) {
              output_handler->writeTime(column_name, input_handler->getBaryTime(column_name));
            } else if ("GEO" == t_correct_uc) {
              output_handler->writeTime(column_name, input_handler->getGeoTime(column_name));
            }
          } catch (const tip::TipException &) {
            // Skip if this column does not exist.
          }
        }
      }
    }

    // Move the temporary output file to the real output file.
    std::remove(outFile_s.c_str());
    std::rename(tmpOutFile_s.c_str(), outFile_s.c_str());

    // Clean up.
    for (factory_cont_type::reverse_iterator fact_itor = factory_cont.rbegin(); fact_itor != factory_cont.rend(); ++fact_itor) {
      delete *fact_itor;
      *fact_itor = 0;
    }
  }

  std::string TimeCorrectorApp::tmpFileName(const std::string & file_name) const {
    return file_name + ".tmp";
  }

}
