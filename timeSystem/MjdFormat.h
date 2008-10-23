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
    /// \brief Construct an Mjd object.
    Mjd(long int_part, double frac_part): m_int(int_part), m_frac(frac_part) {}
    long m_int;
    double m_frac;
  };

  /** \class Mjd1
      \brief Class to hold an MJD number in one double-precision variable.
  */
  struct Mjd1 {
    /// \brief Construct an Mjd1 object.
    explicit Mjd1(double day): m_day(day) {}
    double m_day;
  };

  /** \class Jd
      \brief Class to hold an Julian Day (JD) number in two parts, the integer part and the fractional part, in order to
             hold it in a precision required by GLAST.
  */
  struct Jd {
    /// \brief Construct a Jd object.
    Jd(long int_part, double frac_part): m_int(int_part), m_frac(frac_part) {}
    long m_int;
    double m_frac;
  };

  /** \class Jd1
      \brief Class to hold an JD number in one double-precision variable.
  */
  struct Jd1 {
    /// \brief Construct a Jd1 object.
    explicit Jd1(double day): m_day(day) {}
    double m_day;
  };

  /** \class TimeFormatFactory<Mjd>
      \brief Specialized TimeFormatFactory class for Mjd class.
  */
  template <>
  class TimeFormatFactory<Mjd> {
    public:
      /// \brief Return TimeFormat class for MJD representation.
      static const TimeFormat<Mjd> & getFormat();
  };

  /** \class TimeFormatFactory<Mjd1>
      \brief Specialized TimeFormatFactory class for Mjd1 class.
  */
  template <>
  class TimeFormatFactory<Mjd1> {
    public:
      /// \brief Return TimeFormat class for MJD representation.
      static const TimeFormat<Mjd1> & getFormat();
  };

  /** \class TimeFormatFactory<Jd>
      \brief Specialized TimeFormatFactory class for Jd class.
  */
  template <>
  class TimeFormatFactory<Jd> {
    public:
      /// \brief Return TimeFormat class for JD representation.
      static const TimeFormat<Jd> & getFormat();
  };

  /** \class TimeFormatFactory<Jd1>
      \brief Specialized TimeFormatFactory class for Jd1 class.
  */
  template <>
  class TimeFormatFactory<Jd1> {
    public:
      /// \brief Return TimeFormat class for JD representation.
      static const TimeFormat<Jd1> & getFormat();
  };

  static const TimeFormat<Mjd> & MjdFmt(TimeFormatFactory<Mjd>::getFormat());

  static const TimeFormat<Mjd1> & Mjd1Fmt(TimeFormatFactory<Mjd1>::getFormat());

  static const TimeFormat<Jd> & JdFmt(TimeFormatFactory<Jd>::getFormat());

  static const TimeFormat<Jd1> & Jd1Fmt(TimeFormatFactory<Jd1>::getFormat());

}

#endif
