/** \file MjdFormat.h
    \brief Declaration of MjdFormat and related classes.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_MjdFormat_h
#define timeSystem_MjdFormat_h

#include "timeSystem/IntFracPair.h"
#include "timeSystem/TimeFormat.h"
#include "timeSystem/TimeSystem.h"

#include <limits>
#include <string>

namespace timeSystem {

  /** \class Mjd
      \brief Class to hold an MJD number in two parts, the integer part and the fractional part, in order to
             hold it in a precision required by GLAST.
  */
  struct Mjd {
    Mjd(long int_part, double frac_part): m_int(int_part), m_frac(frac_part) {}
    long m_int;
    double m_frac;
  };

  /** \class Mjd1
      \brief Class to hold an MJD number in one double-precision variable.
  */
  struct Mjd1 {
    Mjd1(double day): m_day(day) {}
    double m_day;
  };

  // TODO: Implement struct Jd, Jd1, MonthDay, WeekDay, YearDay.
  // NOTE: Data members of struct MonthDay: m_year, m_mon,  m_day, m_hour, m_min, and m_sec (cf. struct tm in C).
  // NOTE: Data members of struct WeekDay:  m_year, m_week, m_day, m_hour, m_min, and m_sec (cf. struct tm in C).
  // NOTE: Data members of struct YearDay:  m_year,         m_day, m_hour, m_min, and m_sec (cf. struct tm in C).

  /** \class MjdFormat
      \brief Class to represent MJD format of time representation.
  */
  class MjdFormat : public TimeFormat {
    public:
      MjdFormat(): TimeFormat("MJD") {}

      virtual std::string format(const datetime_type & value, std::streamsize precision = std::numeric_limits<double>::digits10) const;

      virtual datetime_type parse(const std::string & value) const;
  };

  static const MjdFormat mjd_format;
}

#endif
