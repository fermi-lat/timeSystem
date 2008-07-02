/** \file MjdFormat.h
    \brief Declaration of MjdFormat and related classes.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_MjdFormat_h
#define timeSystem_MjdFormat_h

#include "timeSystem/TimeFormat.h"

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
    explicit Mjd1(double day): m_day(day) {}
    double m_day;
  };

  // TODO: Consider implementing Jd and Jd1, with the following note in mind.
  // NOTE: One can create Jd and Jd1 struct to represent a Julian Day just like Mjd and Mjd1 above, but leap-second handling
  //       will be more tricky. Because a leap second is usually inserted at the end of an MJD which is the middle of a Julian Day,
  //       it will be difficult to unambiguously represent a moment in time in the latter half of the day.

  template <>
  class TimeFormatFactory<Mjd> {
    public:
      static const TimeFormat<Mjd> & getFormat();
  };

  template <>
  class TimeFormatFactory<Mjd1> {
    public:
      static const TimeFormat<Mjd1> & getFormat();
  };

  static const TimeFormat<Mjd> & MjdFmt(TimeFormatFactory<Mjd>::getFormat());

  static const TimeFormat<Mjd1> & Mjd1Fmt(TimeFormatFactory<Mjd1>::getFormat());

}

#endif
