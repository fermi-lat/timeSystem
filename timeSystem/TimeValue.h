/** \file TimeValue
    \brief Declaration of TimeValue class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_TimeValue_h
#define timeSystem_TimeValue_h

#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "st_stream/Stream.h"

namespace timeSystem {

  class TimeValue {
    public:
      typedef std::pair<long, double> split_type;
      typedef std::vector<long> carry_type;

      TimeValue(const split_type & split_value): m_split_value(split_value), m_carry_over() {}

      TimeValue(long carry0, const split_type & split_value): m_split_value(split_value), m_carry_over(1) {
        m_carry_over[0] = carry0;
      }

      TimeValue(long carry1, long carry0, const split_type & split_value): m_split_value(split_value), m_carry_over(2) {
        m_carry_over[0] = carry0;
        m_carry_over[1] = carry1;
      }

      TimeValue(long carry2, long carry1, long carry0, const split_type & split_value): m_split_value(split_value),
        m_carry_over(3) {
        m_carry_over[0] = carry0;
        m_carry_over[1] = carry1;
        m_carry_over[2] = carry2;
      }

      TimeValue(long carry3, long carry2, long carry1, long carry0, const split_type & split_value): m_split_value(split_value),
        m_carry_over(4) {
        m_carry_over[0] = carry0;
        m_carry_over[1] = carry1;
        m_carry_over[2] = carry2;
        m_carry_over[3] = carry3;
      }

      TimeValue(long carry4, long carry3, long carry2, long carry1, long carry0, const split_type & split_value):
        m_split_value(split_value), m_carry_over(5) {
        m_carry_over[0] = carry0;
        m_carry_over[1] = carry1;
        m_carry_over[2] = carry2;
        m_carry_over[3] = carry3;
        m_carry_over[4] = carry4;
      }

      TimeValue(long carry5, long carry4, long carry3, long carry2, long carry1, long carry0, const split_type & split_value):
        m_split_value(split_value), m_carry_over(6) {
        m_carry_over[0] = carry0;
        m_carry_over[1] = carry1;
        m_carry_over[2] = carry2;
        m_carry_over[3] = carry3;
        m_carry_over[4] = carry4;
        m_carry_over[5] = carry5;
      }

      TimeValue(double value): m_split_value(split(value)), m_carry_over() {}

      TimeValue(long carry0, const double & value): m_split_value(split(value)), m_carry_over(1) {
        m_carry_over[0] = carry0;
      }

      TimeValue(long carry1, long carry0, const double & value): m_split_value(split(value)), m_carry_over(2) {
        m_carry_over[0] = carry0;
        m_carry_over[1] = carry1;
      }

      TimeValue(long carry2, long carry1, long carry0, const double & value): m_split_value(split(value)),
        m_carry_over(3) {
        m_carry_over[0] = carry0;
        m_carry_over[1] = carry1;
        m_carry_over[2] = carry2;
      }

      TimeValue(long carry3, long carry2, long carry1, long carry0, const double & value): m_split_value(split(value)),
        m_carry_over(4) {
        m_carry_over[0] = carry0;
        m_carry_over[1] = carry1;
        m_carry_over[2] = carry2;
        m_carry_over[3] = carry3;
      }

      TimeValue(long carry4, long carry3, long carry2, long carry1, long carry0, const double & value):
        m_split_value(split(value)), m_carry_over(5) {
        m_carry_over[0] = carry0;
        m_carry_over[1] = carry1;
        m_carry_over[2] = carry2;
        m_carry_over[3] = carry3;
        m_carry_over[4] = carry4;
      }

      TimeValue(long carry5, long carry4, long carry3, long carry2, long carry1, long carry0, const double & value):
        m_split_value(split(value)), m_carry_over(6) {
        m_carry_over[0] = carry0;
        m_carry_over[1] = carry1;
        m_carry_over[2] = carry2;
        m_carry_over[3] = carry3;
        m_carry_over[4] = carry4;
        m_carry_over[5] = carry5;
      }

      long getIntegerPart(carry_type::size_type idx = 0) const {
        long result = 0;
        if (0 == idx) result = m_split_value.first;
        else if (m_carry_over.size() >= idx) result = m_carry_over[idx - 1];
        return result;
      }

      double getFractionalPart() const { return m_split_value.second; }

//      double reduceToDouble() const { return m_split_value.first + m_split_value.second; };

      // TODO: implement these.  Use a template?
      //void write(std::ostream & os) const;

      void write(st_stream::OStream & os) const {

        if (m_split_value.first == 0) {
          os << m_split_value.second;
        } else {
          os << m_split_value.first;

          std::ostringstream oss;
          oss.precision(os.precision());
          oss.setf(std::ios::fixed);
          oss << m_split_value.second;

          std::string frac_part_string = oss.str();
          std::string::iterator itor = frac_part_string.begin();
          for (; (itor != frac_part_string.end()) && (*itor != '.'); ++itor);
          for (; itor != frac_part_string.end(); ++itor) { os << *itor; }
        }
      }

    private:
      split_type split(double value) {
        split_type split_value;
	// split value into integer part and fractional part.
        double int_part_dbl;
        split_value.second = std::modf(value, &int_part_dbl);

	// round integer part of the value.
        int_part_dbl += (int_part_dbl > 0. ? 0.5 : -0.5);
	if (int_part_dbl >= std::numeric_limits<long>::max() + 1.) {
	  std::ostringstream os;
	  os.precision(std::numeric_limits<double>::digits10);
	  os << "TimeValue::TimeValue: overflow while converting " << int_part_dbl << " to a long";
	  throw std::runtime_error(os.str());
	} else if (int_part_dbl <= std::numeric_limits<long>::min() - 1.) {
	  std::ostringstream os;
	  os.precision(std::numeric_limits<double>::digits10);
	  os << "TimeValue::TimeValue: underflow while converting " << int_part_dbl << " to a long";
	  throw std::runtime_error(os.str());
	}
	split_value.first = long(int_part_dbl);

	// clean the tail of fractional part.
        int num_digit_all = std::numeric_limits<double>::digits10;
        int num_digit_int = split_value.first == 0 ? 0 : int(std::floor(std::log10(std::fabs(double(m_split_value.first)))) + 0.5) + 1;
        int num_digit_frac = num_digit_all - num_digit_int;
        double factor = std::floor(std::exp(num_digit_frac * std::log(10.0)));
        split_value.second = std::floor(split_value.second * factor) / factor;

        return split_value;
      }

      split_type m_split_value;
      carry_type m_carry_over;
  };

  inline st_stream::OStream & operator <<(st_stream::OStream & os, const TimeValue & tv) {
    tv.write(os);
    return os;
  }
}

#endif
