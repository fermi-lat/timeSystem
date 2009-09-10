/** \file IntFracUtility.cxx
    \brief Implementation of IntFracUtility.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/IntFracUtility.h"

#include <cctype>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {

  /** \brief Helper function for IntFracUtility::parse method to convert a character string to a numeric type.
             The function throws a given exception if an error occurs, and false otherwise.  Note that it is
             considered a conversion error if a non-whitespace character is left unused after a numeric conversion.
      \param value_string Character string to be converted into a numeric type.
      \param value Value of the specified numeric type that the given character string represents.
      \param except Exception to throw in case of conversion errors.
  */
  template<typename NumericType>
  void convertStringToNumber(const std::string & value_string, NumericType & value, const std::exception & except) {
    // Convert the copied string to a value of a numberic type, removing trailing space to prevent spurious errors.
    std::istringstream iss(value_string);
    iss >> value >> std::ws;

    // Throws an exception if an error occurs.
    if (iss.fail() || !iss.eof()) throw except;
  }

}

namespace timeSystem {

  IntFracUtility::IntFracUtility() {}

  IntFracUtility & IntFracUtility::getUtility() {
    static IntFracUtility s_utility;
    return s_utility;
  }

  void IntFracUtility::check(long int_part, double frac_part) const {
    if ((int_part == 0 && (frac_part <= -1. || frac_part >= +1.)) ||
        (int_part >  0 && (frac_part <   0. || frac_part >= +1.)) ||
        (int_part <  0 && (frac_part <= -1. || frac_part >   0.))) {
      std::ostringstream os;
      os.precision(std::numeric_limits<double>::digits10);
      os << "Fractional part out of bounds: " << frac_part;
      throw std::runtime_error(os.str());
    }
  }

  void IntFracUtility::parse(const std::string & value_string, long & int_part, double & frac_part) const {
    // Read the number into a temporary double variable for three purposes:
    // 1) To check the format as a floating-point number expression
    // 2) To check that nothing but a literal number is in the given string (except for white spaces)
    // 3) To obtain an approximate number that the given string represents
    double value_dbl = 0.;
    std::exception except = std::runtime_error("Error in interpreting \"" + value_string + "\" as a floating-point number");
    convertStringToNumber(value_string, value_dbl, except);

    // Compute the integer part and the fractional part, and store them in temporary variables.
    long int_part_tmp = 0;
    double frac_part_tmp = 0.;
    double value_abs = std::fabs(value_dbl);
    if (value_abs < 0.5) {
      // No integer part in the significand.
      // Note: the condition for this if-branch must be somewhat loose (i.e., not "value_abs < 1."), because
      //       the variable under test (value_abs) holds only an approximate value that the given character
      //       string represents, and the conditional statement may be evaluated incorrectly due to rounding errors.
      int_part_tmp = 0;
      frac_part_tmp = value_dbl;

    } else {
      // Collect the sign and all the decimal digits in the significand, skipping white spaces.
      std::string all_signs("");
      std::string all_digits("");
      for (std::string::const_iterator itor = value_string.begin(); itor != value_string.end() &&
        ('+' == *itor || '-' == *itor || '.' == *itor || 0 != std::isdigit(*itor) || 0 != std::isspace(*itor)); ++itor) {
        if ('+' == *itor || '-' == *itor) all_signs += *itor;
        if (0 != std::isdigit(*itor)) all_digits += *itor;
      }

      // Determine the sign.
      std::string sign_string("");
      if (0 == all_signs.size()) sign_string = "+";
      else if (1 == all_signs.size()) sign_string = all_signs;
      else throw std::runtime_error("Multiple signs found in interpreting \"" + value_string + "\"");

      // Check whether at least one decimal digit is in the significand.
      if (0 == all_digits.size())
        throw std::runtime_error("No decimal digit character found in the significand of \"" + value_string + "\"");

      // Remove leading zeros from the significand.
      all_digits.erase(0, all_digits.find_first_not_of("0"));

      // Analyze the remaining digits.
      if (0 == all_digits.size()) {
        // All decimal digits in the significand are zeros.
        int_part_tmp = 0;
        frac_part_tmp = 0.;

      } else {
        // Get the significand as a number in range [0.1, 1.)
        // Note: the first digit in the digit list (all_digits) is guaranteed to be non-zero at this point.
        double significand_dbl = 0.;
        except = std::runtime_error("Error in converting the significand of \"" + value_string + "\" into a floating-point number");
        convertStringToNumber("0." + all_digits, significand_dbl, except);

        // Compute the number of digits of the integer part.
        // Note: the following computations (log10) is safe because the operand of the first term is larger than or equal to 0.5,
        //       and that of the second term is in range [0.1, 1.).  See the steps above for the value ranges.
        double num_digit_dbl = std::log10(value_abs) - std::log10(significand_dbl);
        num_digit_dbl += (num_digit_dbl > 0. ? .5 : -.5);
        int num_digit_int = static_cast<int>(num_digit_dbl);

        // Compute the integer and the fractional parts of the number that the given string represents.
        std::exception except_int = std::runtime_error("Error in computing the integer part of \"" + value_string + "\"");
        std::exception except_frac = std::runtime_error("Error in computing the fractional part of \"" + value_string + "\"");
        if (num_digit_int <= 0) {
          // No integer part in the significand.
          int_part_tmp = 0;
          frac_part_tmp = value_dbl;

        } else if (num_digit_int < static_cast<int>(all_digits.size())) {
          // Significand contains both parts.
          std::string int_part_string = sign_string + all_digits.substr(0, num_digit_int);
          convertStringToNumber(int_part_string, int_part_tmp, except_int);
          std::string frac_part_string = sign_string + "0." + all_digits.substr(num_digit_int);
          convertStringToNumber(frac_part_string, frac_part_tmp, except_frac);

        } else {
          // No fractional part in the significand.
          std::string int_part_string = sign_string + all_digits + std::string(num_digit_int - all_digits.size(), '0');
          convertStringToNumber(int_part_string, int_part_tmp, except_int);
          frac_part_tmp = 0.;
        }
      }
    }

    // Check the fractional part for the boundary, and trim the results.
    // Note: the fractional part computed above could be out of bounds due to a rounding error in string-to-double
    //       conversion, .e.g., in converting "1.99999999999999999999" which may result in the fractional part of 1.0.
    if (frac_part_tmp >= 1.) {
      frac_part_tmp = 0.;
      if (int_part_tmp < std::numeric_limits<long>::max()) {
        ++int_part_tmp;
      } else {
        throw std::runtime_error("Integer overflow in computing the integer part of \"" + value_string + "\"");
      }

    } else if (frac_part_tmp <= -1.) {
      frac_part_tmp = 0.;
      if (int_part_tmp > std::numeric_limits<long>::min()) {
        --int_part_tmp;
      } else {
        throw std::runtime_error("Integer underflow in computing the integer part of \"" + value_string + "\"");
      }
    }

    // Check consistency of the integer and the fractional parts, just in case.
    try {
      check(int_part_tmp, frac_part_tmp);
    } catch (const std::exception & x) {
      throw std::runtime_error("Error in splitting \"" + value_string + "\" into an integer part and a fractional part: " + x.what());
    }

    // Set the results.
    int_part = int_part_tmp;
    frac_part = frac_part_tmp;
  }

