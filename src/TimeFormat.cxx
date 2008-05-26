/** \file TimeFormat.cxx
    \brief Implementation of TimeFormat and related classes.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/TimeFormat.h"

#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace timeSystem {

  TimeFormat::TimeFormat(const std::string & format_name): m_format_name(format_name) {
    std::string format_name_uc(format_name);
    for (std::string::iterator itor = format_name_uc.begin(); itor != format_name_uc.end(); ++itor) *itor = std::toupper(*itor);
    getContainer()[format_name_uc] = this;
  }

  TimeFormat::~TimeFormat() {
  }

  const TimeFormat & TimeFormat::getFormat(const std::string & format_name) {
    MjdFormat::getMjdFormat();

    std::string format_name_uc(format_name);
    for (std::string::iterator itor = format_name_uc.begin(); itor != format_name_uc.end(); ++itor) *itor = std::toupper(*itor);
    container_type & container(getContainer());

    container_type::iterator cont_itor = container.find(format_name_uc);
    if (container.end() == cont_itor) throw std::runtime_error("TimeFormat::getFormat could not find time format " + format_name);
    return *cont_itor->second;
  }

  TimeFormat::container_type & TimeFormat::getContainer() {
    static container_type s_prototype;
    return s_prototype;
  }

  MjdFormat::MjdFormat(): TimeFormat("MJD") {}

  const MjdFormat & MjdFormat::getMjdFormat() {
    static MjdFormat s_format;
    return s_format;
  }

  std::string MjdFormat::format(const moment_type & value, std::streamsize precision) const {
    std::ostringstream os;
    long mjd_int = 0;
    double mjd_frac = 0.;
    convert(value, mjd_int, mjd_frac);
    IntFracPair int_frac(mjd_int, mjd_frac);
    os << std::setprecision(precision) << int_frac << " MJD";
    return os.str();
  }

  moment_type MjdFormat::parse(const std::string & value) const {
    IntFracPair int_frac(value);
    long mjd_int = int_frac.getIntegerPart();
    double mjd_frac = int_frac.getFractionalPart();
    moment_type moment;
    convert(mjd_int, mjd_frac, moment);
    return moment;
  }

  void MjdFormat::convert(const moment_type & moment, long & mjd_int, double & mjd_frac) const {
    if (SecPerDay() < moment.second) {
      // During an inserted leap-second.
      mjd_int = moment.first + 1;
      mjd_frac = 0.;
    } else {
      mjd_int = moment.first;
      mjd_frac = moment.second * DayPerSec();
    }
  }

  void MjdFormat::convert(const moment_type & moment, double & mjd) const {
    long int_part = 0;
    double frac_part = 0.;
    convert(moment, int_part, frac_part);
    mjd = int_part + frac_part;
  }

  void MjdFormat::convert(long mjd_int, double mjd_frac, moment_type & moment) const {
    // Split mjd_frac into integer part and fractional part.
    IntFracPair mjd_frac_split(mjd_frac);

    // Set the value to the moment_type object.
    moment.first = mjd_int + mjd_frac_split.getIntegerPart();
    moment.second = mjd_frac_split.getFractionalPart() * SecPerDay();
  }

  void MjdFormat::convert(double mjd, moment_type & moment) const {
    IntFracPair mjd_int_frac(mjd);
    convert(mjd_int_frac.getIntegerPart(), mjd_int_frac.getFractionalPart(), moment);
  }
}
