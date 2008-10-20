/** \file MjdFormat.cxx
    \brief Implementation of MjdFormat and related classes.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/MjdFormat.h"

#include <cctype>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {

  using namespace timeSystem;

  // Conversion constants between MJD and JD.
  const long JdMinusMjdInt() { static const long s_value = 2400000; return s_value; }
  const double JdMinusMjdFrac() { static const double s_value = .5; return s_value; }
  const double JdMinusMjdDouble() { static const double s_value = JdMinusMjdInt() + JdMinusMjdFrac(); return s_value; }

  /** \class IntFracUtility
      \brief Helper Class to check, parse, and format a pair of an integer part and a fractional part for time representation
             classess such as MJD format, holding an integer and a fractional part separately.
  */
  class IntFracUtility {
    public:
      static IntFracUtility & getUtility();

      void check(long int_part, double frac_part) const;

      void parse(const std::string & value_string, long & int_part, double & frac_part) const;

      std::string format(long int_part, double frac_part, std::streamsize precision = std::numeric_limits<double>::digits10) const;

      long convert(double value_double) const;

    private:
      IntFracUtility();
  };

  /** \class MjdFormat
      \brief Class to convert time representations in MJD format, holding an integer and a fractional part separately.
  */
  class MjdFormat : public TimeFormat<Mjd> {
    public:
      virtual Mjd convert(const datetime_type & datetime) const;

      virtual datetime_type convert(const Mjd & time_rep) const;

      virtual Mjd parse(const std::string & time_string) const;

      virtual std::string format(const Mjd & time_rep, std::streamsize precision = std::numeric_limits<double>::digits10) const;
  };

  /** \class Mjd1Format
      \brief Class to convert time representations in MJD format, held in one double-precision variable.
  */
  class Mjd1Format : public TimeFormat<Mjd1> {
    public:
      virtual Mjd1 convert(const datetime_type & datetime) const;

      virtual datetime_type convert(const Mjd1 & time_rep) const;

      virtual Mjd1 parse(const std::string & time_string) const;

      virtual std::string format(const Mjd1 & time_rep, std::streamsize precision = std::numeric_limits<double>::digits10) const;
  };

  /** \class JdFormat \brief Class to convert time representations in
      JD format, holding an integer and a fractional part separately.
  */
  class JdFormat : public TimeFormat<Jd> {
    public:
      virtual Jd convert(const datetime_type & datetime) const;

      virtual datetime_type convert(const Jd & time_rep) const;

      virtual Jd parse(const std::string & time_string) const;

      virtual std::string format(const Jd & time_rep, std::streamsize precision = std::numeric_limits<double>::digits10) const;
  };

  /** \class Jd1Format
      \brief Class to convert time representations in JD format, held in one double-precision variable.
  */
  class Jd1Format : public TimeFormat<Jd1> {
    public:
      virtual Jd1 convert(const datetime_type & datetime) const;

      virtual datetime_type convert(const Jd1 & time_rep) const;

      virtual Jd1 parse(const std::string & time_string) const;

      virtual std::string format(const Jd1 & time_rep, std::streamsize precision = std::numeric_limits<double>::digits10) const;
  };

  IntFracUtility::IntFracUtility() {}

  IntFracUtility & IntFracUtility::getUtility() {
    static IntFracUtility s_utility;
    return s_utility;
  }

  void IntFracUtility::check(long int_part, double frac_part) const {
    if ((int_part == 0 && (frac_part <= -1. || frac_part >= +1.)) ||
        (int_part >  0 && (frac_part <   0. || frac_part >= +1.)) ||
        (int_part <  0 && (frac_part <= -1. || frac_part >   0.))) {
      std::ostringstream os;
      os.precision(std::numeric_limits<double>::digits10);
      os << "Fractional part out of bounds: " << frac_part;
      throw std::runtime_error(os.str());
    }
  }

  void IntFracUtility::parse(const std::string & value_string, long & int_part, double & frac_part) const {
    std::string value;
    // Read number into temporary double variable.
    double value_dbl = 0.;
    {
      // Remove trailing space to prevent spurious errors.
      std::string::size_type trail = value_string.find_last_not_of(" \t\v\n");
      if (std::string::npos != trail) value = value_string.substr(0, trail + 1);
      std::istringstream iss(value);
      iss >> value_dbl;
      if (iss.fail() || !iss.eof())
        throw std::runtime_error("Error in converting \"" + value_string + "\" into a floating-point number");
    }

    // Compute integer part.
    int_part = convert(value_dbl);

    // Compute number of digits of integer part.
    int num_digit = (int_part == 0 ? 0 : int(std::floor(std::log10(std::fabs(double(int_part)))) + 0.5) + 1);

    // Skip leading zeros, whitespace, and non-digits.
    std::string::iterator itor = value.begin();
    for (; itor != value.end() && ('0' == *itor || 0 == std::isdigit(*itor)); ++itor) {}

    // Erase numbers in integer part.
    for (int ii_digit = 0; itor != value.end() && ii_digit < num_digit; ++itor) {
      if (0 != std::isdigit(*itor)) {
        *itor = '0';
        ++ii_digit;
      }
    }

    // Read in fractional part.
    {
      std::istringstream iss(value);
      iss >> frac_part;
    }
  }

  std::string IntFracUtility::format(long int_part, double frac_part, std::streamsize precision) const {
    // Check the fractional part.
    const IntFracUtility & utility(IntFracUtility::getUtility());
    utility.check(int_part, frac_part);

    // Prepare a stream to store a string.
    std::ostringstream os;
    os.precision(precision);
    os.setf(std::ios::fixed);

    if (int_part == 0) {
      // Write fractional part only.
      os << frac_part;
    } else {
      // Write integer part first.
      os << int_part;

      // Write fractional part into a temporary string.
      std::ostringstream oss;
      oss.precision(precision);
      oss.setf(std::ios::fixed);
      oss << frac_part;
      std::string frac_part_string = oss.str();

      // Truncate trailing 0s.
      std::string::size_type pos = frac_part_string.find_last_not_of("0 \t\v\n");
      if (std::string::npos != pos) frac_part_string.erase(pos+1);

      // Remove a decimal point ('.') at the end.
      pos = frac_part_string.size() - 1;
      if ('.' == frac_part_string[pos]) frac_part_string.erase(pos);

      // Skip until a decimal point ('.') is found, then output the rest.
      std::string::iterator itor = frac_part_string.begin();
      for (; (itor != frac_part_string.end()) && (*itor != '.'); ++itor);
      for (; itor != frac_part_string.end(); ++itor) { os << *itor; }
    }

    // Return the formatted string.
    return os.str();
  }

  long IntFracUtility::convert(double value_double) const {
    // Check whether the given value can be converted into a long value.
    if (value_double >= std::numeric_limits<long>::max() + 1.) {
      std::ostringstream os;
      os.precision(std::numeric_limits<double>::digits10);
      os << "Integer part too large: overflow while converting " << value_double << " to a long";
      throw std::runtime_error(os.str());
    } else if (value_double <= std::numeric_limits<long>::min() - 1.) {
      std::ostringstream os;
      os.precision(std::numeric_limits<double>::digits10);
      os << "Integer part too small: underflow while converting " << value_double << " to a long";
      throw std::runtime_error(os.str());
    }

    // Return a converted value.
    return long(value_double);
  }

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
    double int_part_dbl = 0.;
    double frac_part = std::modf(time_rep.m_day, &int_part_dbl);

    // Round integer part of the value.
    const IntFracUtility & utility(IntFracUtility::getUtility());
    int_part_dbl += (int_part_dbl > 0. ? 0.5 : -0.5);
    long int_part = utility.convert(int_part_dbl);

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
