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

      static EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name, bool read_only = true);

      virtual void initTimeCorrection(const std::string & sc_file_name, const std::string & sc_extension_name, 
        const std::string & solar_eph, bool match_solar_eph, double angular_tolerance) = 0;

      virtual void setSourcePosition(double ra, double dec) = 0;

      virtual AbsoluteTime readTime(const std::string & field_name, bool from_header = false) const;

      virtual AbsoluteTime getBaryTime(const std::string & field_name, bool from_header = false) const = 0;

      virtual AbsoluteTime parseTimeString(const std::string & time_string, const std::string & time_system = "FILE") const;

    protected:
      GlastTimeHandler(const std::string & file_name, const std::string & extension_name, bool read_only = true);

      static bool checkHeaderKeyword(const std::string & file_name, const std::string & extension_name,
        const std::string & time_ref_value, const std::string & time_sys_value);

      double readGlastTime(const std::string & field_name, bool from_header) const;

      AbsoluteTime computeAbsoluteTime(double glast_time) const;

      AbsoluteTime computeAbsoluteTime(double glast_time, const std::string & time_system_name) const;

    private:
      const TimeSystem * m_time_system;
      Mjd m_mjd_ref;
  };

  /** \class GlastScTimeHandler
      \brief Class which reads out event times from a GLAST event file that is not applied barycentric corrections,
             creates AbsoluteTime objects for event times, and performs barycentric correction on event times,
             typically recorded at a space craft.
  */
  class GlastScTimeHandler: public GlastTimeHandler {
    public:
      virtual ~GlastScTimeHandler();

      static EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name, bool read_only = true);

      virtual void initTimeCorrection(const std::string & sc_file_name, const std::string & sc_extension_name, 
        const std::string & solar_eph, bool match_solar_eph, double angular_tolerance);

      virtual void setSourcePosition(double ra, double dec);

      virtual AbsoluteTime getBaryTime(const std::string & field_name, bool from_header = false) const;

    private:
      std::string m_sc_file;
      char * m_sc_file_char;
      double m_ra_bary;  // RA and Dec for barycentering.
      double m_dec_bary;
      const BaryTimeComputer * m_computer;

      GlastScTimeHandler(const std::string & file_name, const std::string & extension_name, bool read_only = true);
  };

  /** \class GlastBaryTimeHandler
      \brief Class which reads out event times from a GLAST event file that is applied barycentric corrections,
             creates AbsoluteTime objects for event times, and performs barycentric correction on event times,
             typically recorded at a space craft.
  */
  class GlastBaryTimeHandler: public GlastTimeHandler {
    public:
      virtual ~GlastBaryTimeHandler();

      static EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name, bool read_only = true);

      virtual void initTimeCorrection(const std::string & sc_file_name, const std::string & sc_extension_name, 
        const std::string & solar_eph, bool match_solar_eph, double angular_tolerance);

      virtual void setSourcePosition(double ra, double dec);

      virtual AbsoluteTime getBaryTime(const std::string & field_name, bool from_header = false) const;

    private:
      std::string m_file_name;
      std::string m_ext_name;
      double m_ra_nom;  // RA and Dec from an event file header.
      double m_dec_nom;
      std::vector<double> m_vect_nom; // Three vector representation of m_ra_nom and m_dec_nom.
      double m_max_vect_diff;
      std::string m_pl_ephem;

      GlastBaryTimeHandler(const std::string & file_name, const std::string & extension_name, bool read_only = true);

      std::vector<double> computeThreeVector(double ra, double dec) const;
  };

}

#endif
