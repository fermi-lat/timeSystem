/** \file TimeConstant.h
    \brief Declaration of globally accessible static inline functions to assist in various conversions.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_TimeConstant_h
#define timeSystem_TimeConstant_h

namespace timeSystem {

  // Unit conversion constants.
  double DayPerSec();

  double SecPerDay();

  // Time system conversion constants.
  double TaiMinusTtSec();

  double TtMinusTaiSec();

  // Definitions.

  // Unit conversion constants.
  inline double DayPerSec() { static double r = 1. / SecPerDay(); return r; }

  // TODO: use integral value?
  inline double SecPerDay() { static double r = 86400.; return r; }

  // TODO: add HourPerDay() etc.

  // Time system conversion constants.
  inline double TaiMinusTtSec() { static double r = -TtMinusTaiSec(); return r; }

  inline double TtMinusTaiSec() { static double r(32.184); return r; }

}

#endif
