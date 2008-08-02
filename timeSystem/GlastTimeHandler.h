/** \file GlastTimeHandler.h
    \brief Declaration of GlastTimeHandler class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_GlastTimeHandler_h
#define timeSystem_GlastTimeHandler_h

#include "timeSystem/AbsoluteTime.h"
#include "timeSystem/EventTimeHandler.h"
#include "timeSystem/MjdFormat.h"

#include <string>

namespace tip {
  class Header;
  class Table;
  class TableRecord;
}

namespace timeSystem {

  class BaryTimeComputer;

  /** \class GlastTimeHandler
      \brief Class which reads out event times from a GLAST event file, creates AbsoluteTime objects for event times,
             and performs barycentric correction on event times, typically recorded at a space craft.
  */
  class GlastTimeHandler: public EventTimeHandler {
    public:
      virtual ~GlastTimeHandler();

      static EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name,
        double angular_tolerance, bool read_only = true);

      virtual void setSpacecraftFile(const std::string & sc_file_name, const std::string & sc_extension_name);

      virtual AbsoluteTime parseTimeString(const std::string & time_string, const std::string & time_system = "FILE") const;

    protected:
      virtual AbsoluteTime readTime(const tip::Header & header, const std::string & keyword_name, bool request_bary_time,
        double ra, double dec) const;

      virtual AbsoluteTime readTime(const tip::TableRecord & record, const std::string & column_name, bool request_bary_time,
        double ra, double dec) const;

    private:
      const TimeSystem * m_time_system;
      Mjd m_mjd_ref;
      std::string m_sc_file;
      char * m_sc_file_char;

      GlastTimeHandler(const std::string & file_name, const std::string & extension_name, double angular_tolerance,
        bool read_only = true);

      AbsoluteTime computeAbsoluteTime(double glast_time, bool request_bary_time, double ra, double dec) const;

      static bool checkHeaderKeyword(const std::string & file_name, const std::string & extension_name);
  };

}

#endif
