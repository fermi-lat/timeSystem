/** \file EventTimeHandler.h
    \brief Declaration of EventTimeHandler class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_EventTimeHandler_h
#define timeSystem_EventTimeHandler_h

#include "timeSystem/AbsoluteTime.h"

#include "tip/Table.h"

#include <string>

namespace timeSystem {

  /** \class EventTimeHandler
      \brief Class which reads out event times from an event file, creates AbsoluteTime objects for event times,
             and performs barycentric correction on event times, typically recorded at a space craft.
  */
  class EventTimeHandler {
    public:
      EventTimeHandler(tip::Table & table, double position_tolerance = 0.);

      ~EventTimeHandler();

      void initialize(const std::string & pl_ephem, const std::string & sc_file);

      AbsoluteTime readHeader(const std::string & keyword_name);

      AbsoluteTime readHeader(const std::string & keyword_name, const double ra, const double dec);

      AbsoluteTime readColumn(const std::string & column_name);

      AbsoluteTime readColumn(const std::string & column_name, const double ra, const double dec);

      // TODO: Implement this.
      AbsoluteTime readString(const std::string & time_string);

      void setFirstRecord();

      void setNextRecord();

      bool isEndOfTable() const;

      // TODO: Check whether this method is really needed.
      TimeRep * createTimeRep(std::string time_system = "FILE") const;

    private:
      // Variables for event table handling.
      tip::Table * m_table;
      tip::Table::Iterator m_record_itor;
      bool m_bary_time;
      double m_position_tolerance; // In degrees.
      double m_ra_nom;
      double m_dec_nom;
      double m_vect_nom[3]; // Three vector representation of m_ra_nom and m_dec_nom.

      // Variables for barycentering.
      double m_speed_of_light;
      double m_solar_mass;
      std::string m_sc_file;
      char * m_sc_file_char;
      TimeRep * m_mjd_tt;

      // TODO: Consider replacing TimeRep with this class.
      TimeRep * m_time_rep;

      AbsoluteTime readHeader(const std::string & keyword_name, const bool request_bary_time, const double ra, const double dec);

      AbsoluteTime readColumn(const std::string & column_name, const bool request_bary_time, const double ra, const double dec);

      void computeBaryTime(const double ra, const double dec, const double sc_position[], AbsoluteTime & abs_time) const;

      double computeInnerProduct(const double vect_x[], const double vect_y[]) const;

      void computeOuterProduct(const double vect_x[], const double vect_y[], double vect_z[]) const;

      void computeThreeVector(const double ra, const double dec, double vect[]) const;

      void checkSkyPosition(const double ra, const double dec) const;
  };

}

#endif
