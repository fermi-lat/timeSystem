/** \file Duration.cxx
    \brief Implementation of Duration class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include <cctype>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>

#include "timeSystem/Duration.h"
#include "timeSystem/IntFracPair.h"
#include "timeSystem/TimeConstant.h"

#include "st_stream/Stream.h"

namespace {

  using namespace timeSystem;

  /** \class TimeUnit Base class representing different time units, whose subclasses represent
      individual concrete time unit.

      Note: The four units defined below (day, hour, minute, and second) are "safe to use" precision-wise.
      Smaller units (millisecond, microsecond, ...) are not really a distinct unit from seconds.
      Larger units (Week, Year, Decade, Century, ...) would compromise the precision realized by this approach.
      The reason is that internally, Duration stores times as a whole number of days + a fractional number
      of seconds. The act of converting, say .5 years (~ 15768000 seconds) to days plus seconds would
      yield an intermediate result accurate only to 100 ns, whereas 182.5 days (~ .5 year) would be stored
      accurate to within 100 picoseconds.
  */
  class TimeUnit {
    public:
      static const TimeUnit & getUnit(const std::string & time_unit_name);

      long getUnitPerDay() const { return m_unit_per_day; }

      double getUnitPerSec() const { return m_unit_per_sec; }

      long getSecPerUnit() const { return m_sec_per_unit; }

      std::string getUnitString() const { return m_unit_string; }

    protected:
      TimeUnit(long unit_per_day, double unit_per_sec, long sec_per_unit, const std::string & unit_string):
        m_unit_per_day(unit_per_day), m_unit_per_sec(unit_per_sec), m_sec_per_unit(sec_per_unit), m_unit_string(unit_string) {}

    protected:
      typedef std::map<std::string, const TimeUnit *> container_type;

      static container_type & getContainer();

    private:
      long m_unit_per_day;
      double m_unit_per_sec;
      long m_sec_per_unit;
      std::string m_unit_string;
  };

  class TimeUnitDay: public TimeUnit {
    public:
      TimeUnitDay();
  };

  class TimeUnitHour: public TimeUnit {
    public:
      TimeUnitHour();
  };

  class TimeUnitMin: public TimeUnit {
    public:
      TimeUnitMin();
  };

  class TimeUnitSec: public TimeUnit {
    public:
      TimeUnitSec();
  };

  TimeUnitDay::TimeUnitDay(): TimeUnit(1, DayPerSec(), SecPerDayLong(), "days") {
    container_type & container(getContainer());
    container["DAY"] = this;
    container["DAYS"] = this;
  }

  TimeUnitHour::TimeUnitHour(): TimeUnit(HourPerDayLong(), HourPerSec(), SecPerHourLong(), "hours") {
    container_type & container(getContainer());
    container["HOUR"] = this;
    container["HOURS"] = this;
  }

  TimeUnitMin::TimeUnitMin(): TimeUnit(MinPerDayLong(), MinPerSec(), SecPerMinLong(), "minutes") {
    container_type & container(getContainer());
    container["MIN"] = this;
    container["MINUTE"] = this;
    container["MINUTES"] = this;
  }

  TimeUnitSec::TimeUnitSec(): TimeUnit(SecPerDayLong(), 1.0, 1, "seconds") {
    container_type & container(getContainer());
    container["SEC"] = this;
    container["SECOND"] = this;
    container["SECONDS"] = this;
  }

  const TimeUnit & TimeUnit::getUnit(const std::string & time_unit_name) {
    // Create TimeUnit objects.
    static const TimeUnitDay s_day;
    static const TimeUnitHour s_hour;
    static const TimeUnitMin s_min;
    static const TimeUnitSec s_sec;

    // Make the unit name case-insensitive.
    std::string time_unit_name_uc(time_unit_name);
    for (std::string::iterator itor = time_unit_name_uc.begin(); itor != time_unit_name_uc.end(); ++itor) *itor = std::toupper(*itor);

    // Find a requested TimeUnit object and return it.
    TimeUnit::container_type & container(getContainer());
    container_type::iterator cont_itor = container.find(time_unit_name_uc);
    if (container.end() == cont_itor) throw std::runtime_error("TimeUnit::getUnit could not find time unit " + time_unit_name);
    return *cont_itor->second;
  }

  TimeUnit::container_type & TimeUnit::getContainer() {
    static container_type s_prototype;
    return s_prototype;
  }
}

