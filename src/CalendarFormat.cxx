/** \file CalendarFormat.cxx
    \brief Implementation of CalendarFormat and related classes.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/CalendarFormat.h"

#include <cmath>
#include <iomanip>
#include <list>
#include <sstream>
#include <vector>

namespace {

  using namespace timeSystem;

  class GregorianCalendar {
    public:
      static GregorianCalendar & getCalendar();

      long findYear(long mjd, long & ordinal_date) const;

      long computeMjd(long year, long ordinal_date) const;

      long findMonth(long year, long ordinal_date, long & day_of_month) const;

      long computeOrdinalDate(long year, long month, long day) const;

      long findNearestMonday(long mjd) const;

    private:
      typedef std::vector<long> table_type;

      GregorianCalendar();

      long DayPer400Year() const;

      long DayPer100Year() const;

      long DayPer4Year() const;

      long DayPerYear() const;

      const table_type & DayPerMonth(long year) const;

      long DayPerWeek() const;

      long MjdYear2001() const;

      table_type m_day_per_month_regular_year;
      table_type m_day_per_month_leap_year;
  };

  GregorianCalendar::GregorianCalendar() {
    // Initialize the list of the number of days in each month --- for non-leap years.
    long day_per_month_array[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    size_t array_size = sizeof(day_per_month_array)/sizeof(long);
    m_day_per_month_regular_year = table_type(day_per_month_array, day_per_month_array + array_size);

    // Initialize the list of the number of days in each month --- for leap years.
    day_per_month_array[1] += 1;
    m_day_per_month_leap_year = table_type(day_per_month_array, day_per_month_array + array_size);
  }

  GregorianCalendar & GregorianCalendar::getCalendar() {
    static GregorianCalendar s_calendar;
    return s_calendar;
  }

  long GregorianCalendar::findYear(long mjd, long & ordinal_date) const {
    // Split the given MJD into the following two parts.
    // 1) The number of days since the beginning of year 2001 until the beginning of the 400-year cycle that is
    //    the latest of those that are earlier than the beginning of the given MJD.  The result is set to residual_day.
    // 2) The number of 400-year cycles since the beginning of year 2001 until the chosen 400-year cycle.
    //    The result is set to elapsed_400year.
    long elapsed_day = mjd - MjdYear2001();
    long elapsed_400year = elapsed_day / DayPer400Year();
    long residual_day = elapsed_day % DayPer400Year();
    if (elapsed_day < 0) {
      --elapsed_400year;
      residual_day += DayPer400Year();
    }

    // Compute which century of the 400-year cycle, and the number of days since the beginning of the century.
    long elapsed_100year = residual_day / DayPer100Year();
    residual_day = residual_day % DayPer100Year();
    if (elapsed_100year > 3) elapsed_100year = 3;

    // Compute which 4-year cycle of the century, and the number of days since the beginning of the 4-year cycle.
    long elapsed_4year = residual_day / DayPer4Year();
    residual_day = residual_day % DayPer4Year();

    // Compute which year of the 4-year cycle, and the number of days since the bebinning of the year.
    long elapsed_year = residual_day / DayPerYear();
    residual_day = residual_day % DayPerYear();
    if (elapsed_year > 24) elapsed_year = 24;

    // Compute and set/return the results.  Note that an ordinal date starts with 1 (one).
    ordinal_date = residual_day + 1;
    return 2001 + elapsed_400year * 400 + elapsed_100year * 100 + elapsed_4year * 4 + elapsed_year;
  }

  long GregorianCalendar::computeMjd(long year, long ordinal_date) const {
    // Split the given year into the following two parts.
    // 1) The number of days since the beginning of year 2001 until the beginning of the 400-year cycle that is
    //    the latest of those that are earlier than the beginning of the given year.  The result is set to elapsed_day.
    // 2) The number of years since the beginning of the chosen 400-year cycle and the beginning of the given year.
    //    The result is set to residual_year.
    long elapsed_year = year - 2001;
    long elapsed_400year = elapsed_year / 400;
    long residual_year = elapsed_year % 400;
    if (elapsed_year < 0) {
      --elapsed_400year;
      residual_year += 400;
    }
    long elapsed_day = elapsed_400year * DayPer400Year();

    // Compute the nubmer of days in residual_year years.
    // Note: There is no need to add a day for a leap year that occurs every 400 years here, because that will be inserted
    //       in the last year of a 400-year cycle, which comes after the beginning of any year in the 400-year cycle.
    long residual_day = residual_year * 365 + residual_year / 4 - residual_year / 100;

    // Compute the MJD number and return it.  Note that an ordinal date starts with 1 (one).
    return MjdYear2001() + elapsed_day + residual_day + ordinal_date - 1;
  }

  long GregorianCalendar::findMonth(long year, long ordinal_date, long & day_of_month) const {
    // Get the table of the number of days per month.
    const table_type & day_per_month = DayPerMonth(year);

    // Check the given ordinal date.
    if (ordinal_date < 1) {
      std::ostringstream os;
      os << "Ordinal date out of bounds: " << ordinal_date;
      throw std::runtime_error(os.str());
    }

    // Search for the month in which the given ordinal date is.
    long month = 1;
    long residual_day = ordinal_date;
    for (table_type::const_iterator itor = day_per_month.begin(); itor != day_per_month.end(); ++itor, ++month) {
      const long & day_per_this_month(*itor);
      if (residual_day > day_per_this_month) {
        // The given date is in the next month or later.
        residual_day -= day_per_this_month;
      } else {
        // The given date is in this month.
        day_of_month = residual_day;
        return month;
      }
    }

    // Throw an exception --- the given orginal date is too large for this year.
    {
      std::ostringstream os;
      os << "Ordinal date out of bounds: " << ordinal_date;
      throw std::runtime_error(os.str());
    }
  }

  long GregorianCalendar::computeOrdinalDate(long year, long month, long day) const {
    // Get the table of the number of days per month.
    const table_type & day_per_month = DayPerMonth(year);

    // Check the given month number.
    long max_month = day_per_month.size();
    if (month < 1 || month > max_month) {
      std::ostringstream os;
      os << "Month number out of bounds (1-" << max_month << "): " << month;
      throw std::runtime_error(os.str());
    }

    // Check the given day numbers.
    long max_day = day_per_month[month - 1];
    if (day < 1 || day > max_day) {
      std::ostringstream os;
      os << "Day number out of bounds (1-" << max_day << "): " << day;
      throw std::runtime_error(os.str());
    }

    // Compute an ordinal date for the given month and day numbers.
    long ordinal_date = 0;
    for (table_type::const_iterator itor = day_per_month.begin(); itor != day_per_month.begin() + month - 1; ++itor) {
      ordinal_date += *itor;
    }
    ordinal_date += day;

    // Return the result.
    return ordinal_date;
  }

  long GregorianCalendar::findNearestMonday(long mjd) const {
    // Compute the weekday number of the given MJD.
    // Note: The weekday number is 1 for Monday, and 7 for Sunday.
    long weekday_number = (mjd + 2) % DayPerWeek() + 1;

    // Compute the first day of the ISO year that is closest to January 1st of the year.
    long mjd_monday = mjd - weekday_number + 1;
    if (weekday_number > 4) mjd_monday += DayPerWeek();

    // Return MJD of the Monday.
    return mjd_monday;
  }

  long GregorianCalendar::DayPer400Year() const {
    static const long s_num_day = DayPer100Year() * 4 + 1;
    return s_num_day;
  }

  long GregorianCalendar::DayPer100Year() const {
    static const long s_num_day = DayPer4Year() * 25 - 1;
    return s_num_day;
  }

  long GregorianCalendar::DayPer4Year() const {
    static const long s_num_day = DayPerYear() * 4 + 1;
    return s_num_day;
  }

  long GregorianCalendar::DayPerYear() const {
    static const long s_num_day = 365;
    return s_num_day;
  }

  const GregorianCalendar::table_type & GregorianCalendar::DayPerMonth(long year) const {
    bool leap_year = (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
    return (leap_year ? m_day_per_month_leap_year : m_day_per_month_regular_year);
  }

  long GregorianCalendar::DayPerWeek() const {
    static const long s_num_day = 7;
    return s_num_day;
  }

  long GregorianCalendar::MjdYear2001() const {
    static const long s_mjd_year2001 = 51910;
    return s_mjd_year2001;
  }

}

namespace timeSystem {

  template <>
  void TimeFormat::convert(const datetime_type & datetime, Calendar & calendar_rep) {
    // Convert to the ordinal date representation.
    Ordinal ordinal_rep(0, 0, 0, 0, 0.);
    convert(datetime, ordinal_rep);

    // Conmpute month and date of the ordinal date.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    calendar_rep.m_mon = calendar.findMonth(ordinal_rep.m_year, ordinal_rep.m_day, calendar_rep.m_day);

    // Copy year, hour, minute, and second of the representation to the result.
    calendar_rep.m_year = ordinal_rep.m_year;
    calendar_rep.m_hour = ordinal_rep.m_hour;
    calendar_rep.m_min = ordinal_rep.m_min;
    calendar_rep.m_sec = ordinal_rep.m_sec;
  }

  template <>
  void TimeFormat::convert(const Calendar & calendar_rep, datetime_type & datetime) {
    // Convert calendar representation to ordinal date representation.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    long ordinal_date = calendar.computeOrdinalDate(calendar_rep.m_year, calendar_rep.m_mon, calendar_rep.m_day);
    Ordinal ordinal_rep(calendar_rep.m_year, ordinal_date, calendar_rep.m_hour, calendar_rep.m_min, calendar_rep.m_sec);

    // Convert to datetime_type.
    convert(ordinal_rep, datetime);
  }

  template <>
  void TimeFormat::convert(const datetime_type & datetime, IsoWeek & iso_week_rep) {
    // Convert to the ordinal date representation.
    Ordinal ordinal_rep(0, 0, 0, 0, 0.);
    convert(datetime, ordinal_rep);

    // Compute MJD of January 1st of the year.
    long mjd_jan1 = datetime.first - ordinal_rep.m_day + 1;

    // Compute MJD of the first day of the ISO year that is closest to January 1st of the given year.
    // Note: The first day of the ISO year is the closest Monday to January 1st of the year.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    long mjd_day1 = calendar.findNearestMonday(mjd_jan1);

    // Compute the first day of the ISO year in which the given date is.
    iso_week_rep.m_year = ordinal_rep.m_year;
    if (datetime.first < mjd_day1) {
      // The first day of the ISO year for the given date is in the previous year.
      --iso_week_rep.m_year;
      mjd_jan1 = calendar.computeMjd(iso_week_rep.m_year, 1);
      mjd_day1 = calendar.findNearestMonday(mjd_jan1);
    }

    // Compute ISO week number and weekday number.
    long elapsed_day = datetime.first - mjd_day1;
    iso_week_rep.m_week = elapsed_day / 7 + 1;
    iso_week_rep.m_day = elapsed_day % 7 + 1;

    // Copy hour, minute, and second of the representation to the result.
    iso_week_rep.m_hour = ordinal_rep.m_hour;
    iso_week_rep.m_min = ordinal_rep.m_min;
    iso_week_rep.m_sec = ordinal_rep.m_sec;
  }

  template <>
  void TimeFormat::convert(const IsoWeek & iso_week_rep, datetime_type & datetime) {
    // Compute date and time of January 1st of calendar year iso_week_rep.m_year.
    Ordinal ordinal_rep(iso_week_rep.m_year, 1, iso_week_rep.m_hour, iso_week_rep.m_min, iso_week_rep.m_sec);
    convert(ordinal_rep, datetime);

    // Add weeks and days to the result MJD.
    datetime.first += (iso_week_rep.m_week - 1) * 7 + iso_week_rep.m_day - 1;

    // Compute the number of days since January 1st of calendar year iso_week_rep.m_year until ISO year iso_week_rep.m_year.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    long mjd_jan1 = calendar.computeMjd(iso_week_rep.m_year, 1);
    long mjd_day1 = calendar.findNearestMonday(mjd_jan1);

    // Adjust the difference between calendar year and ISO year.
    datetime.first += mjd_day1 - mjd_jan1;
  }

  template <>
  void TimeFormat::convert(const datetime_type & datetime, Ordinal & ordinal_rep) {
    // Compute the year and the ordinal date for the given MJD.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    ordinal_rep.m_year = calendar.findYear(datetime.first, ordinal_rep.m_day);

    // Compute hours.
    ordinal_rep.m_hour = long(std::floor(datetime.second / SecPerHour()) + 0.5);

    // Compute minutes.
    double residual_seconds = datetime.second - ordinal_rep.m_hour * SecPerHour();
    ordinal_rep.m_min = long(std::floor(residual_seconds / SecPerMin()) + 0.5);

    // Compute seconds.
    ordinal_rep.m_sec = datetime.second - ordinal_rep.m_hour * SecPerHour() - ordinal_rep.m_min * SecPerMin();
  }

  template <>
  void TimeFormat::convert(const Ordinal & ordinal_rep, datetime_type & datetime) {
    // Compute an integer part of MJD from the given year and the ordinal date.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    datetime.first = calendar.computeMjd(ordinal_rep.m_year, ordinal_rep.m_day);

    // Compute the number of seconds since the beginning of the day.
    datetime.second = ordinal_rep.m_hour * SecPerHour() + ordinal_rep.m_min * SecPerMin() + ordinal_rep.m_sec;
  }

}

namespace {

#if 0
  using namespace timeSystem;

  /** \class CalendarFormat
      \brief Class to represent a calendar format of time representation.
  */
  class CalendarFormat : public TimeFormat {
    public:
      CalendarFormat(): TimeFormat("Calendar") {}

      virtual std::string format(const datetime_type & value, std::streamsize precision = std::numeric_limits<double>::digits10) const;

      virtual datetime_type parse(const std::string & value) const;
  };

  /** \class IsoWeekFormat
      \brief Class to represent ISO week date and time format of time representation.
  */
  class IsoWeekFormat : public TimeFormat {
    public:
      IsoWeekFormat(): TimeFormat("IsoWeek") {}

      virtual std::string format(const datetime_type & value, std::streamsize precision = std::numeric_limits<double>::digits10) const;

      virtual datetime_type parse(const std::string & value) const;
  };

  /** \class OrdinalFormat
      \brief Class to represent an ordinal date and time of time representation.
  */
  class OrdinalFormat : public TimeFormat {
    public:
      OrdinalFormat(): TimeFormat("Ordinal") {}

      virtual std::string format(const datetime_type & value, std::streamsize precision = std::numeric_limits<double>::digits10) const;

      virtual datetime_type parse(const std::string & value) const;
  };

  static const CalendarFormat s_calendar_format;

  static const IsoWeekFormat s_iso_week_format;

  static const OrdinalFormat s_ordinal_format;

  std::string CalendarFormat::format(const datetime_type & value, std::streamsize precision) const {
    Calendar calendar_rep(0, 0, 0, 0, 0, 0.);
    convert(value, calendar_rep);

    std::ostringstream os;
    os << calendar_rep.m_year << "-" << calendar_rep.m_mon << "-" << calendar_rep.m_day << "T" <<
      calendar_rep.m_hour << ":" << calendar_rep.m_min << ":" << std::setprecision(precision) << calendar_rep.m_sec;
    return os.str();
  }

  datetime_type CalendarFormat::parse(const std::string & value) const {
    // Separate date part and time part.
    std::string::size_type pos_sep = value.find('T');
    std::string date_part;
    std::string time_part;
    if (pos_sep != std::string::npos) {
      date_part = value.substr(0, pos_sep);
      time_part = value.substr(pos_sep + 1);
    } else {
      throw std::runtime_error("Missing separator (\"T\") between date and time: " + value);
    }

    // Split the date part into year, month, and day fields.
    std::list<std::string> field_list;
    pos_sep = 0;
    for (std::string::size_type pos = 0; pos_sep != std::string::npos; pos = pos_sep + 1) {
      pos_sep = date_part.find('-', pos);
      field_list.push_back(date_part.substr(pos, pos_sep));
    }
    if (field_list.size() != 3) throw std::runtime_error("Unsupported date format: " + date_part);

    // Split the time part into hour, minute, and second fields.
    pos_sep = 0;
    for (std::string::size_type pos = 0; pos_sep != std::string::npos; pos = pos_sep + 1) {
      pos_sep = time_part.find(':', pos);
      field_list.push_back(date_part.substr(pos, pos_sep));
    }
    if (field_list.size() != 3) throw std::runtime_error("Unsupported time format: " + time_part);

    // Separate a field for seconds.
    std::string sec_field = field_list.back();
    field_list.pop_back();

    // Convert year, month, day, hour, and minute fields into long variables.
    std::vector<long> ymdhm;
    for (std::list<std::string>::const_iterator itor = field_list.begin(); itor != field_list.end(); ++itor) {
      std::istringstream iss(*itor);
      long value_long = 0;
      iss >> value_long;
      if (iss.fail() || !iss.eof()) throw std::runtime_error("Cannot interpret \"" + *itor + "\" in parsing \"" + value + "\"");
      ymdhm.push_back(value_long);
    }

    // Convert second field into long variables.
    double value_double = 0.;
    {
      std::istringstream iss(sec_field);
      iss >> value_double;
      if (iss.fail() || !iss.eof()) throw std::runtime_error("Cannot interpret \"" + sec_field + "\" in parsing \"" + value + "\"");
    }

    // Convert the date and time into datetime_type, and return it.
    Calendar calendar_rep(ymdhm[0], ymdhm[1], ymdhm[2], ymdhm[3], ymdhm[4], value_double);
    datetime_type datetime(0, 0.);
    convert(calendar_rep, datetime);
    return datetime;
  }

  std::string IsoWeekFormat::format(const datetime_type & value, std::streamsize precision) const {
    // TODO: Implement this method.
    return "";
  }

  datetime_type IsoWeekFormat::parse(const std::string & value) const {
    // TODO: Implement this method.
    return datetime_type(0, 0.);
  }

  std::string OrdinalFormat::format(const datetime_type & value, std::streamsize precision) const {
    // TODO: Implement this method.
    return "";
  }

  datetime_type OrdinalFormat::parse(const std::string & value) const {
    // TODO: Implement this method.
    return datetime_type(0, 0.);
  }

#endif

}
