#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <stdexcept>
#include <string>

#include "st_app/AppParGroup.h"
#include "st_app/StApp.h"
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

using namespace timeSystem;

class GbaryApp : public st_app::StApp {
  public:
    GbaryApp();
    virtual void run();
    std::string tmpFileName(const std::string & file_name) const;
};

GbaryApp::GbaryApp() {
  setName("gtbary");
  setVersion(s_cvs_id);
}

void GbaryApp::run() {
  st_app::AppParGroup & pars = getParGroup();
  pars.Prompt();
  pars.Save();
  std::string inFile_s = pars["evfile"];
  std::string orbitFile_s = pars["scfile"];
  std::string outFile_s = pars["outfile"];
  double ra = pars["ra"];
  double dec = pars["dec"];
  std::string t_correct = pars["tcorrect"];
  std::string solar_eph = pars["solareph"];
  double ang_tolerance = pars["angtol"];
  std::string sc_extension = pars["sctable"];
  bool clobber = pars["clobber"];

  // Check time correction mode.
  std::string t_correct_uc(t_correct);
  for (std::string::iterator itor = t_correct_uc.begin(); itor != t_correct_uc.end(); ++itor) *itor = std::toupper(*itor);
  if ("BARY" != t_correct_uc) {
    throw std::runtime_error("Unsupported arrival time correction: " + t_correct_uc);
  }

  // Check whether output file name already exists or not, if clobber parameter is set to no.
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
  column_other.push_back("TIME");

  // Get the number of extensions of the input FITS file.
  tip::FileSummary file_summary;
  tip::IFileSvc::instance().getFileSummary(inFile_s, file_summary);

  // Modify the output file so that an appropriate EventTimeHandler object will be created from it.
  for (tip::FileSummary::size_type ext_index = 0; ext_index < file_summary.size(); ++ext_index) {
    // Open the extension of the output file.
    std::ostringstream oss;
    oss << ext_index;
    std::string ext_name = oss.str();
    std::auto_ptr<tip::Extension> output_extension(tip::IFileSvc::instance().editExtension(tmpOutFile_s, ext_name));

    // Change the header keywords of the output file that determine how to interpret event times.
    tip::Header & output_header = output_extension->getHeader();
    output_header["TIMESYS"].set("TDB");
    output_header["TIMESYS"].setComment("type of time system that is used");
    output_header["TIMEREF"].set("SOLARSYSTEM");
    output_header["TIMEREF"].setComment("reference frame used for times");
  }

  // Loop over all extensions in input and output files, including primary HDU.
  int ext_number = 0;
  for (tip::FileSummary::const_iterator ext_itor = file_summary.begin(); ext_itor != file_summary.end(); ++ext_itor, ++ext_number) {
    // Open this extension of the input file, and the corresponding extension of the output file.
    std::ostringstream oss;
    oss << ext_number;
    std::string ext_name = oss.str();
    std::auto_ptr<EventTimeHandler> input_handler(IEventTimeHandlerFactory::createHandler(inFile_s, ext_name, true));
    std::auto_ptr<EventTimeHandler> output_handler(IEventTimeHandlerFactory::createHandler(tmpOutFile_s, ext_name, false));

    // Update header keywords with parameters of barycentric corrections.
    tip::Header & output_header = output_handler->getHeader();
    output_header["RA_NOM"].set(ra);
    output_header["RA_NOM"].setComment("Right Ascension used for barycentric corrections");
    output_header["DEC_NOM"].set(dec);
    output_header["DEC_NOM"].setComment("Declination used for barycentric corrections");
    output_header["RADECSYS"].set(refFrame);
    output_header["RADECSYS"].setComment("coordinate reference system");
    output_header["PLEPHEM"].set(pl_ephem);
    output_header["PLEPHEM"].setComment("solar system ephemeris used for barycentric corrections");
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

    // Determine TIERABSO value, and set it to the header.
    // TODO: Add mission-dependent determination of TIERABSO value?
    // Note: TIERABSO was not added for GLAST by axBary.c by Arnold Rots.
    //output_header["TIERABSO"].set(???);
    //output_header["TIERABSO"].setComment("absolute precision of clock correction");

    // Initialize arrival time corrections.
    // Note: Always require for solar system ephemeris to match between successive arrival time conversions.
    static const bool match_solar_eph = true;
    input_handler->initTimeCorrection(orbitFile_s, sc_extension, solar_eph, match_solar_eph, ang_tolerance);
    input_handler->setSourcePosition(ra, dec);

    // Apply barycentric correction to header keyword values.
    for (std::list<std::string>::const_iterator name_itor = keyword_list.begin(); name_itor != keyword_list.end(); ++name_itor) {
      const std::string & keyword_name = *name_itor;
      try {
        const AbsoluteTime abs_time = input_handler->getBaryTime(keyword_name, true);
        output_handler->writeTime(keyword_name, abs_time, true);
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

      // Apply barycentric correction to the specified columns.
      for (std::list<std::string>::const_iterator name_itor = column_list.begin(); name_itor != column_list.end(); ++name_itor) {
        const std::string & column_name = *name_itor;
        try {
          const AbsoluteTime abs_time = input_handler->getBaryTime(column_name);
          output_handler->writeTime(column_name, abs_time);
        } catch (const tip::TipException &) {
          // Skip if this column does not exist.
        }
      }
    }
  }

  // Move the temporary output file to the real output file.
  std::remove(outFile_s.c_str());
  std::rename(tmpOutFile_s.c_str(), outFile_s.c_str());
}

std::string GbaryApp::tmpFileName(const std::string & file_name) const {
  return file_name + ".tmp";
}

// List supported mission(s).
timeSystem::EventTimeHandlerFactory<timeSystem::GlastTimeHandler> glast_handler;

st_app::StAppFactory<GbaryApp> g_factory("gtbary");