namespace timeSystem {

  Duration::Duration(long time_value_int, double time_value_frac, const std::string & time_unit_name) {
    set(time_value_int, time_value_frac, time_unit_name);
  }

  Duration::Duration(double time_value, const std::string & time_unit_name) {
    IntFracPair int_frac(time_value);
    set(int_frac.getIntegerPart(), int_frac.getFractionalPart(), time_unit_name);
  }

  Duration::Duration(const std::string & time_value, const std::string & time_unit_name) {
    IntFracPair int_frac(time_value);
    set(int_frac.getIntegerPart(), int_frac.getFractionalPart(), time_unit_name);
  }

  void Duration::get(const std::string & time_unit_name, long & time_value_int, double & time_value_frac) const {
    const TimeUnit & unit(TimeUnit::getUnit(time_unit_name));

    if (&TimeUnit::getUnit("Day") == &unit) {
      double day_frac = m_time.second * DayPerSec();
      time_value_int = m_time.first;
      time_value_frac = day_frac;
      if (m_time.first < 0 && m_time.second > 0.) {
        ++time_value_int;
        --time_value_frac;
      }

    } else {
      // Let the sec part have the same sign as the day part.
      long signed_day = m_time.first;
      double signed_sec = m_time.second;
      if (m_time.first < 0) {
        signed_day += 1;
        signed_sec -= SecPerDay();
      }

      // Compute time in a given unit.
      double signed_time = signed_sec * unit.getUnitPerSec();

      // TODO: Replace the following with simple use of IntFracPair class?
      // Compute fractional part as a value in range (-1., 1.).
      double int_part_dbl;
      time_value_frac = std::modf(signed_time, &int_part_dbl);

      // Compute integer part of return value using modf() result.
      int_part_dbl += signed_day * unit.getUnitPerDay();
      int_part_dbl += (int_part_dbl > 0. ? 0.5 : -0.5);
      if (int_part_dbl >= std::numeric_limits<long>::max() + 1.) {
        std::ostringstream os;
        os.precision(std::numeric_limits<double>::digits10);
        os << "Duration::getValue: overflow while converting " << int_part_dbl << " " << unit.getUnitString() << " to a long";
        throw std::runtime_error(os.str());
      } else if (int_part_dbl <= std::numeric_limits<long>::min() - 1.) {
        std::ostringstream os;
        os.precision(std::numeric_limits<double>::digits10);
        os << "Duration::getValue: underflow while converting " << int_part_dbl << " " << unit.getUnitString() << " to a long";
        throw std::runtime_error(os.str());
      }
      time_value_int = long(int_part_dbl);
    }
  }

  void Duration::get(const std::string & time_unit_name, double & time_value) const {
    time_value = get(time_unit_name);
  }

  double Duration::get(const std::string & time_unit_name) const {
    long time_value_int = 0;
    double time_value_frac = 0.;
    get(time_unit_name, time_value_int, time_value_frac);
    return time_value_int + time_value_frac;
  }

  Duration Duration::operator +(const Duration & dur) const {
    return Duration(add(m_time, dur.m_time));
  }

  Duration & Duration::operator +=(const Duration & dur) {
    m_time = add(m_time, dur.m_time);
    return *this;
  }

  Duration & Duration::operator -=(const Duration & dur) {
    m_time = add(m_time, negate(dur.m_time));
    return *this;
  }

  Duration Duration::operator -(const Duration & dur) const {
    return Duration(add(m_time, negate(dur.m_time)));
  }

  Duration Duration::operator -() const {
    return Duration(negate(m_time));
  }

  double Duration::operator /(const Duration & dur) const {
    std::string time_unit_name("Day");

    // If both times are less than a day, use seconds to preserve precision. This is not safe if either Duration
    // is longer than one day, because get method does integer math when the units are seconds, and days converted
    // to seconds can overflow in this case.
    if (0 == m_time.first && 0 == dur.m_time.first) time_unit_name = "Sec";

    return get(time_unit_name) / dur.get(time_unit_name);
  }

