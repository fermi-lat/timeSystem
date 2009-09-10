/** \file CalendarFormat.cxx
    \brief Implementation of CalendarFormat and related classes.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/CalendarFormat.h"

#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <vector>

namespace {

  using namespace timeSystem;

  /** \class GregorianCalendar
      \brief Class to perform various computations related to Gregorian calendar.
  */
  class GregorianCalendar {
    public:
      /// \brief Return a GregorianCalendar object.
      static GregorianCalendar & getCalendar();

      /** \brief Find what year a give MJD is, and return it. Also compute an ordinal date for a give MJD, and set it
                 to the second argument of the method.
          \param mjd MJD for which Gregorian year is to be determined.
          \param ordinal_date Ordinal date of a given MJD is set to this argument as a return value.
      */
      long findYear(long mjd, long & ordinal_date) const;

      /** \brief Compute an MJD number for a given combination of a year and an ordinal date, and return it.
          \param year Gregorian year to compute an MJD for.
          \param ordinal_date Ordinal date to compute an MJD for.
      */
      long computeMjd(long year, long ordinal_date) const;

      /** \brief Find which month a given date is in, and return it (1 for January).
          \param year Gregorian year to find a month for.
          \param ordinal_date Ordinal date to find a month for.
          \param day_of_month Day of the month of a given date is set to this argument as a return value.
      */
      long findMonth(long year, long ordinal_date, long & day_of_month) const;

      /** \brief Compute an ordinal date of a given calendar year, month, and day, and return it.
          \param year Calendar year to compute an ordinal date for.
          \param month Month of the year to compute an ordinal date for.
          \param day Day of the month to compute an ordinal date for.
      */
      long computeOrdinalDate(long year, long month, long day) const;

      /** \brief Find a Monday closest to a give date, and return it.
          \param mjd MJD of a date to find the closest Monday to.
      */
      long findNearestMonday(long mjd) const;

      /** \brief Check validity of Gregorian year, month, and day, and throw an exception if any problem is found.
          \param year Gregorian year to check.
          \param month Month of the year to check.
          \param day Day of the month to check.
      */
      void checkCalendarDate(long year, long month, long day) const;

      /** \brief Check validity of Gregorian year and an ordinal date, and throw an exception if any problem is found.
          \param year Gregorian year to check.
          \param day Ordinal date of the year to check.
      */
      void checkOrdinalDate(long year, long day) const;

    private:
      typedef std::vector<long> table_type;

      /// \brief Construct a GregorianCalendar object.
      GregorianCalendar();

      /// \brief Return the number of days in 400 years.
      long DayPer400Year() const;

      /// \brief Return the number of days in 100 years starting at the beginning of a century.
      long DayPer100Year() const;

      /// \brief Return the number of days in 4 years not including the beginning of a century.
      long DayPer4Year() const;

      /// \brief Return the number of days in a non-leap year.
      long DayPerYear() const;

      const table_type & DayPerMonth(long year) const;

      /// \brief Return the number of days in a week.
      long DayPerWeek() const;

      /// \brief Return the MJD number of January 1st, 2001.
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
    // Check the given month and day numbers.
    checkCalendarDate(year, month, day);
    
    // Get the table of the number of days per month.
    const table_type & day_per_month = DayPerMonth(year);

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

  void GregorianCalendar::checkCalendarDate(long year, long month, long day) const {
    // Get the table of the number of days per month.
    const table_type & day_per_month = DayPerMonth(year);

    // Check the given month number.
    long max_month = day_per_month.size();
    if (month < 1 || month > max_month) {
      std::ostringstream os;
      os << "Month number out of bounds (1-" << max_month << "): " << month;
      throw std::runtime_error(os.str());
    }

    // Check the given day number.
    long max_day = day_per_month[month - 1];
    if (day < 1 || day > max_day) {
      std::ostringstream os;
      os << "Day number out of bounds (1-" << max_day << "): " << day;
      throw std::runtime_error(os.str());
    }
  }

  void GregorianCalendar::checkOrdinalDate(long year, long day) const {
    // Compute the maximum ordinal date.
    long max_day = computeOrdinalDate(year, 12, 31);

    // Check the given ordinal date.
    if (day < 1 || day > max_day) {
      std::ostringstream os;
      os << "Ordinal date out of bounds (1-" << max_day << "): " << day;
      throw std::runtime_error(os.str());
    }
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

  /** \function checkHourMinSec
      \brief Helper function to help CalendarFormat, IsoWeekFormat, and OrdinalFormat classes check the time part.
  */
  void checkHourMinSec(long hour, long min, double sec);

  /** \class CalendarFormat
      \brief Class to represent a calendar format of time representation.
  */
  class CalendarFormat : public TimeFormat<Calendar> {
    public:
      /** \brief Convert date and time to a calendar date, and return it.
          \param datetime Date and time to convert.
      */
      virtual Calendar convert(const datetime_type & datetime) const;

      /** \brief Convert a calendar date to date and time.
          \param time_rep Calendar date to convert.
      */
      virtual datetime_type convert(const Calendar & time_rep) const;

      /** \brief Interpret a given time string as a calendar date, and return the calendar date.
          \param time_string Character string to interpret as a calendar date.
      */
      virtual Calendar parse(const std::string & time_string) const;

      /** \brief Create a character string representing a given time in a calendar date format.
          \param time_rep Calendar date to format into a character string.
          \param precision Number of digits after a decimal point in the time part of a given time.
      */
      virtual std::string format(const Calendar & time_rep, std::streamsize precision = std::numeric_limits<double>::digits10) const;
  };

  /** \class IsoWeekFormat
      \brief Class to represent ISO week date and time format of time representation.
  */
  class IsoWeekFormat : public TimeFormat<IsoWeek> {
    public:
      /** \brief Convert date and time to an ISO week date, and return it.
          \param datetime Date and time to convert.
      */
      virtual IsoWeek convert(const datetime_type & datetime) const;

      /** \brief Convert an ISO week date to date and time.
          \param time_rep ISO week date to convert.
      */
      virtual datetime_type convert(const IsoWeek & time_rep) const;

      /** \brief Interpret a given time string as an ISO week date, and return the ISO week date.
          \param time_string Character string to interpret as an ISO week date.
      */
      virtual IsoWeek parse(const std::string & time_string) const;

      /** \brief Create a character string representing a given time in an ISO week date format.
          \param time_rep ISO week date to format into a character string.
          \param precision Number of digits after a decimal point in the time part of a given time.
      */
      virtual std::string format(const IsoWeek & time_rep, std::streamsize precision = std::numeric_limits<double>::digits10) const;

    private:
      /** \brief Check validity of an ISO year, an ISO week number, and an ISO weekday number, and throw an exception
                 if any problem exists.
          \param iso_year ISO year to be tested.
          \param week_number ISO week number to be tested.
          \param weekday_number ISO weekday number to be tested.
      */
      void checkWeekDate(long iso_year, long week_number, long weekday_number) const;
  };

  /** \class OrdinalFormat
      \brief Class to represent an ordinal date and time of time representation.
  */
  class OrdinalFormat : public TimeFormat<Ordinal> {
    public:
      /** \brief Convert date and time to an ordinal date, and return it.
          \param datetime Date and time to convert.
      */
      virtual Ordinal convert(const datetime_type & datetime) const;

      /** \brief Convert an ordinal date to date and time.
          \param time_rep Ordinal date to convert.
      */
      virtual datetime_type convert(const Ordinal & time_rep) const;

      /** \brief Interpret a given time string as an ordinal date, and return the calendar date.
          \param time_string Character string to interpret as an ordinal date.
      */
      virtual Ordinal parse(const std::string & time_string) const;

      /** \brief Create a character string representing a given time in an ordinal date format.
          \param time_rep Ordinal date to format into a character string.
          \param precision Number of digits after a decimal point in the time part of a given time.
      */
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

  void checkHourMinSec(long hour, long min, double sec) {
    // Check the time part of the given time representation.
    if (hour < 0 || hour > 23) {
      std::ostringstream os;
      os << "Hours of the day out of bounds (0-23): " << hour;
      throw std::runtime_error(os.str());

    } else if (min < 0 || min > 59) {
      std::ostringstream os;
      os << "Minutes of the hour out of bounds (0-59): " << min;
      throw std::runtime_error(os.str());

    } else if (sec < 0) {
      std::ostringstream os;
      os << "Seconds of the minute out of bounds: " << sec;
      throw std::runtime_error(os.str());
    }
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

  datetime_type CalendarFormat::convert(const Calendar & time_rep) const {
    // Check the date part of the given time representation.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    calendar.checkCalendarDate(time_rep.m_year, time_rep.m_mon, time_rep.m_day);

    // Convert calendar representation to ordinal date representation.
    long ordinal_date = calendar.computeOrdinalDate(time_rep.m_year, time_rep.m_mon, time_rep.m_day);
    Ordinal ordinal_rep(time_rep.m_year, ordinal_date, time_rep.m_hour, time_rep.m_min, time_rep.m_sec);

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

    // Check the date part of the given time representation.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    calendar.checkCalendarDate(int_array[0], int_array[1], int_array[2]);

    // Check the time part of the given time representation.
    checkHourMinSec(int_array[3], int_array[4], dbl_value);

    // Return the result.
    return Calendar(int_array[0], int_array[1], int_array[2], int_array[3], int_array[4], dbl_value);
  }

  std::string CalendarFormat::format(const Calendar & time_rep, std::streamsize precision) const {
    // Check the date part of the given time representation.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    calendar.checkCalendarDate(time_rep.m_year, time_rep.m_mon, time_rep.m_day);

    // Check the time part of the given time representation.
    checkHourMinSec(time_rep.m_hour, time_rep.m_min, time_rep.m_sec);

    // Format the time into a string.
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

  datetime_type IsoWeekFormat::convert(const IsoWeek & time_rep) const {
    // Check the date part of the given time representation.
    checkWeekDate(time_rep.m_year, time_rep.m_week, time_rep.m_day);

    // Compute date and time of January 1st of calendar year time_rep.m_year.
    const TimeFormat<Ordinal> & ordinal_format(TimeFormatFactory<Ordinal>::getFormat());
    Ordinal ordinal_rep(time_rep.m_year, 1, time_rep.m_hour, time_rep.m_min, time_rep.m_sec);
    datetime_type datetime = ordinal_format.convert(ordinal_rep);

    // Add weeks and days to the result MJD.
    datetime.first += (time_rep.m_week - 1) * 7 + time_rep.m_day - 1;

    // Compute the number of days since January 1st of calendar year time_rep.m_year until ISO year time_rep.m_year.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    long mjd_jan1 = calendar.computeMjd(time_rep.m_year, 1);
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

    // Check the date part of the given time representation.
    checkWeekDate(int_array[0], int_array[1], int_array[2]);

    // Check the time part of the given time representation.
    checkHourMinSec(int_array[3], int_array[4], dbl_value);

    // Return the result.
    return IsoWeek(int_array[0], int_array[1], int_array[2], int_array[3], int_array[4], dbl_value);
  }

  std::string IsoWeekFormat::format(const IsoWeek & time_rep, std::streamsize precision) const {
    // Check the date part of the given time representation.
    checkWeekDate(time_rep.m_year, time_rep.m_week, time_rep.m_day);

    // Check the time part of the given time representation.
    checkHourMinSec(time_rep.m_hour, time_rep.m_min, time_rep.m_sec);

    // Format the time into a string.
    std::ostringstream os;
    os << std::setfill('0') << std::setw(4) << time_rep.m_year << "-W" << std::setw(2) << time_rep.m_week << "-" <<
      std::setw(1) << time_rep.m_day << "T" << std::setw(2) << time_rep.m_hour << ":" << std::setw(2) << time_rep.m_min << ":";
    os.setf(std::ios::fixed);
    if (time_rep.m_sec < 10.) os << '0';
    os << std::setprecision(precision) << time_rep.m_sec;
    return os.str();
  }

  void IsoWeekFormat::checkWeekDate(long iso_year, long week_number, long weekday_number) const {
    // Check the given weekday_number.
    if (weekday_number < 1 || weekday_number > 7) {
      std::ostringstream os;
      os << "Weekday number out of bounds (1-7): " << weekday_number;
      throw std::runtime_error(os.str());
    }

    // Compute the first day of the given ISO year.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    long mjd_jan1 = calendar.computeMjd(iso_year, 1);
    long mjd_day1 = calendar.findNearestMonday(mjd_jan1);

    // Compute the first day of the next ISO year.
    long mjd_jan1_next = calendar.computeMjd(iso_year + 1, 1);
    long mjd_day1_next = calendar.findNearestMonday(mjd_jan1_next);

    // Compute the number of weeks in this ISO year.
    long max_week = (mjd_day1_next - mjd_day1) / 7;

    // Check the given week_number.
    if (week_number < 1 || week_number > max_week) {
      std::ostringstream os;
      os << "Weekday number out of bounds (1-" << max_week << "): " << weekday_number;
      throw std::runtime_error(os.str());
    }
  }

  Ordinal OrdinalFormat::convert(const datetime_type & datetime) const {
    // Check the time part of the given date and time.
    if (datetime.second < 0.) {
      std::ostringstream os;
      os << "Negative number is given for a time of the day: " << datetime.second << " seconds of " << datetime.first << " MJD";
      throw std::runtime_error(os.str());
    }

    // Compute the year and the ordinal date for the given MJD.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    long day = 0;
    long year = calendar.findYear(datetime.first, day);

    // Compute hours.
    long hour = static_cast<long>(std::floor(datetime.second / SecPerHour()) + 0.5);
    if (hour > 23) hour = 23;

    // Compute minutes.
    double residual_seconds = datetime.second - hour * SecPerHour();
    long min = static_cast<long>(std::floor(residual_seconds / SecPerMin()) + 0.5);
    if (min > 59) min = 59;

    // Compute seconds.
    double sec = datetime.second - hour * SecPerHour() - min * SecPerMin();

    // Return the result.
    return Ordinal(year, day, hour, min, sec);
  }

  datetime_type OrdinalFormat::convert(const Ordinal & time_rep) const {
    // Check the date part of the given time representation.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    calendar.checkOrdinalDate(time_rep.m_year, time_rep.m_day);

    // Check the time part of the given time representation.
    checkHourMinSec(time_rep.m_hour, time_rep.m_min, time_rep.m_sec);

    // Compute an integer part of MJD from the given year and the ordinal date.
    long mjd_number = calendar.computeMjd(time_rep.m_year, time_rep.m_day);

    // Compute the number of seconds since the beginning of the day.
    double num_second = time_rep.m_hour * SecPerHour() + time_rep.m_min * SecPerMin() + time_rep.m_sec;

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

    // Check the date part of the given time representation.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    calendar.checkOrdinalDate(int_array[0], int_array[1]);

    // Check the time part of the given time representation.
    checkHourMinSec(int_array[2], int_array[3], dbl_value);

    // Return the result.
    return Ordinal(int_array[0], int_array[1], int_array[2], int_array[3], dbl_value);
  }

  std::string OrdinalFormat::format(const Ordinal & time_rep, std::streamsize precision) const {
    // Check the date part of the given time representation.
    const GregorianCalendar & calendar(GregorianCalendar::getCalendar());
    calendar.checkOrdinalDate(time_rep.m_year, time_rep.m_day);

    // Check the time part of the given time representation.
    checkHourMinSec(time_rep.m_hour, time_rep.m_min, time_rep.m_sec);

    // Format the time into a string.
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