  std::string IntFracUtility::format(long int_part, double frac_part, std::streamsize precision) const {
    // Check the fractional part.
    const IntFracUtility & utility(IntFracUtility::getUtility());
    utility.check(int_part, frac_part);

    // Prepare a stream to store a string.
    std::ostringstream os;
    os.precision(precision);
    os.setf(std::ios::fixed);

    if (int_part == 0) {
      // Write fractional part only.
      os << frac_part;
    } else {
      // Write integer part first.
      os << int_part;

      // Write fractional part into a temporary string.
      std::ostringstream oss;
      oss.precision(precision);
      oss.setf(std::ios::fixed);
      oss << frac_part;
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

    // Return the formatted string.
    return os.str();
  }

  void IntFracUtility::split(double value_double, long & int_part, double & frac_part) const {
    // Split the input value into integer part and fractional part.
    double int_part_dbl = 0.;
    double frac_part_dbl = std::modf(value_double, &int_part_dbl);

    // Convert the days portion into an integer type.
    static double largest_integer = findLargestInteger(std::numeric_limits<long>::max());
    static double smallest_integer = findLargestInteger(std::numeric_limits<long>::min());
    if (int_part_dbl > largest_integer) {
      // Throw an exception for a value too large.
      std::ostringstream os;
      os.precision(std::numeric_limits<double>::digits10);
      os << "Integer overflow in computing the integer part of " << value_double;
      throw std::runtime_error(os.str());

    } else if (int_part_dbl < smallest_integer) {
      // Throw an exception for a value too small (i.e., large negative).
      std::ostringstream os;
      os.precision(std::numeric_limits<double>::digits10);
      os << "Integer underflow in computing the integer part of " << value_double;
      throw std::runtime_error(os.str());
    }
    int_part_dbl += (int_part_dbl > 0. ? .5 : -.5);
    long int_part_int = static_cast<long>(int_part_dbl);

    // Set the results.
    int_part = int_part_int;
    frac_part = frac_part_dbl;
  }

  double IntFracUtility::findLargestInteger(long upper_bound) const {
    // Set initial values.
    double candidate_dval = 0.;
    long candidate_ival = 0;
    long lower_bound = 0;

    // Perform a binary search for the largest integer expressible in double.
    while (upper_bound != lower_bound) {
      // Take a mid-point as a test point.
      long test_point = upper_bound - (upper_bound - lower_bound) / 2;

      // Convert types to check numerical correctness in type-conversions.
      double dval = static_cast<double>(test_point);
      long ival = static_cast<long>(dval);
      double dval_back = static_cast<double>(ival);

      // Check sanity in computations, by computing a difference to the current candidate.
      long diff_ival = ival - candidate_ival;
      double diff_dval = dval - candidate_dval;
      if (std::fabs(dval_back - dval) < 0.5 && std::fabs(diff_dval - diff_ival) < 0.5) {
        // Update candidate.
        candidate_dval = dval;
        candidate_ival = ival;

        // Update lower bound.
        lower_bound = test_point;

      } else {
        // Update upper bound.
        if (test_point != upper_bound) upper_bound = test_point;
        else if (upper_bound > lower_bound) upper_bound--;
        else if (upper_bound < lower_bound) upper_bound++;
      }
    }

    // Return the candidate at the end of search.
    return candidate_dval;
  }

}
