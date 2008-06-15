/** \file IntFracPair.cxx
    \brief Implementation of IntFracPair class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/IntFracPair.h"

#include <cctype>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace timeSystem {

  IntFracPair::IntFracPair(long int_part, double frac_part) {
    // Check the fractional part.
    if ((int_part == 0 && (frac_part <= -1. || frac_part >= +1.)) ||
        (int_part >  0 && (frac_part <   0. || frac_part >= +1.)) ||
        (int_part <  0 && (frac_part <= -1. || frac_part >   0.))) {
      std::ostringstream os;
      os.precision(std::numeric_limits<double>::digits10);
      os << "Fractional part out of bounds: " << frac_part << ".";
      throw std::runtime_error(os.str());
    }

    // Set the given values to this object.
    m_int_part = int_part;
    m_frac_part = frac_part;
  }

  IntFracPair::IntFracPair(double value) {
    // split value into integer part and fractional part.
    double int_part_dbl = 0.;
    m_frac_part = std::modf(value, &int_part_dbl);

    // round integer part of the value.
    int_part_dbl += (int_part_dbl > 0. ? 0.5 : -0.5);
    m_int_part = convert(int_part_dbl);

    // clean the tail of fractional part.
    int num_digit_all = std::numeric_limits<double>::digits10;
    int num_digit_int = m_int_part == 0 ? 0 : int(std::floor(std::log10(std::fabs(double(m_int_part)))) + 0.5) + 1;
    int num_digit_frac = num_digit_all - num_digit_int;
    double factor = std::floor(std::exp(num_digit_frac * std::log(10.0)));
    m_frac_part = std::floor(m_frac_part * factor) / factor;
  }

  IntFracPair::IntFracPair(const std::string & input_value) {
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
    m_int_part = convert(value_dbl);

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

  long IntFracPair::convert(double value) const {
    // Check whether the given value can be converted into a long value.
    if (value >= std::numeric_limits<long>::max() + 1.) {
      std::ostringstream os;
      os.precision(std::numeric_limits<double>::digits10);
      os << "Integer part too large: overflow while converting " << value << " to a long";
      throw std::runtime_error(os.str());
    } else if (value <= std::numeric_limits<long>::min() - 1.) {
      std::ostringstream os;
      os.precision(std::numeric_limits<double>::digits10);
      os << "Integer part too small: underflow while converting " << value << " to a long";
      throw std::runtime_error(os.str());
    }

    // Return a converted value.
    return long(value);
  }

  std::ostream & operator <<(std::ostream & os, const IntFracPair & int_frac) {
    int_frac.write(os);
    return os;
  }

  st_stream::OStream & operator <<(st_stream::OStream & os, const IntFracPair & int_frac) {
    int_frac.write(os);
    return os;
  }

}
