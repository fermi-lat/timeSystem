/** \file TimeConstant.h
    \brief Declaration of globally accessible static inline functions to assist in various conversions.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_TimeConstant_h
#define timeSystem_TimeConstant_h

namespace timeSystem {

  /** \brief Functions to return unit conversion constants (declaration).
             Note that only one direction of conversions is implemented for each pair of time units.
             The conversion constant implemented here is the one expressible as an integer number.
             This is to prevent a loss of precision by pre-computing a conversion constant as a floating-point number.
             If a reverse conversion is needed, the caller of these functions should pay attention to precision issues.
             Also, they are returned as a value of an integral type, not a floating-point type such as double, because
             integral computations like modulo is necessary in some cases, and conversions from a floating-point number
             to an integral number is more complicated than the reverse of it in computation.
  */
  long SecPerMin();
  long MinPerHour();
  long HourPerDay();
  long SecPerHour();
  long MinPerDay();
  long SecPerDay();

  // Definitions of the above functions.
  inline long SecPerMin() { static long r = 60l; return r; }
  inline long MinPerHour() { static long r = 60l; return r; }
  inline long HourPerDay() { static long r = 24l; return r; }
  inline long SecPerHour() { static long r = SecPerMin() * MinPerHour(); return r; }
  inline long MinPerDay() { static long r = MinPerHour() * HourPerDay(); return r; }
  inline long SecPerDay() { static long r = SecPerMin() * MinPerHour() * HourPerDay(); return r; }
}

#endif
