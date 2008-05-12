/** \file Duration
    \brief Declaration of Duration class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_Duration_h
#define timeSystem_Duration_h

#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "timeSystem/IntFracPair.h"
#include "timeSystem/TimeConstant.h"

#include "st_stream/Stream.h"

namespace timeSystem {

  typedef std::pair<long, double> moment_type;

  /** \enum TimeUnit_e Enumerated type representing different time units.

       Note: The four units above are "safe to use" precision-wise. Smaller units (millisecond,
       microsecond, ...) are not really a distinct unit from seconds. Larger units (Week, Year,
       Decade, Century, ...) would compromise the precision realized by this approach. The reason
       is that internally, Duration stores times as a whole number of days + a fractional number
       of seconds. The act of converting, say .5 years (~ 15768000 seconds) to days plus seconds would
       yield an intermediate result accurate only to 100 ns, whereas 182.5 days (~ .5 year) would be stored
       accurate to within 100 picoseconds.
  */
  enum TimeUnit_e {
    Day, Hour, Min, Sec
  };

  static const char * unit_name[] = { "days", "hours", "minutes", "seconds" };

  // Unit conversion factor.
  static const double per[][4] = {
    { 1.,           DayPerHour(), DayPerMin(),  DayPerSec() },
    { HourPerDay(), 1.,           HourPerMin(), HourPerSec() },
    { MinPerDay(),  MinPerHour(), 1.,           MinPerSec() },
    { SecPerDay(),  SecPerHour(), SecPerMin(),  1. }
  };

  static const long perlong[][4] = {
    { 1,                0,                0,               0 },
    { HourPerDayLong(), 1,                0,               0 },
    { MinPerDayLong(),  MinPerHourLong(), 1,               0 },
    { SecPerDayLong(),  SecPerHourLong(), SecPerMinLong(), 1 }
  };

  /** \class Duration
      \brief Low level class used to represent an amount of time together with its nominal unit of measurement. Objects
             of this type represent physical lengths of time only if used together with a time system.
  */
  class Duration {
    public:

      /** \brief Construct a duration object
          \param t The amount of time.
          \param unit The unit in which time t is measured.
      */
      Duration(long day = 0, double sec = 0.): m_time(add(time_type(day, 0.), splitSec(sec))) {}

      Duration(IntFracPair time_value, TimeUnit_e unit) {
        long day = time_value.getIntegerPart() / perlong[unit][Day];
        double sec = (time_value.getIntegerPart() % perlong[unit][Day] + time_value.getFractionalPart()) * perlong[Sec][unit];
        m_time = add(time_type(day, 0.), splitSec(sec));
      }

      /// \brief Return the current value of this time in given unit.
      IntFracPair getValue(TimeUnit_e unit) const;

      Duration operator +(const Duration & dur) const;

      Duration & operator +=(const Duration & dur);

      Duration & operator -=(const Duration & dur);

      Duration operator -(const Duration & dur) const;

      Duration operator -() const;

      double operator /(const Duration & dur) const;

      bool operator !=(const Duration & dur) const
        { return m_time.first != dur.m_time.first || m_time.second != dur.m_time.second; }
      bool operator ==(const Duration & dur) const
        { return m_time.first == dur.m_time.first && m_time.second == dur.m_time.second; }
      bool operator <(const Duration & dur) const
        { return m_time.first != dur.m_time.first ? m_time.first < dur.m_time.first : m_time.second < dur.m_time.second; }
      bool operator <=(const Duration & dur) const
        { return m_time.first != dur.m_time.first ? m_time.first < dur.m_time.first : m_time.second <= dur.m_time.second; }
      bool operator >(const Duration & dur) const
        { return m_time.first != dur.m_time.first ? m_time.first > dur.m_time.first : m_time.second > dur.m_time.second; }
      bool operator >=(const Duration & dur) const
        { return m_time.first != dur.m_time.first ? m_time.first > dur.m_time.first : m_time.second >= dur.m_time.second; }

      bool equivalentTo(const Duration & other, const Duration & tolerance) const {
        return (*this > other ? (*this <= other + tolerance) : (other <= *this + tolerance));
      }

      template <typename StreamType>
      void write(StreamType & os) const;

    private:
      typedef std::pair<long, double> time_type;
      Duration(const time_type & new_time): m_time(new_time) {}

      /** \brief Convert any number of seconds into days & seconds in range [0, 86400).
          \param sec Input number of seconds.
      */
      time_type splitSec(double sec) const {
        double offset;
        if (0. > sec) {
          offset = -.5;
        } else if (SecPerDay() <= sec) {
          offset = +.5;
        } else {
          return time_type(0, sec);
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
        return time_type(day, sec - day * SecPerDay());
      }

      /** \brief Add two times which are represented by long day and double second fields. Seconds
            part of the result is guaranteed to be in the range [0., SecPerDay())
          \param t1 The first time being added.
          \param t2 The second time being added.
      */
      time_type add(time_type t1, time_type t2) const {
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
        return time_type(day, sec);
      }

      /** \brief Multiply by -1 a time represented by long day and double second fields. Seconds
            part of the result is guaranteed to be in the range [0., SecPerDay())
          \param t1 The first time being negated.
      */
      time_type negate(time_type t1) const {
        return time_type(-t1.first - 1, SecPerDay() - t1.second);
      }

      time_type m_time;
  };

  inline IntFracPair Duration::getValue(TimeUnit_e unit) const {
    if (unit == Day) {
      double day_frac = m_time.second * DayPerSec();
      return ((m_time.first >= 0 || m_time.second == 0) ? IntFracPair(m_time.first, day_frac) :
        IntFracPair(m_time.first + 1, day_frac - 1.));
    } else {
      // Let the sec part have the same sign as the day part.
      long signed_day = m_time.first;
      double signed_sec = m_time.second;
      if (m_time.first < 0) {
        signed_day += 1;
        signed_sec -= SecPerDay();
      }

      // Compute time in a given unit.
      double signed_time = signed_sec * per[unit][Sec];

      // TODO: Replace the following with simple use of IntFracPair class?
      // Compute fractional part as a value in range (-1., 1.).
      double int_part_dbl;
      double frac_part = std::modf(signed_time, &int_part_dbl);

      // Compute integer part of return value using modf() result.
      int_part_dbl += signed_day * per[unit][Day];
      int_part_dbl += (int_part_dbl > 0. ? 0.5 : -0.5);
      if (int_part_dbl >= std::numeric_limits<long>::max() + 1.) {
        std::ostringstream os;
        os.precision(std::numeric_limits<double>::digits10);
        os << "Duration::getValue: overflow while converting " << int_part_dbl << " " << unit_name[unit] << " to a long";
        throw std::runtime_error(os.str());
      } else if (int_part_dbl <= std::numeric_limits<long>::min() - 1.) {
        std::ostringstream os;
        os.precision(std::numeric_limits<double>::digits10);
        os << "Duration::getValue: underflow while converting " << int_part_dbl << " " << unit_name[unit] << " to a long";
        throw std::runtime_error(os.str());
      }
      long int_part = long(int_part_dbl);

      // Return int_part and frac_part.
      return IntFracPair(int_part, frac_part);
    }
  }

  inline Duration Duration::operator +(const Duration & dur) const {
    return Duration(add(m_time, dur.m_time));
  }

  inline Duration & Duration::operator +=(const Duration & dur) {
    m_time = add(m_time, dur.m_time);
    return *this;
  }

  inline Duration & Duration::operator -=(const Duration & dur) {
    m_time = add(m_time, negate(dur.m_time));
    return *this;
  }

  inline Duration Duration::operator -(const Duration & dur) const {
    return Duration(add(m_time, negate(dur.m_time)));
  }

  inline Duration Duration::operator -() const {
    return Duration(negate(m_time));
  }

  inline double Duration::operator /(const Duration & dur) const {
    TimeUnit_e unit = Day;

    // If both times are less than a day, use seconds to preserve precision. This is not safe if either Duration
    // is longer than one day, because getValue does integer math when the units are seconds, and days converted
    // to seconds can overflow in this case.
    if (0 == m_time.first && 0 == dur.m_time.first) unit = Sec;

    return getValue(unit).getDouble() / dur.getValue(unit).getDouble();
  }

  template <typename StreamType>
  inline void Duration::write(StreamType & os) const {
    os << m_time.first << " day";
    if (m_time.first != 1) os << "s";
    std::streamsize prec = os.precision(std::numeric_limits<double>::digits10);
    os << " " << m_time.second << " second";
    if (m_time.second != 1.) os << "s";
    os.precision(prec);
  }

  inline std::ostream & operator <<(std::ostream & os, const Duration & dur) {
    dur.write(os);
    return os;
  }

  inline st_stream::OStream & operator <<(st_stream::OStream & os, const Duration & dur) {
    dur.write(os);
    return os;
  }

  typedef std::pair<Duration, Duration> Moment;

}

#endif
