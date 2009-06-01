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
      \brief Class which reads out event times from a Fermi (formerly GLAST) event file, creates AbsoluteTime objects for event times,
             and performs barycentric correction on event times, typically recorded at a space craft.
  */
  class GlastTimeHandler: public EventTimeHandler {
    public:
      /// Destruct this GlastTimeHandler object.
      virtual ~GlastTimeHandler();

      /** \brief Create an instance of a concrete GlastTimeHandler subclass, and return the pointer to the instance.
                 If failed to open a given file, this method returns 0 (null pointer).
          \param file_name Name of FITS file to open.
          \param extension_name Name of FITS extension to open.
          \param read_only Set to true to open the file in a read-only mode. Set to false to open it in a read-write mode.
      */
      static EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name, bool read_only = true);

      /** \brief Initialize arrival time corrections.
          \param sc_file_name Name of spacecraft file to be used for arrival time corrections.
          \param sc_extension_name Name of FITS table that contains spacecraft data in the above file.
          \param solar_eph Name of solar system ephemeris to use for arrival time corrections.
          \param match_solar_eph Set to true if the above solar system ephemeris must match the one written in the opened file.
          \param angular_tolerance Maximum angular difference in degrees allowed in comparison of sky position between
                 the one given to setSourcePosition method and the one in the opened file.
      */
      virtual void initTimeCorrection(const std::string & sc_file_name, const std::string & sc_extension_name, 
        const std::string & solar_eph, bool match_solar_eph, double angular_tolerance) = 0;

      /** \brief Set the source position for arrival time corrections.
          \param ra Right Ascension of the source position.
          \param dec Declination of the source position.
      */
      virtual void setSourcePosition(double ra, double dec) = 0;

      /** \brief Read a given field of the opened FITS header or the current record of the opened FITS table,
                 compute an absolute time that it represents, and return it.
          \param field_name Name of field from which a time is to be read.
          \param from_header Set to true to read a time from the header. Set to false to read a time from the current record
                 of the table.
      */
      virtual AbsoluteTime readTime(const std::string & field_name, bool from_header = false) const;

      /** \brief Write a given absolute time to a specified field of the opened FITS header or the current record of
                 the opened FITS tale.
          \param field_name Name of field to which a given time is written.
          \param abs_time Absolute time to write to the above field.
          \param to_header Set to true to write the time to the header. Set to false to write it to the current record of the table.
      */
      virtual void writeTime(const std::string & field_name, const AbsoluteTime & abs_time, bool to_header = false);

      /** \brief Read a given field of the opened FITS header or the current record of the opened FITS table,
                 compute an absolute time that it represents, compute a geocentric time for it, and return it.
          \param field_name Name of field from which a time is to be read.
          \param from_header Set to true to read a time from the header. Set to false to read a time from the current record
                 of the table.
      */
      virtual AbsoluteTime getGeoTime(const std::string & field_name, bool from_header = false) const = 0;

      /** \brief Read a given field of the opened FITS header or the current record of the opened FITS table,
                 compute an absolute time that it represents, compute a barycentric time for it, and return it.
          \param field_name Name of field from which a time is to be read.
          \param from_header Set to true to read a time from the header. Set to false to read a time from the current record
                 of the table.
      */
      virtual AbsoluteTime getBaryTime(const std::string & field_name, bool from_header = false) const = 0;

      /** \brief Interpret a given character string as a time value in the same format as in the opened file,
                 compute an absolute time that it represents, and return it.
          \param time_string Character string to interpret as a time value.
          \param time_system Time system name to use to interpret the time value. This method uses TIMESYS header keyword value
                             if this argument is set to "FILE".
      */
      virtual AbsoluteTime parseTimeString(const std::string & time_string, const std::string & time_system = "FILE") const;

    protected:
      /** \brief Construct a GlastTimeHandler object.
          \param file_name Name of FITS file to open.
          \param extension_name Name of FITS extension to open.
          \param read_only Set to true to open the file in a read-only mode. Set to false to open it in a read-write mode.
      */
      GlastTimeHandler(const std::string & file_name, const std::string & extension_name, bool read_only = true);

      /** \brief Helper method for createInstance method to check the header keyword required for Fermi (formerly GLAST) event files
                 in order to determine a given file can be handled by this class.
          \param file_name Name of FITS file to open.
          \param extension_name Name of FITS extension to open.
          \param time_ref_value Value of TIMEREF header keyword to accept.
          \param time_sys_value Value of TIMESYS header keyword to accept.
      */
      static bool checkHeaderKeyword(const std::string & file_name, const std::string & extension_name,
        const std::string & time_ref_value, const std::string & time_sys_value);

      /** \brief Read a Fermi (formerly GLAST) Mission Elapsed Time (MET) from a given field of the opened FITS header
                 or the current record of the opened FITS table, and return it.
          \param field_name Name of field from which a Fermi (formerly GLAST) MET is to be read.
          \param from_header Set to true to read a Fermi (formerly GLAST) MET from the header. Set to false
                 to read a time from the current record of the table.
      */
      double readGlastTime(const std::string & field_name, bool from_header) const;

      /** \brief Write a Fermi (formerly GLAST) Mission Elapsed Time (MET) to a specified field of the opened FITS header
                 or the current record of the opened FITS table, and return it.
          \param field_name Name of field to which a given time is written.
          \param glast_time Fermi (formerly GLAST) MET to write to the above field.
          \param to_header Set to true to write the time to the header. Set to false to write it to the current record of the table.
       */
      void writeGlastTime(const std::string & field_name, double glast_time, bool to_header);

      /** \brief Create an AbsoluteTime object that represents a given Fermi (formerly GLAST) Mission Elapsed Time (MET), and return it.
          \param glast_time Fermi (formerly GLAST) MET to convert to an AbsoluteTime object.
      */
      AbsoluteTime computeAbsoluteTime(double glast_time) const;

      /** \brief Create an AbsoluteTime object that represents a given Fermi (formerly GLAST) Mission Elapsed Time (MET)
                 in a specified time system, and return it.
          \param glast_time Fermi (formerly GLAST) MET to convert to an AbsoluteTime object.
          \param time_system_name Name of the time system in which a given Fermi (formerly GLAST) MET is to be interpreted.
      */
      AbsoluteTime computeAbsoluteTime(double glast_time, const std::string & time_system_name) const;

      /** \brief Compute a Fermi (formerly GLAST) Mission Elapsed Time (MET) corresponding to a given absolute time.
          \param abs_time AbsoluteTime object to be converted into a Fermi (formerly GLAST) MET.
      */
      double computeGlastTime(const AbsoluteTime & abs_time) const;

    private:
      const TimeSystem * m_time_system;
      Mjd m_mjd_ref;
  };

  /** \class GlastScTimeHandler
      \brief Class which reads out event times from a Fermi (formerly GLAST) event file that is not applied barycentric corrections,
             creates AbsoluteTime objects for event times, and performs geocentric and barycentric correction on
             event times that are typically recorded at a spacecraft.
  */
  class GlastScTimeHandler: public GlastTimeHandler {
    public:
      /// Destruct this GlastScTimeHandler object.
      virtual ~GlastScTimeHandler();

      /** \brief Create an instance of a GlastScTimeHandler subclass, and return the pointer to the instance.
                 If failed to open a given file, this method returns 0 (null pointer).
          \param file_name Name of FITS file to open.
          \param extension_name Name of FITS extension to open.
          \param read_only Set to true to open the file in a read-only mode. Set to false to open it in a read-write mode.
      */
      static EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name, bool read_only = true);

      /** \brief Initialize arrival time corrections.
          \param sc_file_name Name of spacecraft file to be used for arrival time corrections.
          \param sc_extension_name Name of FITS table that contains spacecraft data in the above file.
          \param solar_eph Name of solar system ephemeris to use for arrival time corrections.
          \param match_solar_eph Set to true if the above solar system ephemeris must match the one written in the opened file.
          \param angular_tolerance Maximum angular difference in degrees allowed in comparison of sky position between
                 the one given to setSourcePosition method and the one in the opened file.
      */
      virtual void initTimeCorrection(const std::string & sc_file_name, const std::string & sc_extension_name, 
        const std::string & solar_eph, bool match_solar_eph, double angular_tolerance);

      /** \brief Set the source position for arrival time corrections.
          \param ra Right Ascension of the source position.
          \param dec Declination of the source position.
      */
      virtual void setSourcePosition(double ra, double dec);

      /** \brief Read a given field of the opened FITS header or the current record of the opened FITS table,
                 compute an absolute time that it represents, compute a geocentric time for it, and return it.
          \param field_name Name of field from which a time is to be read.
          \param from_header Set to true to read a time from the header. Set to false to read a time from the current record
                 of the table.
      */
      virtual AbsoluteTime getGeoTime(const std::string & field_name, bool from_header = false) const;

      /** \brief Read a given field of the opened FITS header or the current record of the opened FITS table,
                 compute an absolute time that it represents, compute a barycentric time for it, and return it.
          \param field_name Name of field from which a time is to be read.
          \param from_header Set to true to read a time from the header. Set to false to read a time from the current record
                 of the table.
      */
      virtual AbsoluteTime getBaryTime(const std::string & field_name, bool from_header = false) const;

    private:
      std::string m_sc_file;
      char * m_sc_file_char;
      double m_ra_bary;  // RA and Dec for barycentering.
      double m_dec_bary;
      const BaryTimeComputer * m_computer;

      /** Construct a GlastScTimeHandler object.
          \param file_name Name of FITS file to open.
          \param extension_name Name of FITS extension to open.
          \param read_only Set to true to open the file in a read-only mode. Set to false to open it in a read-write mode.
      */
      GlastScTimeHandler(const std::string & file_name, const std::string & extension_name, bool read_only = true);

      /** \brief Helper method for getGeoTime and getBaryTime methods. This method performs the actual computations of
                 arrival time corrections, and returns an AbsoluteTime object that represents a corrected time.
          \param field_name Name of field from which a time is to be read.
          \param from_header Set to true to read a time from the header. Set to false to read a time from the current record
                 of the table.
          \param compute_bary Set to true to compute a barycentric time. Set to false to compute a geocentric time.
      */
      AbsoluteTime getCorrectedTime(const std::string & field_name, bool from_header, bool compute_bary) const;
  };

  /** \class GlastGeoTimeHandler
      \brief Class which reads out event times from a Fermi (formerly GLAST) event file that is applied geocentric corrections,
             creates AbsoluteTime objects for event times, and performs geocentric and barycentric correction on
             event times that are typically recorded at a spacecraft.
             typically recorded at a space craft.
  */
  class GlastGeoTimeHandler: public GlastTimeHandler {
    public:
      /// Destruct this GlastGeoTimeHandler object.
      virtual ~GlastGeoTimeHandler();

      /** \brief Create an instance of a GlastGeoTimeHandler subclass, and return the pointer to the instance.
                 If failed to open a given file, this method returns 0 (null pointer).
          \param file_name Name of FITS file to open.
          \param extension_name Name of FITS extension to open.
          \param read_only Set to true to open the file in a read-only mode. Set to false to open it in a read-write mode.
      */
      static EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name, bool read_only = true);

      /** \brief Initialize arrival time corrections.
          \param sc_file_name Name of spacecraft file to be used for arrival time corrections.
          \param sc_extension_name Name of FITS table that contains spacecraft data in the above file.
          \param solar_eph Name of solar system ephemeris to use for arrival time corrections.
          \param match_solar_eph Set to true if the above solar system ephemeris must match the one written in the opened file.
          \param angular_tolerance Maximum angular difference in degrees allowed in comparison of sky position between
                 the one given to setSourcePosition method and the one in the opened file.
      */
      virtual void initTimeCorrection(const std::string & sc_file_name, const std::string & sc_extension_name, 
        const std::string & solar_eph, bool match_solar_eph, double angular_tolerance);

      /** \brief Set the source position for arrival time corrections.
          \param ra Right Ascension of the source position.
          \param dec Declination of the source position.
      */
      virtual void setSourcePosition(double ra, double dec);

      /** \brief Read a given field of the opened FITS header or the current record of the opened FITS table,
                 compute an absolute time that it represents, compute a geocentric time for it, and return it.
          \param field_name Name of field from which a time is to be read.
          \param from_header Set to true to read a time from the header. Set to false to read a time from the current record
                 of the table.
      */
      virtual AbsoluteTime getGeoTime(const std::string & field_name, bool from_header = false) const;

      /** \brief Read a given field of the opened FITS header or the current record of the opened FITS table,
                 compute an absolute time that it represents, compute a barycentric time for it, and return it.
          \param field_name Name of field from which a time is to be read.
          \param from_header Set to true to read a time from the header. Set to false to read a time from the current record
                 of the table.
      */
      virtual AbsoluteTime getBaryTime(const std::string & field_name, bool from_header = false) const;

    private:
      std::string m_file_name;
      std::string m_ext_name;

      /** Construct a GlastGeoTimeHandler object.
          \param file_name Name of FITS file to open.
          \param extension_name Name of FITS extension to open.
          \param read_only Set to true to open the file in a read-only mode. Set to false to open it in a read-write mode.
      */
      GlastGeoTimeHandler(const std::string & file_name, const std::string & extension_name, bool read_only = true);
  };

  /** \class GlastBaryTimeHandler
      \brief Class which reads out event times from a Fermi (formerly GLAST) event file that is applied barycentric corrections,
             creates AbsoluteTime objects for event times, and performs geocentric and barycentric correction on
             event times that are typically recorded at a spacecraft.
  */
  class GlastBaryTimeHandler: public GlastTimeHandler {
    public:
      /// Destruct this GlastBaryTimeHandler object.
      virtual ~GlastBaryTimeHandler();

      /** \brief Create an instance of a GlastBaryTimeHandler subclass, and return the pointer to the instance.
                 If failed to open a given file, this method returns 0 (null pointer).
          \param file_name Name of FITS file to open.
          \param extension_name Name of FITS extension to open.
          \param read_only Set to true to open the file in a read-only mode. Set to false to open it in a read-write mode.
      */
      static EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name, bool read_only = true);

      /** \brief Initialize arrival time corrections.
          \param sc_file_name Name of spacecraft file to be used for arrival time corrections.
          \param sc_extension_name Name of FITS table that contains spacecraft data in the above file.
          \param solar_eph Name of solar system ephemeris to use for arrival time corrections.
          \param match_solar_eph Set to true if the above solar system ephemeris must match the one written in the opened file.
          \param angular_tolerance Maximum angular difference in degrees allowed in comparison of sky position between
                 the one given to setSourcePosition method and the one in the opened file.
      */
      virtual void initTimeCorrection(const std::string & sc_file_name, const std::string & sc_extension_name, 
        const std::string & solar_eph, bool match_solar_eph, double angular_tolerance);

      /** \brief Set the source position for arrival time corrections.
          \param ra Right Ascension of the source position.
          \param dec Declination of the source position.
      */
      virtual void setSourcePosition(double ra, double dec);

      /** \brief Read a given field of the opened FITS header or the current record of the opened FITS table,
                 compute an absolute time that it represents, compute a geocentric time for it, and return it.
          \param field_name Name of field from which a time is to be read.
          \param from_header Set to true to read a time from the header. Set to false to read a time from the current record
                 of the table.
      */
      virtual AbsoluteTime getGeoTime(const std::string & field_name, bool from_header = false) const;

      /** \brief Read a given field of the opened FITS header or the current record of the opened FITS table,
                 compute an absolute time that it represents, compute a barycentric time for it, and return it.
          \param field_name Name of field from which a time is to be read.
          \param from_header Set to true to read a time from the header. Set to false to read a time from the current record
                 of the table.
      */
      virtual AbsoluteTime getBaryTime(const std::string & field_name, bool from_header = false) const;

    private:
      std::string m_file_name;
      std::string m_ext_name;
      double m_ra_nom;  // RA and Dec from an event file header.
      double m_dec_nom;
      std::vector<double> m_vect_nom; // Three vector representation of m_ra_nom and m_dec_nom.
      double m_max_vect_diff;
      std::string m_pl_ephem;

      /** Construct a GlastBaryTimeHandler object.
          \param file_name Name of FITS file to open.
          \param extension_name Name of FITS extension to open.
          \param read_only Set to true to open the file in a read-only mode. Set to false to open it in a read-write mode.
      */
      GlastBaryTimeHandler(const std::string & file_name, const std::string & extension_name, bool read_only = true);

      /** \brief Convert a sky position from a pair of Right Ascension and Declination to a three vector.
          \param ra Right Ascension of a sky position to convert.
          \param dec Declination of a sky position to convert.
      */
      std::vector<double> computeThreeVector(double ra, double dec) const;
  };

}

#endif
