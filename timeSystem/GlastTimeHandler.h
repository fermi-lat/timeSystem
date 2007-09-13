/** \file GlastTimeHandler.h
    \brief Declaration of GlastTimeHandler class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_GlastTimeHandler_h
#define timeSystem_GlastTimeHandler_h

#include "timeSystem/EventTimeHandler.h"

#include <string>

namespace tip {
  class Header;
  class Table;
  class TableRecord;
}

namespace timeSystem {

  class AbsoluteTime;
  class BaryTimeComputer;
  class TimeRep;

  /** \class GlastTimeHandler
      \brief Class which reads out event times from a GLAST event file, creates AbsoluteTime objects for event times,
             and performs barycentric correction on event times, typically recorded at a space craft.
  */
  class GlastTimeHandler: public EventTimeHandler {
    public:
      virtual ~GlastTimeHandler();

      static EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name,
        const std::string & sc_file_name, const std::string & sc_extension_name, const double angular_tolerance = 0.);

    protected:
      virtual AbsoluteTime readTime(const tip::Header & header, const std::string & keyword_name, const bool request_bary_time,
        const double ra, const double dec);

      virtual AbsoluteTime readTime(const tip::TableRecord & record, const std::string & column_name, const bool request_bary_time,
        const double ra, const double dec);

    private:
      std::string m_sc_file;
      char * m_sc_file_char;

      GlastTimeHandler(const std::string & file_name, const std::string & extension_name, const std::string & sc_file,
        const double position_tolerance = 0.);

      // TODO: Consider replacing MetRep's with classes derived from this class.
      TimeRep * m_time_rep;

      AbsoluteTime computeAbsoluteTime(const double glast_time, const bool request_bary_time, const double ra, const double dec);

      static bool checkHeaderKeyword(const std::string & file_name, const std::string & extension_name);
  };

}

#endif
