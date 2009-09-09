/** \File Duration.cxx
    \brief Implementation of Duration class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>

#include "timeSystem/Duration.h"
#include "timeSystem/IntFracUtility.h"
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
      /** \brief Return a TimeUnit object.
          \param time_unit_name Name of a desired time unit.
      */
      static const TimeUnit & getUnit(const std::string & time_unit_name);

      /// \brief Return a ratio of this time unit over a day.
      long getUnitPerDay() const { return m_unit_per_day; }

      /// \brief Return a ratio of a second over this time unit.
      long getSecPerUnit() const { return m_sec_per_unit; }

      /// \brief Return a character string that represents this time unit.
      std::string getUnitString() const { return m_unit_string; }

    protected:
      /** \brief Construct a TimeUnit object.
          \param unit_per_day Ratio of this time unit over a day.
          \param sec_per_unit Ratio of a second over this time unit.
          \param unit_string Character string that represents this time unit.
      */
      TimeUnit(long unit_per_day, long sec_per_unit, const std::string & unit_string):
        m_unit_per_day(unit_per_day), m_sec_per_unit(sec_per_unit), m_unit_string(unit_string) {}

    protected:
      typedef std::map<std::string, const TimeUnit *> container_type;

      /// \brief Return a container of registered time units.
      static container_type & getContainer();

    private:
      long m_unit_per_day;
      long m_sec_per_unit;
      std::string m_unit_string;
  };

  /** \class TimeUnitDay
      \brief Class to represent a time unit of day.
  */
  class TimeUnitDay: public TimeUnit {
    public:
      /// \brief Construct a TimeUnitDay object.
      TimeUnitDay();
  };

  /** \class TimeUnitDay
      \brief Class to represent a time unit of hour.
  */
  class TimeUnitHour: public TimeUnit {
    public:
      /// \brief Construct a TimeUnitHour object.
      TimeUnitHour();
  };

  /** \class TimeUnitDay
      \brief Class to represent a time unit of minute.
  */
  class TimeUnitMin: public TimeUnit {
    public:
      /// \brief Construct a TimeUnitMin object.
      TimeUnitMin();
  };

  /** \class TimeUnitDay
      \brief Class to represent a time unit of secon.
  */
  class TimeUnitSec: public TimeUnit {
    public:
      /// \brief Construct a TimeUnitSec object.
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
    if (container.end() == cont_itor) throw std::runtime_error("No such time unit implemented: " + time_unit_name);
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
    set(day, sec);
  }

  Duration::Duration(long time_value_int, double time_value_frac, const std::string & time_unit_name) {
    // Check the fractional part.
    const IntFracUtility & utility(IntFracUtility::getUtility());
    utility.check(time_value_int, time_value_frac);

    // Convert units.
    const TimeUnit & unit(TimeUnit::getUnit(time_unit_name));
    long day = time_value_int / unit.getUnitPerDay();
    double sec = (time_value_int % unit.getUnitPerDay() + time_value_frac) * unit.getSecPerUnit();

    // Set the result to the data member.
    set(day, sec);
  }

  Duration::Duration(double time_value, const std::string & time_unit_name) {
    // Convert units.
    const TimeUnit & unit(TimeUnit::getUnit(time_unit_name));
    double sec = time_value * unit.getSecPerUnit();

    // Set the result to the data member.
    set(0, sec);
  }

  const Duration & Duration::zero() {
    static const Duration s_zero_duration(duration_type(0, 0.));
    return s_zero_duration;
  }

  void Duration::get(const std::string & time_unit_name, long & time_value_int, double & time_value_frac) const {
    const TimeUnit & unit(TimeUnit::getUnit(time_unit_name));

    // Let the sec part have the same sign as the day part.
    long signed_day = m_duration.first;
    double signed_sec = m_duration.second;
    if (signed_day < 0) {
      signed_day++; // Note: this operation never causes integer over/underflow.
      signed_sec -= SecPerDay();
    }

    // Convert the seconds portion to a given time unit.
    double signed_time = signed_sec / unit.getSecPerUnit();

    // Compute the integer and the fractional parts of a time value coming from the seconds portion.
    const IntFracUtility & utility(IntFracUtility::getUtility());
    long int_part_from_sec = 0;
    double frac_part = 0.;
    utility.split(signed_time, int_part_from_sec, frac_part);

    // Compute the integer part coming from the days portion.
    if (signed_day > std::numeric_limits<long>::max() / unit.getUnitPerDay()) {
      // Throw an exception for a value too large after multiplication.
      std::ostringstream os;
      os << "Integer overflow in expressing time duration of " << *this << " in " << unit.getUnitString();
      throw std::runtime_error(os.str());

    } else if (signed_day < std::numeric_limits<long>::min() / unit.getUnitPerDay()) {
      // Throw an exception for a value too small after multiplication.
      std::ostringstream os;
      os << "Integer underflow in expressing time duration of " << *this << " in " << unit.getUnitString();
      throw std::runtime_error(os.str());
    }
    long int_part_from_day = signed_day * unit.getUnitPerDay();

    // Add the integer part coming from the days portion.
    long int_part = add(int_part_from_day, int_part_from_sec);

    // Set the result.
    time_value_int = int_part;
    time_value_frac = frac_part;
  }

  void Duration::get(const std::string & time_unit_name, double & time_value) const {
    time_value = get(time_unit_name);
  }

  double Duration::get(const std::string & time_unit_name) const {
    const TimeUnit & unit(TimeUnit::getUnit(time_unit_name));
    return std::floor(static_cast<double>(m_duration.first) * unit.getUnitPerDay()) + m_duration.second / unit.getSecPerUnit();
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
    os.precision(std::numeric_limits<double>::digits10);
    os << "Duration(" << m_duration.first << ", " << m_duration.second << ")";
    return os.str();
  }

  void Duration::set(long day, double sec) {
    // Split the given number of seconds into days and seconds portions.
    double double_day = std::floor(sec / SecPerDay());
    double double_sec = sec - double_day * SecPerDay();

    // Convert the days portion into an integer type.
    const IntFracUtility & utility(IntFracUtility::getUtility());
    long int_day = 0;
    double frac_day = 0.;
    utility.split(double_day, int_day, frac_day);

    // Correct the results for a potential rounding error in a result of std::floor function.
    if (frac_day > .5) int_day = add(int_day, 1);
    else if (frac_day < -.5) int_day = add(int_day, -1);
    frac_day = 0.;

    // Add the given number of days to the days portion from the given seconds.
    int_day = add(day, int_day);

    // Set the result to the internal variable.
    m_duration.first = int_day;
    m_duration.second = double_sec;
  }

  long Duration::add(long int1, long int2) const {
    // Check for integer over/underflow.
    if (int1 >=0 && std::numeric_limits<long>::max() - int1 < int2) {
      // Throw an exception for a value too large after addition.
      std::ostringstream os;
      os << "Integer overflow in adding two integer values: " << int1 << " and " << int2;
      throw std::runtime_error(os.str());

    } else if (int1 <=0 && std::numeric_limits<long>::min() - int1 > int2) {
      // Throw an exception for a value too small after addition.
      std::ostringstream os;
      os << "Integer underflow in adding two integer values: " << int1 << " and " << int2;
      throw std::runtime_error(os.str());
    }

    // Return the sum.
    return int1 + int2;
  }

  Duration::duration_type Duration::add(Duration::duration_type t1, Duration::duration_type t2) const {
    // Sum the two seconds portions.
    double total_sec = t1.second + t2.second;

    // Check for carry-over, and sum the days portions.
    long total_day = 0;
    if (total_sec >= SecPerDay()) {
      // Add the days portions, with careful attention to the order of addition.
      if (t1.first != std::numeric_limits<long>::max()) {
        // Safe to increment the days portion of t1.
        total_day = add(t1.first+1, t2.first);

      } else if (t2.first != std::numeric_limits<long>::max()) {
        // Safe to increment the days portion of t2.
        total_day = add(t1.first, t2.first+1);

      } else {
        // This case overflows no matter what.
        total_day = add(add(t1.first, t2.first), 1);

      }

      // Re-compute the seconds portion, noting the following points:
      // 1. Do not reuse sum variable from above, in order to preserve maximum precision.
      // 2. Prevent small negative values, which sometimes occur when performing floating point subtraction.
      total_sec = std::max(0., (t1.second - SecPerDay()) + t2.second);

    } else {
      // Add the days portions.
      total_day = add(t1.first, t2.first);

    }

    // Return the sum.
    return duration_type(total_day, total_sec);
  }

  Duration::duration_type Duration::negate(Duration::duration_type t1) const {
    // Check the day part of the value against the boundaries of long type.
    if (t1.first <= 0 && t1.first + std::numeric_limits<long>::max() < -1) {
      // Throw an exception for a value too large after negation.
      std::ostringstream os;
      os << "Integer overflow in negating time duration of " << *this;
      throw std::runtime_error(os.str());
      
    } else if (t1.first >= 0 && t1.first + std::numeric_limits<long>::min() > -1) {
      // Throw an exception for a value too small after negation.
      std::ostringstream os;
      os << "Integer underflow in negating time duration of " << *this;
      throw std::runtime_error(os.str());
    }

    // Compute and return the negative of this object.
    return duration_type(-t1.first - 1, SecPerDay() - t1.second);
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
