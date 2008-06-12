/** \file Duration.h
    \brief Declaration of Duration class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_Duration_h
#define timeSystem_Duration_h

#include <iostream>
#include <limits>

#include "timeSystem/TimeConstant.h"

namespace st_stream {
  class OStream;
}

namespace timeSystem {

  /** \class Duration
      \brief Low level class used to represent an amount of time duration together with its nominal unit of measurement.
             Objects of this type represent physical lengths of time only if used together with a time system.
  */
  class Duration {
    public:

      /** \brief Construct a duration object from a pair of the numbers of days and seconds.
          \param day The number of days.
          \param sec The number of seconds.
      */
      Duration(long day, double sec);

      Duration();

      Duration(long time_value_int, double time_value_frac, const std::string & time_unit_name);

      Duration(double time_value, const std::string & time_unit_name);

      void get(const std::string & time_unit_name, long & time_value_int, double & time_value_frac) const;

      void get(const std::string & time_unit_name, double & time_value) const;

      double get(const std::string & time_unit_name) const;

      Duration operator +(const Duration & other) const;

      Duration & operator +=(const Duration & other);

      Duration & operator -=(const Duration & other);

      Duration operator -(const Duration & other) const;

      Duration operator -() const;

      double operator /(const Duration & other) const;

      bool operator !=(const Duration & other) const;

      bool operator ==(const Duration & other) const;

      bool operator <(const Duration & other) const;

      bool operator <=(const Duration & other) const;

      bool operator >(const Duration & other) const;

      bool operator >=(const Duration & other) const;

      bool equivalentTo(const Duration & other, const Duration & tolerance) const;

      template <typename StreamType>
      void write(StreamType & os) const;

      std::string describe() const;

    private:
      typedef std::pair<long, double> duration_type;

      Duration(const duration_type & new_duration): m_duration(new_duration) {}

      /** \brief Convert any number of seconds into days & seconds in range [0, 86400).
          \param sec Input number of seconds.
      */
      duration_type splitSec(double sec) const;

      /** \brief Add two time durations which are represented by long day and double second fields. Seconds
                 part of the result is guaranteed to be in the range [0., SecPerDay())
          \param t1 The first time duration being added.
          \param t2 The second time duration being added.
      */
      duration_type add(duration_type t1, duration_type t2) const;

      /** \brief Multiply by -1 a time duration represented by long day and double second fields. Seconds
            part of the result is guaranteed to be in the range [0., SecPerDay())
          \param t1 The first time duration being negated.
      */
      duration_type negate(duration_type t1) const;

      void set(long time_value_int, double time_value_frac, const std::string & time_unit_name);

      long round(double value, const std::string & time_unit) const;

      void convert(long day, double sec, duration_type & time_duration) const;

      duration_type m_duration;
  };

  template <typename StreamType>
  inline void Duration::write(StreamType & os) const {
    // Make the printed duration human-friendly, e.g, "-1 seconds" instead of "-1 day 86399 seconds".
    long print_day = m_duration.first;
    double print_sec = m_duration.second;
    if (m_duration.first < 0) {
      ++print_day;
      print_sec -= SecPerDay();
    }

    // Print the number of days, if not zero.
    if (print_day != 0) {
      os << print_day << " day";
      if (print_day != 1) os << "s";
      os << " ";
    }

    // Print the number of seconds.
    std::streamsize prec = os.precision(std::numeric_limits<double>::digits10);
    os << print_sec << " second";
    if (print_sec != 1.) os << "s";
    os.precision(prec);
  }

  std::ostream & operator <<(std::ostream & os, const Duration & time_duration);

  st_stream::OStream & operator <<(st_stream::OStream & os, const Duration & time_duration);

}

#endif
