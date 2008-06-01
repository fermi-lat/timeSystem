/** \file IntFracPair
    \brief Declaration of IntFracPair class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_IntFracPair_h
#define timeSystem_IntFracPair_h

#include <cctype>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

#include "st_stream/Stream.h"

namespace timeSystem {

  class IntFracPair {
    public:
      IntFracPair(): m_int_part(0), m_frac_part(0.) {}

      // TODO: What if frac_part is out of [0., 1.) range?
      IntFracPair(long int_part, double frac_part): m_int_part(int_part), m_frac_part(frac_part) {}

      IntFracPair(double value) {
	// split value into integer part and fractional part.
        double int_part_dbl;
        m_frac_part = std::modf(value, &int_part_dbl);

	// round integer part of the value.
        int_part_dbl += (int_part_dbl > 0. ? 0.5 : -0.5);
	if (int_part_dbl >= std::numeric_limits<long>::max() + 1.) {
	  std::ostringstream os;
	  os.precision(std::numeric_limits<double>::digits10);
	  os << "IntFracPair::IntFracPair: overflow while converting " << int_part_dbl << " to a long";
	  throw std::runtime_error(os.str());
	} else if (int_part_dbl <= std::numeric_limits<long>::min() - 1.) {
	  std::ostringstream os;
	  os.precision(std::numeric_limits<double>::digits10);
	  os << "IntFracPair::IntFracPair: underflow while converting " << int_part_dbl << " to a long";
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

      explicit IntFracPair(const std::string & input_value) {
        std::string value;
        // Read number into temporary double variable.
        double value_dbl = 0.;
        {
          // Remove trailing space to prevent spurious errors.
          std::string::size_type trail = input_value.find_last_not_of(" \t\v\n");
          if (std::string::npos != trail) value = input_value.substr(0, trail + 1);
          std::istringstream iss(value);
          iss >> value_dbl;
          if (iss.fail() || !iss.eof())
            throw std::runtime_error("IntFracPair::IntFracPair: cannot construct from \"" + input_value + "\"");
        }

        // Compute integer part.
        if (value_dbl >= std::numeric_limits<long>::max() + 1.) {
	  std::ostringstream os;
	  os.precision(std::numeric_limits<double>::digits10);
	  os << "IntFracPair::IntFracPair: overflow while converting " << value_dbl << " to a long";
	  throw std::runtime_error(os.str());
        } else if (value_dbl <= std::numeric_limits<long>::min() - 1.) {
	  std::ostringstream os;
	  os.precision(std::numeric_limits<double>::digits10);
	  os << "IntFracPair::IntFracPair: underflow while converting " << value_dbl << " to a long";
	  throw std::runtime_error(os.str());
        }
        m_int_part = long(value_dbl);

        // Compute number of digits of integer part.
        int num_digit = (m_int_part == 0 ? 0 : int(std::floor(std::log10(std::fabs(double(m_int_part)))) + 0.5) + 1);

        // Skip leading zeros, whitespace, and non-digits.
        std::string::iterator itor = value.begin();
        for (; itor != value.end() && ('0' == *itor || 0 == std::isdigit(*itor)); ++itor) {}

        // Erase numbers in integer part.
        for (int ii_digit = 0; itor != value.end() && ii_digit < num_digit; ++itor) {
          if (0 != std::isdigit(*itor)) {
            *itor = '0';
            ++ii_digit;
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

      double getDouble() const { return m_int_part + m_frac_part; }

      template <typename StreamType>
      void write(StreamType & os) const {
        if (m_int_part == 0) {
          // Write fractional part only.
          os << m_frac_part;
        } else {
          // Write integer part first.
          os << m_int_part;

          // Write fractional part into a temporary string.
          std::ostringstream oss;
          // TODO: Handle other settings/flags of "os" other than precision?
          oss.precision(os.precision());
          // TODO: Handle the case that "os" comes with std::ios::scientific?
          oss.setf(std::ios::fixed);
          oss << m_frac_part;
          std::string frac_part_string = oss.str();

          // Truncate trailing 0s.
          std::string::size_type pos = frac_part_string.find_last_not_of("0 \t\v\n");
          if (std::string::npos != pos) frac_part_string.erase(pos+1);

          // Remove a decimal point ('.') at the end.
          pos = frac_part_string.size() - 1;
          if ('.' == frac_part_string[pos]) frac_part_string.erase(pos);

          // Skip until a decimal point ('.') is found, then output the rest.
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

}

#endif
