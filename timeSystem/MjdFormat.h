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
      \brief Class to hold an Modified Julian Day (MJD) number in two parts, the integer part and the fractional part,
             in order to hold it in a precision required by GLAST.
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

  /** \class Jd
      \brief Class to hold an Julian Day (JD) number in two parts, the integer part and the fractional part, in order to
             hold it in a precision required by GLAST.
  */
  struct Jd {
    Jd(long int_part, double frac_part): m_int(int_part), m_frac(frac_part) {}
    long m_int;
    double m_frac;
  };

  /** \class Jd1
      \brief Class to hold an JD number in one double-precision variable.
  */
  struct Jd1 {
    explicit Jd1(double day): m_day(day) {}
    double m_day;
  };

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

  template <>
  class TimeFormatFactory<Jd> {
    public:
      static const TimeFormat<Jd> & getFormat();
  };

  template <>
  class TimeFormatFactory<Jd1> {
    public:
      static const TimeFormat<Jd1> & getFormat();
  };

  static const TimeFormat<Mjd> & MjdFmt(TimeFormatFactory<Mjd>::getFormat());

  static const TimeFormat<Mjd1> & Mjd1Fmt(TimeFormatFactory<Mjd1>::getFormat());

  static const TimeFormat<Jd> & JdFmt(TimeFormatFactory<Jd>::getFormat());

  static const TimeFormat<Jd1> & Jd1Fmt(TimeFormatFactory<Jd1>::getFormat());

}

#endif
