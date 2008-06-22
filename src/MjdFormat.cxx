/** \file MjdFormat.cxx
    \brief Implementation of MjdFormat and related classes.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/MjdFormat.h"

#include "timeSystem/IntFracPair.h"

#include <iomanip>
#include <sstream>

namespace timeSystem {

  template <>
  void TimeFormat::convert(const datetime_type & datetime, Mjd & mjd_rep) {
    if (SecPerDay() < datetime.second) {
      // During an inserted leap-second.
      mjd_rep.m_int = datetime.first + 1;
      mjd_rep.m_frac = 0.;
    } else {
      mjd_rep.m_int = datetime.first;
      mjd_rep.m_frac = datetime.second / SecPerDay();
    }
  }

  template <>
  void TimeFormat::convert(const datetime_type & datetime, Mjd1 & mjd1_rep) {
    Mjd mjd_rep(0, 0.);
    convert(datetime, mjd_rep);
    mjd1_rep.m_day = mjd_rep.m_int + mjd_rep.m_frac;
  }

  template <>
  void TimeFormat::convert(const Mjd & mjd_rep, datetime_type & datetime) {
    // Split mjd_frac into integer part and fractional part.
    IntFracPair mjd_frac_split(mjd_rep.m_frac);

    // Set the value to the datetime_type object.
    datetime.first = mjd_rep.m_int + mjd_frac_split.getIntegerPart();
    datetime.second = mjd_frac_split.getFractionalPart() * SecPerDay();
  }

  template <>
  void TimeFormat::convert(const Mjd1 & mjd1_rep, datetime_type & datetime) {
    IntFracPair mjd_int_frac(mjd1_rep.m_day);
    Mjd mjd_rep(mjd_int_frac.getIntegerPart(), mjd_int_frac.getFractionalPart());
    convert(mjd_rep, datetime);
  }

}

namespace {

  using namespace timeSystem;

  /** \class MjdFormat
      \brief Class to represent MJD format of time representation.
  */
  class MjdFormat : public TimeFormat {
    public:
      MjdFormat(): TimeFormat("MJD") {}

      virtual std::string format(const datetime_type & value, std::streamsize precision = std::numeric_limits<double>::digits10) const;

      virtual datetime_type parse(const std::string & value) const;
  };

  static const MjdFormat s_mjd_format;

  std::string MjdFormat::format(const datetime_type & value, std::streamsize precision) const {
    Mjd mjd_rep(0, 0.);
    convert(value, mjd_rep);
    IntFracPair int_frac(mjd_rep.m_int, mjd_rep.m_frac);

    std::ostringstream os;
    os << std::setprecision(precision) << int_frac << " MJD";
    return os.str();
  }

  datetime_type MjdFormat::parse(const std::string & value) const {
    IntFracPair int_frac(value);
    Mjd mjd_rep(int_frac.getIntegerPart(), int_frac.getFractionalPart());
    datetime_type datetime(0, 0.);
    convert(mjd_rep, datetime);
    return datetime;
  }

}
