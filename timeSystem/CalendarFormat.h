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
    /** \brief Construct a Calendar object.
        \param year Calendar year.
        \param month Month of the year.
        \param day Day of the month.
        \param hour Hour of the day.
        \param minute Minute of the hour.
        \param second Second of the minute.
    */
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
    /** \brief Construct an IsoWeek object.
        \param iso_year ISO year.
        \param week_number ISO week number of the ISO year.
        \param weekday_number ISO weekday number of the ISO week.
        \param hour Hour of the day.
        \param minute Minute of the hour.
        \param second Second of the minute.
    */
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
    /** \brief Construct an Ordinal object.
        \param year Calendar year.
        \param day Ordinal date.
        \param hour Hour of the day.
        \param minute Minute of the hour.
        \param second Second of the minute.
    */
    Ordinal(long year, long day, long hour, long minute, double second): m_year(year), m_day(day), m_hour(hour), m_min(minute),
      m_sec(second) {}
    long m_year;
    long m_day;
    long m_hour;
    long m_min;
    double m_sec;
  };

  /** \class TimeFormatFactory<Calendar>
      \brief Specialized TimeFormatFactory class for Calendar class.
  */
  template <>
  class TimeFormatFactory<Calendar> {
    public:
      /// \brief Return TimeFormat class for Calendar representation.
      static const TimeFormat<Calendar> & getFormat();
  };

  /** \class TimeFormatFactory<IsoWeek>
      \brief Specialized TimeFormatFactory class for IsoWeek class.
  */
  template <>
  class TimeFormatFactory<IsoWeek> {
    public:
      /// \brief Return TimeFormat class for IsoWeek representation.
      static const TimeFormat<IsoWeek> & getFormat();
  };

  /** \class TimeFormatFactory<Ordinal>
      \brief Specialized TimeFormatFactory class for Ordinal class.
  */
  template <>
  class TimeFormatFactory<Ordinal> {
    public:
      /// \brief Return TimeFormat class for Ordinal representation.
      static const TimeFormat<Ordinal> & getFormat();
  };

  static const TimeFormat<Calendar> & CalendarFmt(TimeFormatFactory<Calendar>::getFormat());

  static const TimeFormat<IsoWeek> & IsoWeekFmt(TimeFormatFactory<IsoWeek>::getFormat());

  static const TimeFormat<Ordinal> & OrdinalFmt(TimeFormatFactory<Ordinal>::getFormat());

}

#endif
