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

  class IntFracPair;

  typedef std::pair<long, double> moment_type;

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

      Duration(long time_int, double time_frac, const std::string & unit_name);

      Duration(double time_value, const std::string & unit_name);

      Duration(const std::string & time_value, const std::string & unit_name);

      void get(const std::string & unit_name, long & time_int, double & time_frac) const;

      void get(const std::string & unit_name, double & time_value) const;

      double get(const std::string & unit_name) const;

      Duration operator +(const Duration & dur) const;

      Duration & operator +=(const Duration & dur);

      Duration & operator -=(const Duration & dur);

      Duration operator -(const Duration & dur) const;

      Duration operator -() const;

      double operator /(const Duration & dur) const;

      bool operator !=(const Duration & dur) const;

      bool operator ==(const Duration & dur) const;

      bool operator <(const Duration & dur) const;

      bool operator <=(const Duration & dur) const;

      bool operator >(const Duration & dur) const;

      bool operator >=(const Duration & dur) const;

      bool equivalentTo(const Duration & other, const Duration & tolerance) const;

      template <typename StreamType>
      void write(StreamType & os) const;

    private:
      typedef std::pair<long, double> time_type;

      Duration(const time_type & new_time): m_time(new_time) {}

      Duration(IntFracPair time_value, const std::string & unit_name);

      /** \brief Convert any number of seconds into days & seconds in range [0, 86400).
          \param sec Input number of seconds.
      */
      time_type splitSec(double sec) const;

      /** \brief Add two times which are represented by long day and double second fields. Seconds
            part of the result is guaranteed to be in the range [0., SecPerDay())
          \param t1 The first time being added.
          \param t2 The second time being added.
      */
      time_type add(time_type t1, time_type t2) const;

      /** \brief Multiply by -1 a time represented by long day and double second fields. Seconds
            part of the result is guaranteed to be in the range [0., SecPerDay())
          \param t1 The first time being negated.
      */
      time_type negate(time_type t1) const;

      void setValue(IntFracPair time_value, const std::string & unit_name);

      /// \brief Return the current value of this time in given unit.
      IntFracPair getValue(const std::string & unit_name) const;

      time_type m_time;
  };

  template <typename StreamType>
  inline void Duration::write(StreamType & os) const {
    os << m_time.first << " day";
    if (m_time.first != 1) os << "s";
    std::streamsize prec = os.precision(std::numeric_limits<double>::digits10);
    os << " " << m_time.second << " second";
    if (m_time.second != 1.) os << "s";
    os.precision(prec);
  }

  std::ostream & operator <<(std::ostream & os, const Duration & dur);

  st_stream::OStream & operator <<(st_stream::OStream & os, const Duration & dur);

}

#endif
