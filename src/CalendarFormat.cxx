/** \file CalendarFormat.cxx
    \brief Implementation of CalendarFormat and related classes.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/CalendarFormat.h"

#include <cmath>
#include <iomanip>
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
    if (elapsed_100year > 3) elapsed_100year = 3;
    residual_day -= elapsed_100year * DayPer100Year();

    // Compute which 4-year cycle of the century, and the number of days since the beginning of the 4-year cycle.
    long elapsed_4year = residual_day / DayPer4Year();
    residual_day -= elapsed_4year * DayPer4Year();

    // Compute which year of the 4-year cycle, and the number of days since the bebinning of the year.
    long elapsed_year = residual_day / DayPerYear();
    if (elapsed_year > 3) elapsed_year = 3;
    residual_day -= elapsed_year * DayPerYear();

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

namespace {

  using namespace timeSystem;

  /** \function parseIso8601Format
      \brief Helper function to help CalendarFormat, IsoWeekFormat, and OrdinalFormat classes parse a date-and-time string.
             Note that this function does NOT cover all possible combinations of date and time representations defined by
             the ISO 8601 standard. It interprets only the extended format, and requires all values (i.e., it does not
             allow omission of any value in the given string.
  */
  typedef std::vector<long> array_type;
  enum DateType { CalendarDate, IsoWeekDate, OrdinalDate, UnsupportedDate };
  DateType parseIso8601Format(const std::string & time_string, array_type & integer_value, double & double_value);

  /** \class CalendarFormat
      \brief Class to represent a calendar format of time representation.
  */
  class CalendarFormat : public TimeFormat<Calendar> {
    public:
      virtual Calendar convert(const datetime_type & datetime) const;

      virtual datetime_type convert(const Calendar & time_rep) const;

      virtual Calendar parse(const std::string & time_string) const;

      virtual std::string format(const Calendar & time_rep, std::streamsize precision = std::numeric_limits<double>::digits10) const;
  };

  /** \class IsoWeekFormat
      \brief Class to represent ISO week date and time format of time representation.
  */
  class IsoWeekFormat : public TimeFormat<IsoWeek> {
    public:
      virtual IsoWeek convert(const datetime_type & datetime) const;

      virtual datetime_type convert(const IsoWeek & time_rep) const;

      virtual IsoWeek parse(const std::string & time_string) const;

      virtual std::string format(const IsoWeek & time_rep, std::streamsize precision = std::numeric_limits<double>::digits10) const;
  };

  /** \class OrdinalFormat
      \brief Class to represent an ordinal date and time of time representation.
  */
  class OrdinalFormat : public TimeFormat<Ordinal> {
    public:
      virtual Ordinal convert(const datetime_type & datetime) const;

      virtual datetime_type convert(const Ordinal & time_rep) const;

      virtual Ordinal parse(const std::string & time_string) const;

      virtual std::string format(const Ordinal & time_rep, std::streamsize precision = std::numeric_limits<double>::digits10) const;
  };

  DateType parseIso8601Format(const std::string & time_string, array_type & integer_value, double & double_value) {
    // Separate date part and time part.
    std::string::size_type pos_sep = time_string.find('T');
    std::string date_part;
    std::string time_part;
    if (pos_sep != std::string::npos) {
      date_part = time_string.substr(0, pos_sep);
      time_part = time_string.substr(pos_sep + 1);
    } else {
      throw std::runtime_error("Missing separator (\"T\") between date and time: " + time_string);
    }

    // Split the date part into year, month, and day fields.
    std::vector<std::string> field_list;
    field_list.reserve(6);
    pos_sep = 0;
    for (std::string::size_type pos = 0; pos_sep != std::string::npos; pos = pos_sep + 1) {
      pos_sep = date_part.find('-', pos);
      std::string::size_type length = (pos_sep == std::string::npos ? pos_sep : pos_sep - pos);
      field_list.push_back(date_part.substr(pos, length));
    }

    // Determine date type: CalendarDate, IsoWeekDate, or OrdinalDate.
    DateType date_type(UnsupportedDate);
    std::vector<std::string>::size_type date_part_size = field_list.size();
    if (date_part_size > 0 && field_list[0].size() == 4) {
      if (date_part_size == 3 && field_list[1].size() == 2 && field_list[2].size() == 2) date_type = CalendarDate;
      else if (date_part_size == 3 && field_list[1].size() == 3 && field_list[2].size() == 1 && field_list[1].at(0) == 'W') {
        date_type = IsoWeekDate;
        // Remove 'W' in the field value.
        field_list[1].erase(0, 1);
      } else if (date_part_size == 2 && field_list[1].size() == 3) date_type = OrdinalDate;
      else date_type = UnsupportedDate;
    } else {
      date_type = UnsupportedDate;
    }
    if (date_type == UnsupportedDate) throw std::runtime_error("Unsupported date format: " + date_part);

    // Split the time part into hour, minute, and second fields.
    pos_sep = 0;
    for (std::string::size_type pos = 0; pos_sep != std::string::npos; pos = pos_sep + 1) {
      pos_sep = time_part.find(':', pos);
      std::string::size_type length = (pos_sep == std::string::npos ? pos_sep : pos_sep - pos);
      field_list.push_back(time_part.substr(pos, length));
    }
    if (field_list.size() != date_part_size + 3) throw std::runtime_error("Unsupported time format: " + time_part);

    // Separate a field for seconds.
    std::string sec_field = field_list.back();
    field_list.pop_back();

    // Clear the contents of the given container for integer values.
    integer_value.clear();

    // Convert year, month, day, hour, and minute fields into long variables.
    for (std::vector<std::string>::const_iterator itor = field_list.begin(); itor != field_list.end(); ++itor) {
      std::istringstream iss(*itor);
      long long_variable = 0;
      iss >> long_variable;
      if (iss.fail() || !iss.eof()) throw std::runtime_error("Cannot interpret \"" + *itor + "\" in parsing \"" + time_string + "\"");
      integer_value.push_back(long_variable);
    }

    // Convert second field into long variables.
    {
      std::istringstream iss(sec_field);
      double double_variable = 0.;
      iss >> double_variable;
      if (iss.fail() || !iss.eof()) {
        throw std::runtime_error("Cannot interpret \"" + sec_field + "\" in parsing \"" + time_string + "\"");
      }
      double_value = double_variable;
    }

    // Return the date type.
    return date_type;
  }

  Calendar CalendarFormat::convert(const datetime_type & datetime) const {
    // Convert to the ordinal date representation.
    const TimeFormat<Ordinal> & ordinal_format(TimeFormatFactory<Ordinal>::getFormat());
    Ordinal ordinal_rep = ordinal_format.convert(datetime);

    // Conmpute month and date of the ordinal date.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    long day = 0;
    long month = calendar.findMonth(ordinal_rep.m_year, ordinal_rep.m_day, day);

    // Return the result.
    return Calendar(ordinal_rep.m_year, month, day, ordinal_rep.m_hour, ordinal_rep.m_min, ordinal_rep.m_sec);
  }

  datetime_type CalendarFormat::convert(const Calendar & calendar_rep) const {
    // Convert calendar representation to ordinal date representation.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    long ordinal_date = calendar.computeOrdinalDate(calendar_rep.m_year, calendar_rep.m_mon, calendar_rep.m_day);
    Ordinal ordinal_rep(calendar_rep.m_year, ordinal_date, calendar_rep.m_hour, calendar_rep.m_min, calendar_rep.m_sec);

    // Convert to datetime_type.
    const TimeFormat<Ordinal> & ordinal_format(TimeFormatFactory<Ordinal>::getFormat());
    return ordinal_format.convert(ordinal_rep);
  }

  Calendar CalendarFormat::parse(const std::string & time_string) const {
    // Split the given string to integer and double values.
    array_type int_array;
    double dbl_value;
    DateType date_type = parseIso8601Format(time_string, int_array, dbl_value);

    // Check date_type and throw an exception if it is not CalendarDate.
    if (date_type != CalendarDate) throw std::runtime_error("Unable to recognize as a calendar date format: " + time_string);

    // Return the result.
    return Calendar(int_array[0], int_array[1], int_array[2], int_array[3], int_array[4], dbl_value);
  }

  std::string CalendarFormat::format(const Calendar & time_rep, std::streamsize precision) const {
    std::ostringstream os;
    os << std::setfill('0') << std::setw(4) << time_rep.m_year << "-" << std::setw(2) << time_rep.m_mon << "-" <<
      std::setw(2) << time_rep.m_day << "T" << std::setw(2) << time_rep.m_hour << ":" << std::setw(2) << time_rep.m_min << ":";
    os.setf(std::ios::fixed);
    if (time_rep.m_sec < 10.) os << '0';
    os << std::setprecision(precision) << time_rep.m_sec;
    return os.str();
  }

  IsoWeek IsoWeekFormat::convert(const datetime_type & datetime) const {
    // Convert to the ordinal date representation.
    const TimeFormat<Ordinal> & ordinal_format(TimeFormatFactory<Ordinal>::getFormat());
    Ordinal ordinal_rep = ordinal_format.convert(datetime);

    // Compute MJD of January 1st of the year.
    long mjd_jan1 = datetime.first - ordinal_rep.m_day + 1;

    // Compute MJD of the first day of the ISO year that is closest to January 1st of the given year.
    // Note: The first day of the ISO year is the closest Monday to January 1st of the year.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    long mjd_day1 = calendar.findNearestMonday(mjd_jan1);

    // Compute the first day of the ISO year in which the given date is.
    long iso_year = 0;
    if (datetime.first < mjd_day1) {
      // The first day of the ISO year for the given date is in the previous year.
      long mjd_jan1_prev = calendar.computeMjd(ordinal_rep.m_year - 1, 1);
      mjd_day1 = calendar.findNearestMonday(mjd_jan1_prev);
      iso_year = ordinal_rep.m_year - 1;
    } else {
      // The first day of the ISO year for the given date is in this year.
      long mjd_jan1_next = calendar.computeMjd(ordinal_rep.m_year + 1, 1);
      long mjd_day1_next = calendar.findNearestMonday(mjd_jan1_next);
      if (datetime.first < mjd_day1_next) {
        // The ISO year for the given date is the same as the calendar year for it.
        iso_year = ordinal_rep.m_year;
      } else {
        // The ISO year for the given date is the next calendar year.
        mjd_day1 = mjd_day1_next;
        iso_year = ordinal_rep.m_year + 1;
      }
    }

    // Compute ISO week number and weekday number.
    long elapsed_day = datetime.first - mjd_day1;
    long week_number = elapsed_day / 7 + 1;
    long weekday_number = elapsed_day % 7 + 1;

    // Return the result.
    return IsoWeek(iso_year, week_number, weekday_number, ordinal_rep.m_hour, ordinal_rep.m_min, ordinal_rep.m_sec);
  }

  datetime_type IsoWeekFormat::convert(const IsoWeek & iso_week_rep) const {
    // Compute date and time of January 1st of calendar year iso_week_rep.m_year.
    const TimeFormat<Ordinal> & ordinal_format(TimeFormatFactory<Ordinal>::getFormat());
    Ordinal ordinal_rep(iso_week_rep.m_year, 1, iso_week_rep.m_hour, iso_week_rep.m_min, iso_week_rep.m_sec);
    datetime_type datetime = ordinal_format.convert(ordinal_rep);

    // Add weeks and days to the result MJD.
    datetime.first += (iso_week_rep.m_week - 1) * 7 + iso_week_rep.m_day - 1;

    // Compute the number of days since January 1st of calendar year iso_week_rep.m_year until ISO year iso_week_rep.m_year.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    long mjd_jan1 = calendar.computeMjd(iso_week_rep.m_year, 1);
    long mjd_day1 = calendar.findNearestMonday(mjd_jan1);

    // Adjust the difference between calendar year and ISO year.
    datetime.first += mjd_day1 - mjd_jan1;

    // Return the result.
    return datetime;
  }

  IsoWeek IsoWeekFormat::parse(const std::string & time_string) const {
    // Split the given string to integer and double values.
    array_type int_array;
    double dbl_value;
    DateType date_type = parseIso8601Format(time_string, int_array, dbl_value);

    // Check date_type and throw an exception if it is not IsoWeekDate.
    if (date_type != IsoWeekDate) throw std::runtime_error("Unable to recognize as an ISO week date format: " + time_string);

    // Return the result.
    return IsoWeek(int_array[0], int_array[1], int_array[2], int_array[3], int_array[4], dbl_value);
  }

  std::string IsoWeekFormat::format(const IsoWeek & time_rep, std::streamsize precision) const {
    std::ostringstream os;
    os << std::setfill('0') << std::setw(4) << time_rep.m_year << "-W" << std::setw(2) << time_rep.m_week << "-" <<
      std::setw(1) << time_rep.m_day << "T" << std::setw(2) << time_rep.m_hour << ":" << std::setw(2) << time_rep.m_min << ":";
    os.setf(std::ios::fixed);
    if (time_rep.m_sec < 10.) os << '0';
    os << std::setprecision(precision) << time_rep.m_sec;
    return os.str();
  }

  Ordinal OrdinalFormat::convert(const datetime_type & datetime) const {
    // Compute the year and the ordinal date for the given MJD.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    long day = 0;
    long year = calendar.findYear(datetime.first, day);

    // Compute hours.
    long hour = long(std::floor(datetime.second / SecPerHour()) + 0.5);
    if (hour > 23) hour = 23;

    // Compute minutes.
    double residual_seconds = datetime.second - hour * SecPerHour();
    long min = long(std::floor(residual_seconds / SecPerMin()) + 0.5);
    if (min > 59) min = 59;

    // Compute seconds.
    double sec = datetime.second - hour * SecPerHour() - min * SecPerMin();

    // Return the result.
    return Ordinal(year, day, hour, min, sec);
  }

  datetime_type OrdinalFormat::convert(const Ordinal & ordinal_rep) const {
    // Compute an integer part of MJD from the given year and the ordinal date.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    long mjd_number = calendar.computeMjd(ordinal_rep.m_year, ordinal_rep.m_day);

    // Compute the number of seconds since the beginning of the day.
    double num_second = ordinal_rep.m_hour * SecPerHour() + ordinal_rep.m_min * SecPerMin() + ordinal_rep.m_sec;

    // Return the result.
    return datetime_type(mjd_number, num_second);
  }

  Ordinal OrdinalFormat::parse(const std::string & time_string) const {
    // Split the given string to integer and double values.
    array_type int_array;
    double dbl_value;
    DateType date_type = parseIso8601Format(time_string, int_array, dbl_value);

    // Check date_type and throw an exception if it is not OrdinalDate.
    if (date_type != OrdinalDate) throw std::runtime_error("Unable to recognize as an ordinal date format: " + time_string);

    // Return the result.
    return Ordinal(int_array[0], int_array[1], int_array[2], int_array[3], dbl_value);
  }

  std::string OrdinalFormat::format(const Ordinal & time_rep, std::streamsize precision) const {
    std::ostringstream os;
    os << std::setfill('0') << std::setw(4) << time_rep.m_year << "-" << std::setw(3) << time_rep.m_day << "T" <<
      std::setw(2) << time_rep.m_hour << ":" << std::setw(2) << time_rep.m_min << ":";
    os.setf(std::ios::fixed);
    if (time_rep.m_sec < 10.) os << '0';
    os << std::setprecision(precision) << time_rep.m_sec;
    return os.str();
  }

}

namespace timeSystem {

  const TimeFormat<Calendar> & TimeFormatFactory<Calendar>::getFormat() {
    static const CalendarFormat s_calendar_format;
    return s_calendar_format;
  }

  const TimeFormat<IsoWeek> & TimeFormatFactory<IsoWeek>::getFormat() {
    static const IsoWeekFormat s_iso_week_format;
    return s_iso_week_format;
  }

  const TimeFormat<Ordinal> & TimeFormatFactory<Ordinal>::getFormat() {
    static const OrdinalFormat s_ordinal_format;
    return s_ordinal_format;
  }

}
