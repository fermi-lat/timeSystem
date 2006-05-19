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

  // TODO: where to put IntFracPair class?
  class IntFracPair {
    public:
      IntFracPair(long int_part = 0, double frac_part = 0.): m_int_part(int_part), m_frac_part(frac_part) {}

      IntFracPair(double value) {
	// split value into integer part and fractional part.
        double int_part_dbl;
        m_frac_part = std::modf(value, &int_part_dbl);

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
	m_int_part = long(int_part_dbl);

	// clean the tail of fractional part.
        int num_digit_all = std::numeric_limits<double>::digits10;
        int num_digit_int = m_int_part == 0 ? 0 : int(std::floor(std::log10(std::fabs(double(m_int_part)))) + 0.5) + 1;
        int num_digit_frac = num_digit_all - num_digit_int;
        double factor = std::floor(std::exp(num_digit_frac * std::log(10.0)));
        m_frac_part = std::floor(m_frac_part * factor) / factor;
      }

      IntFracPair(std::string value) {
        // Read number into temporary double variable.
        double value_dbl;
        {
          std::istringstream iss(value);
          iss >> value_dbl;
        }

        // Compute integer part.
        if (value_dbl >= std::numeric_limits<long>::max() + 1.) {
	  std::ostringstream os;
	  os.precision(std::numeric_limits<double>::digits10);
	  os << "TimeValue::TimeValue: overflow while converting " << value_dbl << " to a long";
	  throw std::runtime_error(os.str());
        } else if (value_dbl <= std::numeric_limits<long>::min() - 1.) {
	  std::ostringstream os;
	  os.precision(std::numeric_limits<double>::digits10);
	  os << "TimeValue::TimeValue: underflow while converting " << value_dbl << " to a long";
	  throw std::runtime_error(os.str());
        }
        m_int_part = long(value_dbl);

        // Compute number of digits of integer part.
        int num_digit = (m_int_part == 0 ? 0 : int(std::floor(std::log10(std::fabs(double(m_int_part)))) + 0.5) + 1);

        // Skip leading zeros.
        std::string::iterator itor = value.begin();
        for (; (itor != value.end()) && ((*itor == '0') || !isdigit(*itor)); itor++) {}

        // Erase numbers in integer part.
        for (int ii_digit = 0; (itor != value.end()) && (ii_digit < num_digit); itor++) {
          if (isdigit(*itor)) {
            *itor = '0';
            ii_digit++;
          }
        }

        // Read in fractional part.
        {
          std::istringstream iss(value);
          iss >> m_frac_part;
        }
      }

      long getIntegerPart() const { return m_int_part; }

      double getFractionalPart() const { return m_frac_part; }

      template <typename StreamType>
      void write(StreamType & os) const {
        // write split value part.
        if (m_int_part == 0) {
          os << m_frac_part;
        } else {
          os << m_int_part;

          std::ostringstream oss;
          oss.precision(os.precision());
          oss.setf(std::ios::fixed);
          oss << m_frac_part;

          std::string frac_part_string = oss.str();
          std::string::iterator itor = frac_part_string.begin();
          for (; (itor != frac_part_string.end()) && (*itor != '.'); ++itor);
          for (; itor != frac_part_string.end(); ++itor) { os << *itor; }
        }
      }

    private:
      long m_int_part;
      double m_frac_part;
  };

  inline std::ostream & operator <<(std::ostream & os, const IntFracPair & int_frac) {
    int_frac.write(os);
    return os;
  }

  inline st_stream::OStream & operator <<(st_stream::OStream & os, const IntFracPair & int_frac) {
    int_frac.write(os);
    return os;
  }

  class TimeValue {
    public:
      typedef std::pair<long, double> split_type;
      typedef std::vector<long> carry_type;

      TimeValue(const IntFracPair & base_value): m_base_value(base_value), m_carry_over() {}

      TimeValue(long carry0, const IntFracPair & base_value): m_base_value(base_value), m_carry_over(pack(carry0)) {}

      TimeValue(long carry1, long carry0, const IntFracPair & base_value): m_base_value(base_value), m_carry_over(pack(carry1, carry0)) {}

      TimeValue(long carry2, long carry1, long carry0, const IntFracPair & base_value): m_base_value(base_value),
        m_carry_over(pack(carry2, carry1, carry0)) {}

      TimeValue(long carry3, long carry2, long carry1, long carry0, const IntFracPair & base_value): m_base_value(base_value),
        m_carry_over(pack(carry3, carry2, carry1, carry0)) {}

      TimeValue(long carry4, long carry3, long carry2, long carry1, long carry0, const IntFracPair & base_value):
        m_base_value(base_value), m_carry_over(pack(carry4, carry3, carry2, carry1, carry0)) {}

      TimeValue(long carry5, long carry4, long carry3, long carry2, long carry1, long carry0, const IntFracPair & base_value):
        m_base_value(base_value), m_carry_over(pack(carry5, carry4, carry3, carry2, carry1, carry0)) {}

      IntFracPair getBaseValue() const { return m_base_value; }

      long getCarryOver(carry_type::size_type idx = 0) const {
        long result = 0;
        if (m_carry_over.size() > idx) result = m_carry_over[idx];
        return result;
      }

      template <typename StreamType>
      void write(StreamType & os) const {
        // write carry over part.
        for (carry_type::const_reverse_iterator itor = m_carry_over.rbegin(); itor != m_carry_over.rend(); itor++) {
          os << *itor << ",";
        }

        // write base value.
        os << m_base_value;
      }

    private:

      carry_type pack(long carry0) {
        carry_type carry_over(1);
        carry_over[0] = carry0;
        return carry_over;
      }

      carry_type pack(long carry1, long carry0) {
        carry_type carry_over(2);
        carry_over[0] = carry0;
        carry_over[1] = carry1;
        return carry_over;
      }

      carry_type pack(long carry2, long carry1, long carry0) {
        carry_type carry_over(3);
        carry_over[0] = carry0;
        carry_over[1] = carry1;
        carry_over[2] = carry2;
        return carry_over;
      }

      carry_type pack(long carry3, long carry2, long carry1, long carry0) {
        carry_type carry_over(4);
        carry_over[0] = carry0;
        carry_over[1] = carry1;
        carry_over[2] = carry2;
        carry_over[3] = carry3;
        return carry_over;
      }

      carry_type pack(long carry4, long carry3, long carry2, long carry1, long carry0) {
        carry_type carry_over(5);
        carry_over[0] = carry0;
        carry_over[1] = carry1;
        carry_over[2] = carry2;
        carry_over[3] = carry3;
        carry_over[4] = carry4;
        return carry_over;
      }

      carry_type pack(long carry5, long carry4, long carry3, long carry2, long carry1, long carry0) {
        carry_type carry_over(6);
        carry_over[0] = carry0;
        carry_over[1] = carry1;
        carry_over[2] = carry2;
        carry_over[3] = carry3;
        carry_over[4] = carry4;
        carry_over[5] = carry5;
        return carry_over;
      }

      IntFracPair m_base_value;
      carry_type m_carry_over;
  };

  inline std::ostream & operator <<(std::ostream & os, const TimeValue & tv) {
    tv.write(os);
    return os;
  }

  inline st_stream::OStream & operator <<(st_stream::OStream & os, const TimeValue & tv) {
    tv.write(os);
    return os;
  }
}

#endif
