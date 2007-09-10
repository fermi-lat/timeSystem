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

    // Initializing clock and orbit are not necessary for GLAST.
    // Note: Leave these here as a reminder of an official way to call them.
    //enum Observatory mission(GLAST);
    //scorbitinit(mission);
    //clockinit(mission);
  }

  GlastTimeHandler::~GlastTimeHandler() {
    delete m_time_rep;
  }

  AbsoluteTime GlastTimeHandler::readTime(const tip::Header & header, const std::string & keyword_name, const bool request_bary_time,
    const double ra, const double dec) {
    // Read keyword value from header as a signle variable of double type.
    double keyword_value = 0.;
    header[keyword_name].get(keyword_value);

    // Compute an absolute time and return it.
    return computeAbsoluteTime(keyword_value, request_bary_time, ra, dec);
  }

  AbsoluteTime GlastTimeHandler::readTime(const tip::TableRecord & record, const std::string & column_name, const bool request_bary_time,
    const double ra, const double dec) {
    // Read column value from a given record as a signle variable of double type.
    double column_value;
    record[column_name].get(column_value);

    // Compute an absolute time and return it.
    return computeAbsoluteTime(column_value, request_bary_time, ra, dec);
  }

  AbsoluteTime GlastTimeHandler::computeAbsoluteTime(const double glast_time, const bool request_bary_time,
    const double ra, const double dec) {
    // Convert GLAST time to AbsoluteTime.
    m_time_rep->set("TIME", glast_time);
    AbsoluteTime abs_time(*m_time_rep);

    if (request_bary_time) {
      // Get space craft position at the given time.
      int error = 0;
      double * sc_position = glastscorbit(m_sc_file_char, glast_time, &error);
      if (error) {
        std::ostringstream os;
        os << "Error in getting GLAST spacecraft position for " << *m_time_rep << ".";
        throw std::runtime_error(os.str());
      }

      // Perform barycentric correction on abs_time.
      computeBaryTime(ra, dec, sc_position, abs_time);
    }

    // Return the requested time.
    return abs_time;
  }
}
