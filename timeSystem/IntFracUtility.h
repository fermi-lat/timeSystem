/** \file IntFracUtility.h
    \brief Declaration of IntFracUtility.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_IntFracUtility_h
#define timeSystem_IntFracUtility_h

#include <limits>
#include <string>

namespace timeSystem {

  /** \class IntFracUtility
      \brief Helper Class to check, parse, and format a pair of an integer part and a fractional part for time representation
             classess such as MJD format, holding an integer and a fractional part separately.
  */
  class IntFracUtility {
    public:
      /// \brief Return an IntFracUtility object.
      static IntFracUtility & getUtility();

      /** \brief Check consistency and validity of a given pair of integer and fractional parts, and throw an exception
                 if the pair has a problem.
          \param int_part Integer part to be tested.
          \param frac_part Fractional part to be tested.
      */
      void check(long int_part, double frac_part) const;

      /** \brief Convert a character string representing a floating-point number into a pair of the integer part of the number
                 and the fractional part of it.
          \param value_string Character string to parse.
          \param int_part Integer part of the parsed number.
          \param frac_part Fractional part of the parsed number.
      */
      void parse(const std::string & value_string, long & int_part, double & frac_part) const;

      /** \brief Convert a pair of an integer part and a fractional part representing a floating-point number into a character
                 string, and return it.
          \param int_part Integer part of a floating-point number to convert.
          \param frac_part Fractional part of a floating-point number to convert.
          \param precision Number of digits after a decimal point in a resultant character string.
      */
      std::string format(long int_part, double frac_part, std::streamsize precision = std::numeric_limits<double>::digits10) const;

      /** \brief Split a double-precision floating-point number into an integer part and a fractional part, checking an
                 expressible range by an integer variable, etc.  Both parts have the same sign as the input number.
          \param value_double Double-precision floating-point number to be converted.
          \param int_part Integer part of the input value.
          \param frac_part Fractional part of the input value.
      */
      void split(double value_double, long & int_part, double & frac_part) const;

    private:
      /// \brief Construct a IntFracUtility object.
      IntFracUtility();

      /** \brief Find an integer value between zero (0) and an integer value specified by the first argument of this method,
                 that is farthest from zero (i.e., the numerical distance to zero is largest), and that can be correctly
                 converted back and forth to a double-precision floating-point number.  The return value is a double-precision
                 floating-point number that is equal to the integer number found.
          \param upper_bound One end of search range (the other end is zero).
      */
      double findLargestInteger(long upper_bound) const;
  };

}

#endif
