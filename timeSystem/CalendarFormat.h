/** \file CalendarFormat.h
    \brief Declaration of CalendarFormat and related classes.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_CalendarFormat_h
#define timeSystem_CalendarFormat_h

#include "timeSystem/TimeFormat.h"

#include <limits>
#include <string>

namespace timeSystem {

  /** \class Calendar
      \brief Class to hold a calendar date and time.
  */
  struct Calendar {
    Calendar(long year, long month, long day, long hour, long minute, double second): m_year(year), m_mon(month), m_day(day),
      m_hour(hour), m_min(minute), m_sec(second) {}
    long m_year;
    long m_mon;
    long m_day;
    long m_hour;
    long m_min;
    double m_sec;
  };

  /** \class IsoWeek
      \brief Class to hold an ISO week date and time.
  */
  struct IsoWeek {
    IsoWeek(long iso_year, long week_number, long weekday_number, long hour, long minute, double second): m_year(iso_year),
      m_week(week_number), m_day(weekday_number), m_hour(hour), m_min(minute), m_sec(second) {}
    long m_year;
    long m_week;
    long m_day;
    long m_hour;
    long m_min;
    double m_sec;
  };

  /** \class Ordinal
      \brief Class to hold an ordinal date and time.
  */
  struct Ordinal {
    Ordinal(long year, long day, long hour, long minute, double second): m_year(year), m_day(day), m_hour(hour), m_min(minute),
      m_sec(second) {}
    long m_year;
    long m_day;
    long m_hour;
    long m_min;
    double m_sec;
  };

  template <>
  class TimeFormatFactory<Calendar> {
    public:
      static const TimeFormat<Calendar> & getFormat();
  };

  template <>
  class TimeFormatFactory<IsoWeek> {
    public:
      static const TimeFormat<IsoWeek> & getFormat();
  };

  template <>
  class TimeFormatFactory<Ordinal> {
    public:
      static const TimeFormat<Ordinal> & getFormat();
  };

  static const TimeFormat<Calendar> & CalendarFmt(TimeFormatFactory<Calendar>::getFormat());

  static const TimeFormat<IsoWeek> & IsoWeekFmt(TimeFormatFactory<IsoWeek>::getFormat());

  static const TimeFormat<Ordinal> & OrdinalFmt(TimeFormatFactory<Ordinal>::getFormat());

}

#endif
