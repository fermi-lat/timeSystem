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
  double MinPerSec();
  double SecPerMin();
  long SecPerMinLong();

  double HourPerMin();
  double MinPerHour();
  long MinPerHourLong();

  double DayPerHour();
  double HourPerDay();
  long HourPerDayLong();

  double HourPerSec();
  double SecPerHour();
  long SecPerHourLong();

  double DayPerMin();
  double MinPerDay();
  long MinPerDayLong();

  double DayPerSec();
  double SecPerDay();
  long SecPerDayLong();

  // Time system conversion constants.
  double TaiMinusTtSec();

  double TtMinusTaiSec();

  // Definitions.

  // Unit conversion constants.
  inline double MinPerSec() { static double r = 1. / SecPerMin(); return r; }
  inline double SecPerMin() { static double r = SecPerMinLong(); return r; }
  inline long SecPerMinLong() { static long r = 60l; return r; }

  inline double HourPerMin() { static double r = 1. / MinPerHour(); return r; }
  inline double MinPerHour() { static double r = MinPerHourLong(); return r; }
  inline long MinPerHourLong() { static long r = 60l; return r; }

  inline double DayPerHour() { static double r = 1. / HourPerDay(); return r; }
  inline double HourPerDay() { static double r = HourPerDayLong(); return r; }
  inline long HourPerDayLong() { static long r = 24l; return r; }

  inline double HourPerSec() { static double r = 1. / SecPerHour(); return r; }
  inline double SecPerHour() { static double r = SecPerHourLong(); return r; }
  inline long SecPerHourLong() { static long r = SecPerMinLong() * MinPerHourLong(); return r; }

  inline double DayPerMin() { static double r = 1. / MinPerDay(); return r; }
  inline double MinPerDay() { static double r = MinPerDayLong(); return r; }
  inline long MinPerDayLong() { static long r = MinPerHourLong() * HourPerDayLong(); return r; }

  inline double DayPerSec() { static double r = 1. / SecPerDay(); return r; }
  inline double SecPerDay() { static double r = SecPerDayLong(); return r; }
  inline long SecPerDayLong() { static long r = SecPerMinLong() * MinPerHourLong() * HourPerDayLong(); return r; }

  // Time system conversion constants.
  inline double TaiMinusTtSec() { static double r = -TtMinusTaiSec(); return r; }

  inline double TtMinusTaiSec() { static double r(32.184); return r; }

}

#endif
