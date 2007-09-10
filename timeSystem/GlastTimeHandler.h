/** \file GlastTimeHandler.h
    \brief Declaration of GlastTimeHandler class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_GlastTimeHandler_h
#define timeSystem_GlastTimeHandler_h

#include "timeSystem/EventTimeHandler.h"

#include "tip/Table.h"

#include <string>

namespace timeSystem {

  class AbsoluteTime;
  class BaryTimeComputer;
  class TimeRep;

  /** \class GlastTimeHandler
      \brief Class which reads out event times from an event file, creates AbsoluteTime objects for event times,
             and performs barycentric correction on event times, typically recorded at a space craft.
  */
  class GlastTimeHandler: public EventTimeHandler {
    public:
      GlastTimeHandler(tip::Table & table, const std::string & sc_file, double position_tolerance = 0.);

      virtual ~GlastTimeHandler();

    protected:
      virtual AbsoluteTime readHeader(const std::string & keyword_name, const bool request_bary_time, const double ra, const double dec);

      virtual AbsoluteTime readColumn(const std::string & column_name, const bool request_bary_time, const double ra, const double dec);

    private:
      std::string m_sc_file;
      char * m_sc_file_char;

      // TODO: Consider replacing MetRep's with classes derived from this class.
      TimeRep * m_time_rep;
  };

}

#endif