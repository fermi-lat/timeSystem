/** \file TimeConstant.h
    \brief Declaration of globally accessible static inline functions to assist in various conversions.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_TimeConstant_h
#define timeSystem_TimeConstant_h

namespace timeSystem {

  // Unit conversion constants.
  // TODO: Remove functions that return double. Reasons are:
  //       1) There is a concern on potential loss of precision in computation.
  //       2) They are not often used in pulsar tools packages.
  long SecPerMin();
  long MinPerHour();
  long HourPerDay();
  long SecPerHour();
  long MinPerDay();
  long SecPerDay();

  // Time system conversion constants.
  double TaiMinusTtSec();

  double TtMinusTaiSec();

  // Definitions.

  // Unit conversion constants.
  inline long SecPerMin() { static long r = 60l; return r; }
  inline long MinPerHour() { static long r = 60l; return r; }
  inline long HourPerDay() { static long r = 24l; return r; }
  inline long SecPerHour() { static long r = SecPerMin() * MinPerHour(); return r; }
  inline long MinPerDay() { static long r = MinPerHour() * HourPerDay(); return r; }
  inline long SecPerDay() { static long r = SecPerMin() * MinPerHour() * HourPerDay(); return r; }

  // Time system conversion constants.
  inline double TaiMinusTtSec() { static double r = -TtMinusTaiSec(); return r; }

  inline double TtMinusTaiSec() { static double r(32.184); return r; }

}

#endif
