/** \file IntFracPair.h
    \brief Declaration of IntFracPair class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_IntFracPair_h
#define timeSystem_IntFracPair_h

#include <sstream>

#include "st_stream/Stream.h"

namespace timeSystem {

  class IntFracPair {
    public:
      IntFracPair(): m_int_part(0), m_frac_part(0.) {}

      // TODO: What if frac_part is out of [0., 1.) range?
      IntFracPair(long int_part, double frac_part): m_int_part(int_part), m_frac_part(frac_part) {}

      IntFracPair(double value);

      explicit IntFracPair(const std::string & input_value);

      long getIntegerPart() const { return m_int_part; }

      double getFractionalPart() const { return m_frac_part; }

      double getDouble() const { return m_int_part + m_frac_part; }

      template <typename StreamType>
      void write(StreamType & os) const;

    private:
      long m_int_part;
      double m_frac_part;
  };

  template <typename StreamType>
  inline void IntFracPair::write(StreamType & os) const {
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
