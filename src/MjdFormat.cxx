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
  void TimeFormat::convert(const datetime_type & datetime, Mjd & mjd) {
    if (SecPerDay() < datetime.second) {
      // During an inserted leap-second.
      mjd.m_int = datetime.first + 1;
      mjd.m_frac = 0.;
    } else {
      mjd.m_int = datetime.first;
      mjd.m_frac = datetime.second / SecPerDay();
    }
  }

  template <>
  void TimeFormat::convert(const datetime_type & datetime, Mjd1 & mjd1) {
    Mjd mjd(0, 0.);
    convert(datetime, mjd);
    mjd1.m_day = mjd.m_int + mjd.m_frac;
  }

  template <>
  void TimeFormat::convert(const Mjd & mjd, datetime_type & datetime) {
    // Split mjd_frac into integer part and fractional part.
    IntFracPair mjd_frac_split(mjd.m_frac);

    // Set the value to the datetime_type object.
    datetime.first = mjd.m_int + mjd_frac_split.getIntegerPart();
    datetime.second = mjd_frac_split.getFractionalPart() * SecPerDay();
  }

  template <>
  void TimeFormat::convert(const Mjd1 & mjd1, datetime_type & datetime) {
    IntFracPair mjd_int_frac(mjd1.m_day);
    Mjd mjd(mjd_int_frac.getIntegerPart(), mjd_int_frac.getFractionalPart());
    convert(mjd, datetime);
  }

  std::string MjdFormat::format(const datetime_type & value, std::streamsize precision) const {
    Mjd mjd(0, 0.);
    convert(value, mjd);
    IntFracPair int_frac(mjd.m_int, mjd.m_frac);

    std::ostringstream os;
    os << std::setprecision(precision) << int_frac << " MJD";
    return os.str();
  }

  datetime_type MjdFormat::parse(const std::string & value) const {
    IntFracPair int_frac(value);
    Mjd mjd(int_frac.getIntegerPart(), int_frac.getFractionalPart());
    datetime_type moment;
    convert(mjd, moment);
    return moment;
  }
}
