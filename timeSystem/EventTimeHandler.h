/** \file EventTimeHandler.h
    \brief Declaration of EventTimeHandler class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_EventTimeHandler_h
#define timeSystem_EventTimeHandler_h

#include "tip/Table.h"

#include <string>

namespace tip {
  class Header;
}

namespace timeSystem {

  class AbsoluteTime;
  class BaryTimeComputer;
  class TimeRep;

  /** \class EventTimeHandler
      \brief Class which reads out event times from an event file, creates AbsoluteTime objects for event times,
             and performs barycentric correction on event times, typically recorded at a space craft.
  */
  class EventTimeHandler {
    public:
      EventTimeHandler(tip::Table & table, double position_tolerance = 0.);

      virtual ~EventTimeHandler();

      // TODO: Consider adding the following methods to replace TimeRep objects for MET's.
      //virtual AbsoluteTime readString(const std::string & time_string, const std::string & time_system = "FILE") = 0;
      //virtual TimeRep * createTimeRep(const std::string & time_system = "FILE") const = 0;

      AbsoluteTime readHeader(const std::string & keyword_name);

      AbsoluteTime readHeader(const std::string & keyword_name, const double ra, const double dec);

      AbsoluteTime readColumn(const std::string & column_name);

      AbsoluteTime readColumn(const std::string & column_name, const double ra, const double dec);

      void setFirstRecord();

      void setNextRecord();

      bool isEndOfTable() const;

    protected:
      virtual AbsoluteTime readTime(const tip::Header & header, const std::string & keyword_name, const bool request_bary_time,
        const double ra, const double dec) = 0;

      virtual AbsoluteTime readTime(const tip::TableRecord & record, const std::string & column_name, const bool request_bary_time,
        const double ra, const double dec) = 0;

      tip::Header & getHeader() const;

      tip::TableRecord & getCurrentRecord() const;

      void computeBaryTime(const double ra, const double dec, const double sc_position[], AbsoluteTime & abs_time) const;

      // TODO: Should those (computeInnerProduct/OuterProduct/ThreeVector) be replaced by the ones in BaryTimeComputer?
      double computeInnerProduct(const double vect_x[], const double vect_y[]) const;

      void computeOuterProduct(const double vect_x[], const double vect_y[], double vect_z[]) const;

      void computeThreeVector(const double ra, const double dec, double vect[]) const;

      void checkSkyPosition(const double ra, const double dec) const;

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
      BaryTimeComputer & m_computer;
      // TODO: Should BaryTimeComputer be passed as a constructor argument?
  };

}

#endif
