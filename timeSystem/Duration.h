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

#include "timeSystem/TimeConstant.h"

#include "st_stream/Stream.h"

namespace timeSystem {

  // TODO is there a better place for this?
  typedef std::pair<long, double> time_type;

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

      // TODO Implement this?
//      Duration(time_type time_value, bool day = true): m_time(add(time_type(day, 0.), splitSec(sec))) {}

      /// \brief Return the current value of this time in days.
      time_type day() const;

      /// \brief Return the current value of this time in seconds.
      time_type sec() const;

      Duration operator +(const Duration & dur) const;

      Duration & operator +=(const Duration & dur);

      Duration & operator -=(const Duration & dur);

      Duration operator -(const Duration & dur) const;

      Duration operator -() const;

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

      void write(std::ostream & os) const;

      void write(st_stream::OStream & os) const;

    private:
      Duration(const time_type & new_time): m_time(new_time) {}

#if 0
      // This is commented out because there are problems converting days into days + seconds
      // without losing precision. Not needed with current scheme because constructors work
      // directly from long values for days.
      /** \brief Convert any number of days into days & seconds in range [0, 86400).
          \param day Input number of days.
      */
      time_type splitDay(double day) const {
        double offset;
        if (0. > day) {
          offset = -.5;
        } else {
          offset = +.5;
        }
        double int_part;
        double frac_part = std::modf(day, &int_part);
        int num_digit_all = std::numeric_limits<double>::digits10;
        int num_digit_int = int_part == 0 ? 0 : int(floor(log10(fabs(int_part))) + 0.5) + 1;
        int num_digit_frac = num_digit_all - num_digit_int;
        double factor = floor(exp(num_digit_frac * log(10.0)));
        frac_part = floor(frac_part * factor) / factor;
        return time_type(long(int_part + offset), frac_part * SecPerDay());

        long del = long(std::floor(day) + offset);
        return time_type(del, (day - del) * SecPerDay());
      }
#endif

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
          // Do not reuse sum variable from above, in order to preserve maximum precision.
          sec = (t1.second - SecPerDay()) + t2.second;
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

  inline time_type Duration::day() const {
    double day_frac = m_time.second * DayPerSec();
    return ((m_time.first >= 0 || m_time.second == 0) ? time_type(m_time.first, day_frac) :
	    time_type(m_time.first + 1, day_frac - 1.));
  }

  inline time_type Duration::sec() const {
    // Let the sec part have the same sign as the day part.
    long signed_day = m_time.first;
    double signed_sec = m_time.second;
    if (m_time.first < 0) {
      signed_day += 1;
      signed_sec -= SecPerDay();
    }

    // Compute fractional part of second as a value in range (-1., 1.).
    double int_part;
    double sec_frac = std::modf(signed_sec, &int_part);

    // Compute integer part of second using modf() result.
    double sec = signed_day * SecPerDay() + int_part;
    double rounding = (sec > 0. ? 0.5 : -0.5);
    double sec_half_int = sec + rounding;
    if (sec_half_int >= std::numeric_limits<long>::max() + 1.) {
      std::ostringstream os;
      os.precision(std::numeric_limits<double>::digits10);
      os << "Duration::sec: overflow while converting " << sec_half_int << " seconds to a long";
      throw std::runtime_error(os.str());
    } else if (sec_half_int <= std::numeric_limits<long>::min() - 1.) {
      std::ostringstream os;
      os.precision(std::numeric_limits<double>::digits10);
      os << "Duration::sec: underflow while converting " << sec_half_int << " seconds to a long";
      throw std::runtime_error(os.str());
    }
    long sec_int = long(sec_half_int);

    return time_type(sec_int, sec_frac);
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

  inline void Duration::write(std::ostream & os) const {
    os << m_time.first << " day";
    if (m_time.first != 1) os << "s";
    std::streamsize prec = os.precision(std::numeric_limits<double>::digits10);
    os << ", " << m_time.second << " second";
    if (m_time.second != 1.) os << "s";
    os.precision(prec);
  }

  inline void Duration::write(st_stream::OStream & os) const {
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

}

#endif
