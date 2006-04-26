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

#include "st_stream/Stream.h"

namespace timeSystem {

  class TimeValue {
    public:
      TimeValue(long int_part = 0, double frac_part = 0.): m_int_part(int_part), m_frac_part(frac_part) {};

      TimeValue(double value) {
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
      };

      long getIntegerPart() const { return m_int_part; };

      double getFractionalPart() const { return m_frac_part; };

      double reduceToDouble() const { return m_int_part + m_frac_part; };

      // TODO: implement these.  Use a template?
      //void write(std::ostream & os) const;
      //void write(st_stream::OStream & os) const;

    private:
      long m_int_part;
      double m_frac_part;
  };

}

#endif
