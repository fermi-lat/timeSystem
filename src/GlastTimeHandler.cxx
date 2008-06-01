/** \file GlastTimeHandler.cxx
    \brief Implementation of GlastTimeHandler class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/GlastTimeHandler.h"

#include "timeSystem/AbsoluteTime.h"
#include "timeSystem/ElapsedTime.h"
#include "timeSystem/IntFracPair.h"

#include "tip/IFileSvc.h"

#include <sstream>
#include <stdexcept>
#include <vector>

extern "C" {
#include "bary.h"
}

namespace timeSystem {

  GlastTimeHandler::GlastTimeHandler(const std::string & file_name, const std::string & extension_name, const double angular_tolerance,
    const bool read_only): EventTimeHandler(file_name, extension_name, angular_tolerance, read_only), m_time_system(),
    m_mjd_ref(0, 0.), m_sc_file(), m_sc_file_char(0) {
    // Get time system from TIMESYS keyword.
    const tip::Header & header(getHeader());
    header["TIMESYS"].get(m_time_system);
    for (std::string::iterator itor = m_time_system.begin(); itor != m_time_system.end(); ++itor) *itor = std::toupper(*itor);

    // Get MJDREF value.
    m_mjd_ref = readMjdRef(header);
  }

  GlastTimeHandler::~GlastTimeHandler() {}

  void GlastTimeHandler::setSpacecraftFile(const std::string & sc_file_name, const std::string & sc_extension_name) {
    // Check header keywords.
    if (!checkHeaderKeyword(sc_file_name, sc_extension_name)) {
      throw std::runtime_error("Unsupported spacecraft file \"" + sc_file_name + "[" + sc_extension_name + "]\"");
    }

    // Set space craft file to internal variable.
    m_sc_file = sc_file_name;
    m_sc_file_char = const_cast<char *>(m_sc_file.c_str());

    // Initializing clock and orbit are not necessary for GLAST.
    // Note: Leave these here as a reminder of an official way to call them.
    //enum Observatory mission(GLAST);
    //scorbitinit(mission);
    //clockinit(mission);
  }

  AbsoluteTime GlastTimeHandler::parseTimeString(const std::string & time_string, const std::string & time_system) const {
    // Rationalize time system name.
    std::string time_system_rat(time_system);
    for (std::string::iterator itor = time_system_rat.begin(); itor != time_system_rat.end(); ++itor)
      *itor = std::toupper(*itor);
    if ("FILE" == time_system_rat) time_system_rat = m_time_system;

    // Parse time string into an absolute time, and return it.
    return AbsoluteTime(time_system_rat, m_mjd_ref) + ElapsedTime(time_system_rat, Duration(IntFracPair(time_string), Sec));
  }

  EventTimeHandler * GlastTimeHandler::createInstance(const std::string & file_name, const std::string & extension_name,
    const double angular_tolerance, const bool read_only) {
    // Create an object to hold a return value and set a default return value.
    EventTimeHandler * handler(0);

    // Check header keywords to identify
    if (checkHeaderKeyword(file_name, extension_name)) {
      handler = new GlastTimeHandler(file_name, extension_name, angular_tolerance, read_only);
    }

    // Return the handler (or zero if this class cannot handle it).
    return handler;
  }

  bool GlastTimeHandler::checkHeaderKeyword(const std::string & file_name, const std::string & extension_name) {
    // Get the table and the header.
    std::auto_ptr<const tip::Table> table(tip::IFileSvc::instance().readTable(file_name, extension_name));
    const tip::Header & header(table->getHeader());

    // Get TELESCOP keyword value.
    std::string telescope;
    header["TELESCOP"].get(telescope);
    for (std::string::iterator itor = telescope.begin(); itor != telescope.end(); ++itor) *itor = std::toupper(*itor);

    // Get INSTRUME keyword value.
    std::string instrument;
    header["INSTRUME"].get(instrument);
    for (std::string::iterator itor = instrument.begin(); itor != instrument.end(); ++itor) *itor = std::toupper(*itor);

    // Return whether this class can handle the file or not.
    return (telescope == "GLAST" && instrument == "LAT");
  }

  AbsoluteTime GlastTimeHandler::readTime(const tip::Header & header, const std::string & keyword_name, const bool request_bary_time,
    const double ra, const double dec) const {
    // Read keyword value from header as a signle variable of double type.
    double keyword_value = 0.;
    header[keyword_name].get(keyword_value);

    // Compute an absolute time and return it.
    return computeAbsoluteTime(keyword_value, request_bary_time, ra, dec);
  }

  AbsoluteTime GlastTimeHandler::readTime(const tip::TableRecord & record, const std::string & column_name, const bool request_bary_time,
    const double ra, const double dec) const {
    // Read column value from a given record as a signle variable of double type.
    double column_value;
    record[column_name].get(column_value);

    // Compute an absolute time and return it.
    return computeAbsoluteTime(column_value, request_bary_time, ra, dec);
  }

  AbsoluteTime GlastTimeHandler::computeAbsoluteTime(const double glast_time, const bool request_bary_time,
    const double ra, const double dec) const {
    // Convert GLAST time to AbsoluteTime.
    AbsoluteTime abs_time = AbsoluteTime(m_time_system, m_mjd_ref) + ElapsedTime(m_time_system, Duration(0, glast_time));

    if (request_bary_time) {
      // Check spacecraft file.
      if (m_sc_file.empty()) throw std::runtime_error("Spacecraft file not properly set.");

      // Get space craft position at the given time.
      int error = 0;
      double * sc_position_array = glastscorbit(m_sc_file_char, glast_time, &error);
      if (error) {
        std::ostringstream os;
        os << "Error in getting GLAST spacecraft position for " << glast_time << " GLAST MET (" << m_time_system << ").";
        throw std::runtime_error(os.str());
      }
      std::vector<double> sc_position(sc_position_array, sc_position_array + 3);

      // Perform barycentric correction on abs_time.
      computeBaryTime(ra, dec, sc_position, abs_time);
    }

    // Return the requested time.
    return abs_time;
  }
}
