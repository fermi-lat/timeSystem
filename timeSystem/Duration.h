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

      /** \brief Construct a Duration object from a pair of the numbers of days and seconds.
          \param day The number of days.
          \param sec The number of seconds.
      */
      Duration(long day, double sec);

      /// \brief Construct a Duration object without an argument. Data members may not be initialized.
      Duration();

      /** \brief Construct a Duration object from a time value and a time unit.
          \param time_value_int Integer part of a time value.
          \param time_value_frac Fractional part of a time value.
          \param time_unit_name Name of time unit.
      */
      Duration(long time_value_int, double time_value_frac, const std::string & time_unit_name);

      /** \brief Construct a Duration object from a time value and a time unit.
          \param time_value Time value.
          \param time_unit_name Name of time unit.
      */
      Duration(double time_value, const std::string & time_unit_name);

      /// \brief Return a Duration object representing a zero-length time duration.
      static const Duration & zero();

      /** \brief Compute the length of time duration in the specified unit, and set the result to the arguments of this method.
          \param time_unit_name Name of time unit.
          \param time_value_int Integer part of the result is set to this argument.
          \param time_value_frac Fractional part of the result is set to this argument.
      */
      void get(const std::string & time_unit_name, long & time_value_int, double & time_value_frac) const;

      /** \brief Compute the length of time duration in the specified unit, and set the result to the arguments of this method.
          \param time_unit_name Name of time unit.
          \param time_value The result is set to this argument.
      */
      void get(const std::string & time_unit_name, double & time_value) const;

      /** \brief Compute the length of time duration in the specified unit, and return the result.
          \param time_unit_name Name of time unit.
      */
      double get(const std::string & time_unit_name) const;

      /** \brief Create a Duration object that represents a sum of a given Duration object and this object.
          \param other Duration object to be added.
      */
      Duration operator +(const Duration & other) const;

      /** \brief Add a given Duration object to this object, and set the result to this object.
          \param other Duration object to be added.
      */
      Duration & operator +=(const Duration & other);

      /** \brief Subtract a given Duration object from this object, and set the result to this object.
          \param other Duration object to subtract.
      */
      Duration & operator -=(const Duration & other);

      /** \brief Create a Duration object that represents this object subtracted by a given Duration object.
          \param other Duration object to subtract.
      */
      Duration operator -(const Duration & other) const;

      /// \brief Create a Duration object that represents the same time duration as this object, but with an opposite sign.
      Duration operator -() const;

      /** \brief Compute a ratio of this object over a given Duration object, and return the result.
          \param other Duration object to divide this object.
      */
      double operator /(const Duration & other) const;

      /** \brief Return true if this object represents a time duration that is not equal to the one represented by a given
                 Duration object, and return false otherwise.
          \param other Duration object to be compared.
      */
      bool operator !=(const Duration & other) const;

      /** \brief Return true if this object represents a time duration that is equal to the one represented by a given
                 Duration object, and return false otherwise.
          \param other Duration object to be compared.
      */
      bool operator ==(const Duration & other) const;

      /** \brief Return true if this object represents a time duration that is smaller than the one represented by a given
                 Duration object, and return false otherwise.
          \param other Duration object to be compared.
      */
      bool operator <(const Duration & other) const;

      /** \brief Return true if this object represents a time duration that is smaller than or equal to the one represented
                 by a given Duration object, and return false otherwise.
          \param other Duration object to be compared.
      */
      bool operator <=(const Duration & other) const;

      /** \brief Return true if this object represents a time duration that is larger than to the one represented by a given
                 Duration object, and return false otherwise.
          \param other Duration object to be compared.
      */
      bool operator >(const Duration & other) const;

      /** \brief Return true if this object represents a time duration that is larger than or equal to the one represented
                 by a given Duration object, and return false otherwise.
          \param other Duration object to be compared.
      */
      bool operator >=(const Duration & other) const;

      /** \brief Return true if a difference between a time duration represented by this object and the one represented by
                 a given Duration object is smaller than or equal to a specified time duration, and return false otherwise.
          \param other Duration object to be compared.
          \param tolerance Maximum allowed difference between this object and the given Duration object.
      */
      bool equivalentTo(const Duration & other, const Duration & tolerance) const;

      /** \brief Write a text representation of this object to an output stream.
          \param os Output stream to write a text representation of this object to.
      */
      template <typename StreamType>
      void write(StreamType & os) const;

      /// \brief Create a character string that contains a text description of this object.
      std::string describe() const;

    private:
      typedef std::pair<long, double> duration_type;

      /** \brief Construct a Duration object from a pair of an integer variable and a double variable.
          \param new_duration Time duration represented in a form of the internal representation.
      */
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

      /** \brief Set time duration to this object.
          \param time_value_int Integer part of the time duration to set.
          \param time_value_frac Fractional part of the time duration to set.
          \param time_unit_name Name of time unit.
      */
      void set(long time_value_int, double time_value_frac, const std::string & time_unit_name);

      /** \brief Round a floating-point number into an integer, paying attention to the expressible range of integer values.
          \param value Floating-point number to be rounded.
          \param time_unit Character string representing a time unit of the number to be rounded. The value of this argument
                 is used only in an error message of an exception thrown by this method.
      */
      long round(double value, const std::string & time_unit) const;

      /** \brief Convert a pair of days and seconds into the type of the internal variable, paying attention to
                 carry-overs and precision preservation.
      */
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
