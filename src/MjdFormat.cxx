/** \file MjdFormat.cxx
    \brief Implementation of MjdFormat and related classes.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/MjdFormat.h"

#include "timeSystem/IntFracUtility.h"

#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {

  using namespace timeSystem;

  /// \brief Conversion constants between MJD and JD.
  const long JdMinusMjdInt() { static const long s_value = 2400000; return s_value; }
  const double JdMinusMjdFrac() { static const double s_value = .5; return s_value; }
  const double JdMinusMjdDouble() { static const double s_value = JdMinusMjdInt() + JdMinusMjdFrac(); return s_value; }

  /** \class MjdFormat
      \brief Class to convert time representations in MJD format, holding an integer and a fractional part separately.
  */
  class MjdFormat : public TimeFormat<Mjd> {
    public:
      /** \brief Convert date and time into an MJD number, and return it.
          \param datetime Date and time to convert.
      */
      virtual Mjd convert(const datetime_type & datetime) const;

      /** \brief Convert an MJD number into date and time, and return it.
          \param time_rep MJD number to convert.
      */
      virtual datetime_type convert(const Mjd & time_rep) const;

      /** \brief Interpret a given character string as an MJD number, and return the MJD number.
          \param time_string Character string to interpret as an MJD number.
      */
      virtual Mjd parse(const std::string & time_string) const;

      /** \brief Create a character string that represents a given MJD number, and return it.
          \param time_rep MJD number to format into a character string.
          \param precision Number of digits after a decimal point in a resultant character string.
      */
      virtual std::string format(const Mjd & time_rep, std::streamsize precision = std::numeric_limits<double>::digits10) const;
  };

  /** \class Mjd1Format
      \brief Class to convert time representations in MJD format, held in one double-precision variable.
  */
  class Mjd1Format : public TimeFormat<Mjd1> {
    public:
      /** \brief Convert date and time into an MJD number, and return it.
          \param datetime Date and time to convert.
      */
      virtual Mjd1 convert(const datetime_type & datetime) const;

      /** \brief Convert an MJD number into date and time, and return it.
          \param time_rep MJD number to convert.
      */
      virtual datetime_type convert(const Mjd1 & time_rep) const;

      /** \brief Interpret a given character string as an MJD number, and return the MJD number.
          \param time_string Character string to interpret as an MJD number.
      */
      virtual Mjd1 parse(const std::string & time_string) const;

      /** \brief Create a character string that represents a given MJD number, and return it.
          \param time_rep MJD number to format into a character string.
          \param precision Number of digits after a decimal point in a resultant character string.
      */
      virtual std::string format(const Mjd1 & time_rep, std::streamsize precision = std::numeric_limits<double>::digits10) const;
  };

  /** \class JdFormat \brief Class to convert time representations in
      JD format, holding an integer and a fractional part separately.
  */
  class JdFormat : public TimeFormat<Jd> {
    public:
      /** \brief Convert date and time into a JD number, and return it.
          \param datetime Date and time to convert.
      */
      virtual Jd convert(const datetime_type & datetime) const;

      /** \brief Convert a JD number into date and time, and return it.
          \param time_rep JD number to convert.
      */
      virtual datetime_type convert(const Jd & time_rep) const;

      /** \brief Interpret a given character string as a JD number, and return the JD number.
          \param time_string Character string to interpret as a JD number.
      */
      virtual Jd parse(const std::string & time_string) const;

      /** \brief Create a character string that represents a given JD number, and return it.
          \param time_rep JD number to format into a character string.
          \param precision Number of digits after a decimal point in a resultant character string.
      */
      virtual std::string format(const Jd & time_rep, std::streamsize precision = std::numeric_limits<double>::digits10) const;
  };

  /** \class Jd1Format
      \brief Class to convert time representations in JD format, held in one double-precision variable.
  */
  class Jd1Format : public TimeFormat<Jd1> {
    public:
      /** \brief Convert date and time into a JD number, and return it.
          \param datetime Date and time to convert.
      */
      virtual Jd1 convert(const datetime_type & datetime) const;

      /** \brief Convert a JD number into date and time, and return it.
          \param time_rep JD number to convert.
      */
      virtual datetime_type convert(const Jd1 & time_rep) const;

      /** \brief Interpret a given character string as a JD number, and return the JD number.
          \param time_string Character string to interpret as a JD number.
      */
      virtual Jd1 parse(const std::string & time_string) const;

      /** \brief Create a character string that represents a given JD number, and return it.
          \param time_rep JD number to format into a character string.
          \param precision Number of digits after a decimal point in a resultant character string.
      */
      virtual std::string format(const Jd1 & time_rep, std::streamsize precision = std::numeric_limits<double>::digits10) const;
  };

  Mjd MjdFormat::convert(const datetime_type & datetime) const {
    // Check whether the second part is in bounds.
    if (datetime.second < 0. || datetime.second >= SecPerDay()) {
      // During an inserted leap-second.
      std::ostringstream os;
      os << "Unable to compute an MJD number for the given time: " << datetime.second << " seconds of " << datetime.first << " MJD";
      throw std::runtime_error(os.str());
    }

    // Return an Mjd object.
    return Mjd(datetime.first, datetime.second / SecPerDay());
  }

  datetime_type MjdFormat::convert(const Mjd & time_rep) const {
    // Check the fractional part.
    const IntFracUtility & utility(IntFracUtility::getUtility());
    utility.check(time_rep.m_int, time_rep.m_frac);

    // Return the date and time.
    return datetime_type(time_rep.m_int, time_rep.m_frac * SecPerDay());
  }

  Mjd MjdFormat::parse(const std::string & time_string) const {
    Mjd mjd_rep(0, 0);

    // Convert the string into a pair of an integer and a fractional parts.
    const IntFracUtility & utility(IntFracUtility::getUtility());
    utility.parse(time_string, mjd_rep.m_int, mjd_rep.m_frac);

    // Return the result.
    return mjd_rep;
  }

  std::string MjdFormat::format(const Mjd & time_rep, std::streamsize precision) const {
    // Convert the pair of an integer and a fractional parts of MJD into a string, and return it.
    const IntFracUtility & utility(IntFracUtility::getUtility());
    return utility.format(time_rep.m_int, time_rep.m_frac, precision) + " MJD";
  }

  Mjd1 Mjd1Format::convert(const datetime_type & datetime) const {
    const TimeFormat<Mjd> & mjd_format(TimeFormatFactory<Mjd>::getFormat());
    Mjd mjd_rep = mjd_format.convert(datetime);
    return Mjd1(mjd_rep.m_int + mjd_rep.m_frac);
  }

  datetime_type Mjd1Format::convert(const Mjd1 & time_rep) const {
    // Split MJD value into integer part and fractional part.
    const IntFracUtility & utility(IntFracUtility::getUtility());
    long int_part = 0;
    double frac_part = 0.;
    utility.split(time_rep.m_day, int_part, frac_part);

    // Return the date and time.
    return datetime_type(int_part, frac_part * SecPerDay());
  }

  Mjd1 Mjd1Format::parse(const std::string & time_string) const {
    std::istringstream iss(time_string);
    Mjd1 mjd1_rep(0.);
    iss >> mjd1_rep.m_day;
    if (iss.fail() || !iss.eof()) throw std::runtime_error("Error parsing \"" + time_string + "\"");

    return mjd1_rep;
  }

  std::string Mjd1Format::format(const Mjd1 & time_rep, std::streamsize precision) const {
    std::ostringstream os;
    os.setf(std::ios::fixed);
    os << std::setprecision(precision) << time_rep.m_day << " MJD";
    return os.str();
  }

  Jd JdFormat::convert(const datetime_type & datetime) const {
    // Convert the given time into MJD first.
    const TimeFormat<Mjd> & mjd_format(TimeFormatFactory<Mjd>::getFormat());
    Mjd mjd_rep = mjd_format.convert(datetime);

    // Convert the MJD number to JD number, and return it.
    Jd jd_rep(mjd_rep.m_int + JdMinusMjdInt(), mjd_rep.m_frac + JdMinusMjdFrac());
    if (jd_rep.m_frac >= 1.) {
      ++jd_rep.m_int;
      --jd_rep.m_frac;
    }
    return jd_rep;
  }

  datetime_type JdFormat::convert(const Jd & time_rep) const {
    // Check the fractional part.
    const IntFracUtility & utility(IntFracUtility::getUtility());
    utility.check(time_rep.m_int, time_rep.m_frac);

    // Compute MJD number.
    long mjd_int = time_rep.m_int - JdMinusMjdInt();
    double mjd_frac = time_rep.m_frac - JdMinusMjdFrac();
    if (mjd_frac < 0.) {
      --mjd_int;
      ++mjd_frac;
    }

    // Return the date and time.
    return datetime_type(mjd_int, mjd_frac * SecPerDay());
  }

  Jd JdFormat::parse(const std::string & time_string) const {
    Jd jd_rep(0, 0);

    // Convert the string into a pair of an integer and a fractional parts.
    const IntFracUtility & utility(IntFracUtility::getUtility());
    utility.parse(time_string, jd_rep.m_int, jd_rep.m_frac);

    // Return the result.
    return jd_rep;
  }

  std::string JdFormat::format(const Jd & time_rep, std::streamsize precision) const {
    // Convert the pair of an integer and a fractional parts of JD into a string, and return it.
    const IntFracUtility & utility(IntFracUtility::getUtility());
    return utility.format(time_rep.m_int, time_rep.m_frac, precision) + " JD";
  }

  Jd1 Jd1Format::convert(const datetime_type & datetime) const {
    const TimeFormat<Jd> & jd_format(TimeFormatFactory<Jd>::getFormat());
    Jd jd_rep = jd_format.convert(datetime);
    return Jd1(jd_rep.m_int + jd_rep.m_frac);
  }

  datetime_type Jd1Format::convert(const Jd1 & time_rep) const {
    Mjd1 mjd1_rep(time_rep.m_day - JdMinusMjdDouble());
    const TimeFormat<Mjd1> & mjd1_format(TimeFormatFactory<Mjd1>::getFormat());
    return mjd1_format.convert(mjd1_rep);
  }

  Jd1 Jd1Format::parse(const std::string & time_string) const {
    std::istringstream iss(time_string);
    Jd1 jd1_rep(0.);
    iss >> jd1_rep.m_day;
    if (iss.fail() || !iss.eof()) throw std::runtime_error("Error parsing \"" + time_string + "\"");

    return jd1_rep;
  }

  std::string Jd1Format::format(const Jd1 & time_rep, std::streamsize precision) const {
    std::ostringstream os;
    os.setf(std::ios::fixed);
    os << std::setprecision(precision) << time_rep.m_day << " JD";
    return os.str();
  }

}

namespace timeSystem {

  const TimeFormat<Mjd> & TimeFormatFactory<Mjd>::getFormat() {
    static const MjdFormat s_mjd_format;
    return s_mjd_format;
  }

  const TimeFormat<Mjd1> & TimeFormatFactory<Mjd1>::getFormat() {
    static const Mjd1Format s_mjd1_format;
    return s_mjd1_format;
  }

  const TimeFormat<Jd> & TimeFormatFactory<Jd>::getFormat() {
    static const JdFormat s_jd_format;
    return s_jd_format;
  }

  const TimeFormat<Jd1> & TimeFormatFactory<Jd1>::getFormat() {
    static const Jd1Format s_jd1_format;
    return s_jd1_format;
  }

}
