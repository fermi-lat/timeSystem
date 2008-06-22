/** \file MjdFormat.h
    \brief Declaration of MjdFormat and related classes.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_MjdFormat_h
#define timeSystem_MjdFormat_h

#include "timeSystem/TimeFormat.h"
#include "timeSystem/TimeSystem.h"

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

  // TODO: Implement struct Jd, Jd1.

  template <>
  void TimeFormat::convert(const datetime_type & datetime, Mjd & mjd_rep);

  template <>
  void TimeFormat::convert(const datetime_type & datetime, Mjd1 & mjd1_rep);

  template <>
  void TimeFormat::convert(const Mjd & mjd_rep, datetime_type & datetime);

  template <>
  void TimeFormat::convert(const Mjd1 & mjd1_rep, datetime_type & datetime);

}

#endif