  bool Duration::operator !=(const Duration & dur) const {
    return m_time.first != dur.m_time.first || m_time.second != dur.m_time.second;
  }

  bool Duration::operator ==(const Duration & dur) const {
    return m_time.first == dur.m_time.first && m_time.second == dur.m_time.second;
  }

  bool Duration::operator <(const Duration & dur) const {
    return m_time.first < dur.m_time.first || (m_time.first == dur.m_time.first && m_time.second < dur.m_time.second);
  }

  bool Duration::operator <=(const Duration & dur) const {
    return m_time.first < dur.m_time.first || (m_time.first == dur.m_time.first && m_time.second <= dur.m_time.second);
  }

  bool Duration::operator >(const Duration & dur) const {
    return m_time.first > dur.m_time.first || (m_time.first == dur.m_time.first && m_time.second > dur.m_time.second);
  }

  bool Duration::operator >=(const Duration & dur) const {
    return m_time.first > dur.m_time.first || (m_time.first == dur.m_time.first && m_time.second >= dur.m_time.second);
  }

  bool Duration::equivalentTo(const Duration & other, const Duration & tolerance) const {
    return (*this > other ? (*this <= other + tolerance) : (other <= *this + tolerance));
  }

  /** \brief Convert any number of seconds into days & seconds in range [0, 86400).
      \param sec Input number of seconds.
  */
  Duration::time_type Duration::splitSec(double sec) const {
    // TODO: Replace the following with simple use of IntFracPair class?
    double offset;
    if (0. > sec) {
      offset = -.5;
    } else if (SecPerDay() <= sec) {
      offset = +.5;
    } else {
      return Duration::time_type(0, sec);
    }
    double double_day = std::floor(sec * DayPerSec()) + offset;
    long day;
    if (std::numeric_limits<long>::min() <= double_day && double_day <= std::numeric_limits<long>::max()) {
      day = long(double_day);
    } else if (std::numeric_limits<long>::min() > double_day) {
      std::ostringstream os;
      os << "Time " << double_day << " is too small to be expressed as a long integer";
      throw std::runtime_error(os.str());
    } else { // std::numeric_limits<long>::max() < double_day
      std::ostringstream os;
      os << "Time " << double_day << " is too large to be expressed as a long integer";
      throw std::runtime_error(os.str());
    }
    return Duration::time_type(day, sec - day * SecPerDay());
  }

  /** \brief Add two times which are represented by long day and double second fields. Seconds
      part of the result is guaranteed to be in the range [0., SecPerDay())
      \param t1 The first time being added.
      \param t2 The second time being added.
  */
  Duration::time_type Duration::add(Duration::time_type t1, Duration::time_type t2) const {
    // Day portions are added no matter what.
    long day = t1.first + t2.first;

    // Sum the two seconds portions.
    double sec = t1.second + t2.second;

    // Check for overflow.
    if (sec >= SecPerDay()) {
      ++day;
      // 1. Do not reuse sum variable from above, in order to preserve maximum precision.
      // 2. Prevent small negative values, which sometimes occur when performing floating point subtraction.
      sec = std::max(0., (t1.second - SecPerDay()) + t2.second);
    }
    return Duration::time_type(day, sec);
  }

  /** \brief Multiply by -1 a time represented by long day and double second fields. Seconds
      part of the result is guaranteed to be in the range [0., SecPerDay())
      \param t1 The first time being negated.
  */
  Duration::time_type Duration::negate(Duration::time_type t1) const {
    return Duration::time_type(-t1.first - 1, SecPerDay() - t1.second);
  }

  void Duration::set(long time_value_int, double time_value_frac, const std::string & time_unit_name) {
    const TimeUnit & unit(TimeUnit::getUnit(time_unit_name));
    long day = time_value_int / unit.getUnitPerDay();
    double sec = (time_value_int % unit.getUnitPerDay() + time_value_frac) * unit.getSecPerUnit();
    m_time = add(Duration::time_type(day, 0.), splitSec(sec));
  }

  std::ostream & operator <<(std::ostream & os, const Duration & dur) {
    dur.write(os);
    return os;
  }

  st_stream::OStream & operator <<(st_stream::OStream & os, const Duration & dur) {
    dur.write(os);
    return os;
  }

}
