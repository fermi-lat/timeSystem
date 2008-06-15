/** \File Duration.cxx
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

      long getSecPerUnit() const { return m_sec_per_unit; }

      std::string getUnitString() const { return m_unit_string; }

    protected:
      TimeUnit(long unit_per_day, long sec_per_unit, const std::string & unit_string):
        m_unit_per_day(unit_per_day), m_sec_per_unit(sec_per_unit), m_unit_string(unit_string) {}

    protected:
      typedef std::map<std::string, const TimeUnit *> container_type;

      static container_type & getContainer();

    private:
      long m_unit_per_day;
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

  TimeUnitDay::TimeUnitDay(): TimeUnit(1, SecPerDay(), "days") {
    container_type & container(getContainer());
    container["DAY"] = this;
    container["DAYS"] = this;
  }

  TimeUnitHour::TimeUnitHour(): TimeUnit(HourPerDay(), SecPerHour(), "hours") {
    container_type & container(getContainer());
    container["HOUR"] = this;
    container["HOURS"] = this;
  }

  TimeUnitMin::TimeUnitMin(): TimeUnit(MinPerDay(), SecPerMin(), "minutes") {
    container_type & container(getContainer());
    container["MIN"] = this;
    container["MINUTE"] = this;
    container["MINUTES"] = this;
  }

  TimeUnitSec::TimeUnitSec(): TimeUnit(SecPerDay(), 1, "seconds") {
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

  Duration::Duration(): m_duration(0, 0.) {}

  Duration::Duration(long day, double sec) {
    convert(day, sec, m_duration);
  }

  Duration::Duration(long time_value_int, double time_value_frac, const std::string & time_unit_name) {
    // Check the fractional part.
    if ((time_value_int == 0 && (time_value_frac <= -1. || time_value_frac >= +1.)) ||
        (time_value_int >  0 && (time_value_frac <   0. || time_value_frac >= +1.)) ||
        (time_value_int <  0 && (time_value_frac <= -1. || time_value_frac >   0.))) {
      std::ostringstream os;
      os.precision(std::numeric_limits<double>::digits10);
      os << "Fractional part out of bounds: " << time_value_frac << ".";
      throw std::runtime_error(os.str());
    }

    // Set the duration to this object.
    set(time_value_int, time_value_frac, time_unit_name);
  }

  Duration::Duration(double time_value, const std::string & time_unit_name) {
    set(0, time_value, time_unit_name);
  }

  const Duration & Duration::zero() {
    static const Duration s_zero_duration(duration_type(0, 0.));
    return s_zero_duration;
  }

  void Duration::get(const std::string & time_unit_name, long & time_value_int, double & time_value_frac) const {
    const TimeUnit & unit(TimeUnit::getUnit(time_unit_name));

    // Let the sec part have the same sign as the day part.
    double signed_day = m_duration.first;
    double signed_sec = m_duration.second;
    if (m_duration.first < 0) {
      signed_day += 1.;
      signed_sec -= SecPerDay();
    }

    // Compute time in a given unit.
    double signed_time = signed_sec / unit.getSecPerUnit();

    // Compute fractional part as a value in range (-1., 1.).
    double int_part_dbl;
    time_value_frac = std::modf(signed_time, &int_part_dbl);

    // Compute integer part of return value using modf() result.
    int_part_dbl += signed_day * unit.getUnitPerDay();
    time_value_int = round(int_part_dbl, unit.getUnitString());
  }

  void Duration::get(const std::string & time_unit_name, double & time_value) const {
    time_value = get(time_unit_name);
  }

  double Duration::get(const std::string & time_unit_name) const {
    const TimeUnit & unit(TimeUnit::getUnit(time_unit_name));
    return std::floor(double(m_duration.first) * unit.getUnitPerDay()) + m_duration.second / unit.getSecPerUnit();
  }

  Duration Duration::operator +(const Duration & other) const {
    return Duration(add(m_duration, other.m_duration));
  }

  Duration & Duration::operator +=(const Duration & other) {
    m_duration = add(m_duration, other.m_duration);
    return *this;
  }

  Duration & Duration::operator -=(const Duration & other) {
    m_duration = add(m_duration, negate(other.m_duration));
    return *this;
  }

  Duration Duration::operator -(const Duration & other) const {
    return Duration(add(m_duration, negate(other.m_duration)));
  }

  Duration Duration::operator -() const {
    return Duration(negate(m_duration));
  }

  double Duration::operator /(const Duration & other) const {
    std::string time_unit_name("Day");

    // If both times are less than a day, use seconds to preserve precision. This is not safe if either Duration
    // is longer than one day, because get method does integer math when the units are seconds, and days converted
    // to seconds can overflow in this case.
    if (0 == m_duration.first && 0 == other.m_duration.first) time_unit_name = "Sec";

    return get(time_unit_name) / other.get(time_unit_name);
  }

  bool Duration::operator !=(const Duration & other) const {
    return m_duration.first != other.m_duration.first || m_duration.second != other.m_duration.second;
  }

  bool Duration::operator ==(const Duration & other) const {
    return m_duration.first == other.m_duration.first && m_duration.second == other.m_duration.second;
  }

  bool Duration::operator <(const Duration & other) const {
    return m_duration.first < other.m_duration.first ||
      (m_duration.first == other.m_duration.first && m_duration.second < other.m_duration.second);
  }

  bool Duration::operator <=(const Duration & other) const {
    return m_duration.first < other.m_duration.first ||
      (m_duration.first == other.m_duration.first && m_duration.second <= other.m_duration.second);
  }

  bool Duration::operator >(const Duration & other) const {
    return m_duration.first > other.m_duration.first ||
      (m_duration.first == other.m_duration.first && m_duration.second > other.m_duration.second);
  }

  bool Duration::operator >=(const Duration & other) const {
    return m_duration.first > other.m_duration.first ||
      (m_duration.first == other.m_duration.first && m_duration.second >= other.m_duration.second);
  }

  bool Duration::equivalentTo(const Duration & other, const Duration & tolerance) const {
    return (*this > other ? (*this <= other + tolerance) : (other <= *this + tolerance));
  }

  std::string Duration::describe() const {
    std::ostringstream os;
    os << "Duration(" << m_duration.first << ", " << m_duration.second << ")";
    return os.str();
  }

  Duration::duration_type Duration::splitSec(double sec) const {
    double double_day = std::floor(sec / SecPerDay());
    long day = round(double_day, "days");
    return duration_type(day, sec - double_day * SecPerDay());
  }

  Duration::duration_type Duration::add(Duration::duration_type t1, Duration::duration_type t2) const {
    // Day portions are added no matter what.
    double double_day = double(t1.first) + double(t2.first);

    // Sum the two seconds portions.
    double sec = t1.second + t2.second;

    // Check for overflow.
    if (sec >= SecPerDay()) {
      double_day += 1.;
      // 1. Do not reuse sum variable from above, in order to preserve maximum precision.
      // 2. Prevent small negative values, which sometimes occur when performing floating point subtraction.
      sec = std::max(0., (t1.second - SecPerDay()) + t2.second);
    }

    // Round day part and return a new object.
    long day = round(double_day, "days");
    return duration_type(day, sec);
  }

  Duration::duration_type Duration::negate(Duration::duration_type t1) const {
    long day = round(-double(t1.first) - 1., "days");
    return duration_type(day, SecPerDay() - t1.second);
  }

  void Duration::set(long time_value_int, double time_value_frac, const std::string & time_unit_name) {
    // Convert units.
    const TimeUnit & unit(TimeUnit::getUnit(time_unit_name));
    long day = time_value_int / unit.getUnitPerDay();
    double sec = (time_value_int % unit.getUnitPerDay() + time_value_frac) * unit.getSecPerUnit();

    // Set the result to the data member.
    convert(day, sec, m_duration);
  }

  long Duration::round(double value, const std::string & time_unit) const {
    // Add one half for rounding oepartion.
    value += (value > 0. ? 0.5 : -0.5);

    // Check the value against the boundaries of long type.
    if (value >= std::numeric_limits<long>::max() + 1.) {
      // Throw an exception for a value too large.
      std::ostringstream os;
      os.precision(std::numeric_limits<double>::digits10);
      os << "Integer overflow in computing time duration of " << value << " " << time_unit << ".";
      throw std::runtime_error(os.str());

    } else if (value <= std::numeric_limits<long>::min() - 1.) {
      // Throw an exception for a value too small (i.e., large negative).
      std::ostringstream os;
      os.precision(std::numeric_limits<double>::digits10);
      os << "Integer underflow in computing time duration of " << value << " " << time_unit << ".";
      throw std::runtime_error(os.str());
    }

    // Return the rounded value.
    return long(value);
  }

  void Duration::convert(long day, double sec, duration_type & time_duration) const {
    time_duration = add(duration_type(day, 0.), splitSec(sec));
  }

  std::ostream & operator <<(std::ostream & os, const Duration & time_duration) {
    time_duration.write(os);
    return os;
  }

  st_stream::OStream & operator <<(st_stream::OStream & os, const Duration & time_duration) {
    time_duration.write(os);
    return os;
  }

}
