/** \file GlastTimeHandler.cxx
    \brief Implementation of GlastTimeHandler class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/GlastTimeHandler.h"

#include "timeSystem/AbsoluteTime.h"
#include "timeSystem/GlastMetRep.h"
#include "timeSystem/TimeRep.h"

#include <sstream>
#include <stdexcept>

extern "C" {
#include "bary.h"
}

namespace timeSystem {

  GlastTimeHandler::GlastTimeHandler(tip::Table & table, const std::string & sc_file, double position_tolerance):
    EventTimeHandler(table, position_tolerance), m_sc_file(sc_file) {
    // Get time system from TIMESYS keyword.
    const tip::Header & header(getHeader());
    std::string time_system;
    header["TIMESYS"].get(time_system);
    for (std::string::iterator itor = time_system.begin(); itor != time_system.end(); ++itor) *itor = std::toupper(*itor);

    // Create TimeRep.
    m_time_rep = new GlastMetRep(time_system, 0.);

    // Set space craft file to internal variable.
    // TODO: Any better way than below?
    m_sc_file_char = (char *)m_sc_file.c_str();

    // Initialize clock and orbit
    // TODO: Need to call these for GLAST?
    enum Observatory mission(GLAST);
    scorbitinit (mission);
    clockinit (mission);
  }

  GlastTimeHandler::~GlastTimeHandler() {
    delete m_time_rep;
  }

  AbsoluteTime GlastTimeHandler::readHeader(const std::string & keyword_name, const bool request_bary_time,
    const double ra, const double dec) {
    // Read keyword value from header as a signle variable of double type.
    const tip::Header & header(getHeader());
    double keyword_value = 0.;
    header[keyword_name].get(keyword_value);

    // Convert keyword value to AbsoluteTime.
    m_time_rep->set("TIME", keyword_value);
    AbsoluteTime abs_time(*m_time_rep);

    if (request_bary_time) {
      // Get space craft position at the given time.
      int error = 0;
      double * sc_position = xscorbit(m_sc_file_char, keyword_value, &error);

      // Perform barycentric correction on abs_time.
      computeBaryTime(ra, dec, sc_position, abs_time);
    }

    // Return the requested time.
    return abs_time;
  }

  AbsoluteTime GlastTimeHandler::readColumn(const std::string & column_name, const bool request_bary_time,
    const double ra, const double dec) {
    // Read column value from a given record as a signle variable of double type.
    tip::TableRecord & record(getCurrentRecord());
    double column_value = record[column_name].get();

    // Convert keyword value to AbsoluteTime.
    m_time_rep->set("TIME", column_value);
    AbsoluteTime abs_time(*m_time_rep);

    if (request_bary_time) {
      // Get space craft position at the given time.
      int error = 0;
      double * sc_position = xscorbit(m_sc_file_char, column_value, &error);

      // Perform barycentric correction on abs_time.
      computeBaryTime(ra, dec, sc_position, abs_time);
    }

    // Return the requested time.
    return abs_time;
  }
}
