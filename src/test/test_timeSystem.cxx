/** \file test_timeSystem.cxx
    \brief Unit test for timeSystem package.
    \author Masa Hirayama, James Peachey
*/
#include <cmath>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

#include "st_app/AppParGroup.h"
#include "st_app/StApp.h"
#include "st_app/StAppFactory.h"

#include "timeSystem/AbsoluteTime.h"
#include "timeSystem/BaryTimeComputer.h"
#include "timeSystem/CalendarFormat.h"
#include "timeSystem/Duration.h"
#include "timeSystem/ElapsedTime.h"
#include "timeSystem/EventTimeHandler.h"
#include "timeSystem/GlastTimeHandler.h"
#include "timeSystem/IntFracUtility.h"
#include "timeSystem/MjdFormat.h"
#include "timeSystem/PulsarTestApp.h"
#include "timeSystem/SourcePosition.h"
#include "timeSystem/TimeCorrectorApp.h"
#include "timeSystem/TimeFormat.h"
#include "timeSystem/TimeInterval.h"
#include "timeSystem/TimeSystem.h"

extern "C" {
#include "timeSystem/glastscorbit.h"
}

#include "tip/IFileSvc.h"
#include "tip/KeyRecord.h"
#include "tip/TipFile.h"

static const std::string s_cvs_id("$Name:  $");

using namespace st_app;
using namespace timeSystem;

/** \class TimeCorrectorAppTester
    \brief Test TimeCorrectorApp application (gtbary).
*/
class TimeCorrectorAppTester: public PulsarApplicationTester {
  public:
  /** \brief Construct a TimeCorrectorAppTester object.
      \param test_app Unit test appliction of pulsar tool package, under which this application tester is to run.
  */
  TimeCorrectorAppTester(PulsarTestApp & test_app);

  /// \brief Destruct this TimeCorrectorAppTester object.
  virtual ~TimeCorrectorAppTester() throw() {}

  /// \brief Returns an application object to be tested.
  virtual st_app::StApp * createApplication() const;

  /** \brief Return a logical true if the given header keyword is determined correct, and a logical false otherwise.
      \param keyword_name Name of the header keyword to be verified.
      \param out_keyword Header keyword taken from the output file to be verified.
      \param ref_keyword Header keyword taken from the reference file which out_keyword is checked against.
      \param error_stream Output stream for this method to put an error messages when verification fails.
  */
  virtual bool verify(const std::string & keyword_name, const tip::KeyRecord & out_keyword,
    const tip::KeyRecord & ref_keyword, std::ostream & error_stream) const;

  /** \brief Return a logical true if the given table cell is considered correct, and a logical false otherwise.
      \param column_name Name of the FITS column that the given table cell belongs to.
      \param out_cell Table cell taken from the output file to be verified.
      \param ref_cell Table cell taken from the reference file which out_cell is checked against.
      \param error_stream Output stream for this method to put an error message when verification fails.
  */
  virtual bool verify(const std::string & column_name, const tip::TableCell & out_cell, const tip::TableCell & ref_cell,
    std::ostream & error_stream) const;

  /** \brief Return a logical true if the given character string is considered correct, and a logical false otherwise.
      \param out_string Character string taken from the output file to be verified.
      \param ref_string Character string taken from the reference file which out_string is checked against.
      \param error_stream Output stream for this method to put an error message when verification fails.
  */
  virtual bool verify(const std::string & out_string, const std::string & ref_string, std::ostream & error_stream) const;
};

TimeCorrectorAppTester::TimeCorrectorAppTester(PulsarTestApp & test_app): PulsarApplicationTester("gtbary", test_app) {}

st_app::StApp * TimeCorrectorAppTester::createApplication() const {
  return new TimeCorrectorApp();
}

bool TimeCorrectorAppTester::verify(const std::string & keyword_name, const tip::KeyRecord & out_keyword,
  const tip::KeyRecord & ref_keyword, std::ostream & error_stream) const {
  // Initialize return value.
  bool verified = false;

  // Extract keyword values as character strings.
  std::string out_value = out_keyword.getValue();
  std::string ref_value = ref_keyword.getValue();

  // Compare values.
  if ("RA_NOM" == keyword_name || "DEC_NOM" == keyword_name) {
    // Require the 10th decimal point to match.
    verified = equivalent(out_value, ref_value, 1.e-10, 0.);
    if (!verified) {
      error_stream << "Coordinate " << out_value << " not equivalent to reference " << ref_value <<
        " with absolute tolerance of 1e-10 degrees.";
    }

  } else if ("TIMEZERO" == keyword_name || "TSTART" == keyword_name || "TSTOP" == keyword_name || "DATE-OBS" == keyword_name
    || "DATE-END" == keyword_name || "TIERABSO" == keyword_name) {
    // Require a match down to 10 microseconds.
    verified = equivalent(out_value, ref_value, 1.e-5, 0.);
    if (!verified) {
      error_stream << "Time value " << out_value << " not equivalent to reference " << ref_value <<
        " with absolute tolerance of 10 microseconds.";
    }

  } else if ("TIERRELA" == keyword_name) {
    // Compare as floating-point numbers.
    verified = equivalent(out_value, ref_value);
    if (!verified) error_stream << "Floating-point number " << out_value << " not close enough to reference " << ref_value;

  } else {
    // Require an exact match as character strings.
    verified = (out_value == ref_value);
    if (!verified) error_stream << "Character string \"" << out_value << "\" not identical to \"" << ref_value << "\"";
  }

  // Return the result.
  return verified;
}

bool TimeCorrectorAppTester::verify(const std::string & column_name, const tip::TableCell & out_cell,
  const tip::TableCell & ref_cell, std::ostream & error_stream) const {
  // Initialize return value.
  bool verified = false;

  // Compare columns.
  if ("TIME" == column_name || "START" == column_name || "STOP" == column_name) {
    // Extract cell values as floating-point numbers.
    double out_value;
    double ref_value;
    out_cell.get(out_value);
    ref_cell.get(ref_value);

    // Require a match down to 10 microseconds.
    verified = (std::fabs(out_value - ref_value) <= 1.e-5);
    if (!verified) {
      error_stream << std::setprecision(std::numeric_limits<double>::digits10) << "Time Value " << out_value <<
        " not equivalent to reference " << ref_value << " with absolute tolerance of 10 microseconds.";
    }

  } else {
    // Ignore other columns.
    verified = true;
  }

  // Return the result.
  return verified;
}

bool TimeCorrectorAppTester::verify(const std::string & out_string, const std::string & ref_string, std::ostream & error_stream) const {
  bool verified = (out_string == ref_string);
  if (!verified) {
    error_stream << "Line not identical to reference." <<
      std::endl << "[OUT] " << out_string << std::endl << "[REF] " << ref_string;
  }
  return verified;
}

/** \class TimeSystemTestApp
    \brief Test timeSystem package and applications in it.
*/
class TimeSystemTestApp : public PulsarTestApp {
  public:
    /// \brief Construct a TimeSystemTestApp object.
    TimeSystemTestApp();

    /// \brief Destruct this TimeSystemTestApp object.
    virtual ~TimeSystemTestApp() throw() {}

    /// \brief Do all tests.
    virtual void runTest();

    /// \brief Test Duration class.
    void testDuration();

    /// \brief Test TimeSystem class.
    void testTimeSystem();

    /// \brief Test AbsoluteTime class.
    void testAbsoluteTime();

    /// \brief Test ElapsedTime class.
    void testElapsedTime();

    /// \brief Test TimeInterval class.
    void testTimeInterval();

    /// \brief Test TimeFormat class.
    void testTimeFormat();

    /// \brief Test IntFracUtility class.
    void testIntFracUtility();

    /// \brief Test SourcePosition class.
    void testSourcePosition();

    /// \brief Test BaryTimeComputer class.
    void testBaryTimeComputer();

    /// \brief Test EventTimeHandlerFactory class.
    void testEventTimeHandlerFactory();

    /// \brief Test C functions in glastscorbit.c.
    void testglastscorbit();

    /// \brief Test GlastTimeHandler class.
    void testGlastTimeHandler();

    /// \brief Test TimeCorrectorApp class.
    void testTimeCorrectorApp();

  private:
    /** \brief Helper method for testDuration, to test getter methods of Duration class.
        \param day Day part of time duration to be tested.
        \param sec Second part of time duration to be tested.
        \param time_unit_name Character string to represent a time unit to be tested.
        \param int_part Interger part of reference value for a test result to be compared with.
        \param frac_part Fractional part of reference value for a test result to be compared with.
        \param tolerance_high Difference allowed in comparing fractional part of a test result.
        \param tolerance_low Difference allowed in comparing the sum of integral and fractional parts of a test result.
    */
    void testDurationGetter(long day, double sec, const std::string & time_unit_name, long int_part, double frac_part,
      double tolerance_high, double tolerance_low);

    /** \brief Helper method for testDuration, to test constructors of Duration class.
        \param time_unit_name Character string to represent a time unit to be tested.
        \param int_part Interger part of a time duration to be created, in the unit of time_unit_name.
        \param frac_part Fractoinal part of a time duration to be created, in the unit of time_unit_name.
        \param expected_result Reference object of Duration type for a test result to be compared with.
        \param tolerance_high Difference allowed in comparing a Duration object created from integer and fractional parts
               with a reference object given as expected_result.
        \param tolerance_high Difference allowed in comparing a Duration object created from a single floating-point value
               with a reference object given as expected_result.
    */
    void testDurationConstructor(const std::string & time_unit_name, long int_part, double frac_part, const Duration & expected_result,
    const Duration & tolerance_high, const Duration & tolerance_low);

    /** \brief Helper method for testDuration, to test comparison operators of Duration class.
        \param comparator Character string representing a comparison operator.
        \param dur1 Duration object to be placed on the left side of a comparison operator to be tested.
        \param dur2 Duration object to be placed on the right side of a comparison operator to be tested.
        \param expected_result Reference boolean value that is expected to be a result of a comparison under test.
    */
    void testOneComparison(const std::string & comparator, const Duration & dur1, const Duration & dur2,
      bool expected_result);

    /** \brief Helper method for testDuration, to test computing operators of Duration class.
        \param computation Character string representing a computing operator.
        \param dur1 Duration object to be placed on the left side of a computing operator to be tested.
        \param dur2 Duration object to be placed on the right side of a computing operator to be tested.
        \param expected_result Reference Duration object that is expected to be a result of a computation under test.
        \param tolerance Difference allowed in comparison of a test result with a reference Duration object.
    */
    void testOneComputation(const std::string & computation, const Duration & dur1, const Duration & dur2,
      const Duration & expected_result, const Duration & tolerance);

    /** \brief Helper method for testTimeSystem, to test conversions between time systems.
        \param src_system_name Name of time system from which a time moment is converted.
        \param src_moment Time moment to be converted.
        \param dest_system_name Name of time system to which a time moment is converted.
        \param expected_moment Reference time moment for a test result to be compared with.
        \param tolerance Time difference in seconds allowed in comparison of a test result with a reference time moment.
    */
    void testOneConversion(const std::string & src_system_name, const moment_type & src_moment,
      const std::string & dest_system_name, const moment_type & expected_moment, double tolerance = 1.e-9);

    /** \brief Helper method for testTimeSystem, to test time subtractions.
        \param moment1 Time moment from which a time moment is subtracted.
        \param moment2 Time moment to be subtracted from a time moment.
        \param difference Reference time difference between moment1 and moment2.
        \param difference_utc Reference time difference between moment1 and moment2, when computed in UTC system.
    */
    void testOneSubtraction(const moment_type & moment1, const moment_type & moment2, double difference, double difference_utc);

    /** \brief Helper method for testTimeSystem, to test computations of date and time.
        \param moment Time moment to be converted to date and time.
        \param datetime Reference date and time for a test result to be compared with.
        \param datetime_utc Reference date and time for a test result to be compared with, when computed in UTC system.
    */
    void testOneDateTimeComputation(const moment_type & moment, const datetime_type & datetime, const datetime_type & datetime_utc);

    /** \brief Helper method for testAbsoluteTime, to test comparison operators of AbsoluteTime class.
        \param abs_time Absolute time which points to an earliear time than later_time.
        \param later_time Absolute time which points to a later time than abs_time.
    */
    void compareAbsoluteTime(const AbsoluteTime & abs_time, const AbsoluteTime & later_time);

    /** \brief Helper method for testTimeFormat, to test converting an MJD number to a calendar date, a week date, and an ordinal date.
        \param mjd MJD number to be converted into a calendar date, a week day, and an ordinal date.
        \param calendar_year Reference calendar year to be compared with a test result.
        \param calendar_year Reference month to be compared with a test result.
        \param calendar_year Reference day of month to be compared with a test result.
        \param calendar_year Reference ISO year to be compared with a test result.
        \param calendar_year Reference ISO week number to be compared with a test result.
        \param calendar_year Reference ISO week day number to be compared with a test result.
        \param calendar_year Reference ordinal date of the calendar year to be compared with a test result.
    */
    void testOneCalendarDate(long mjd, long calendar_year, long month, long month_day, long iso_year, long week_number,
      long weekday_number, long ordinal_date);

    /** \brief Helper method for testTimeFormat, to test detection of invalid date and time.
        \param time_format Time format to be tested.
        \param datetime Date and time to be tested.
        \param time_rep_name Name of time representation under test, to be used in an error message.
        \param data_description Character string to represent this test, to be used in an error message.
        \param exception_expected Set to true if an exception is expected to be thrown. Set to false otherwise.
    */
    template <typename TimeRepType>
    void testOneBadDateTime(const TimeFormat<TimeRepType> & time_format, const datetime_type & datetime,
      const std::string & time_rep_name, const std::string & data_description, bool exception_expected = true);

    /** \brief Helper method for testTimeFormat, to test detection of invalid time representation.
        \param time_format Time format to be tested.
        \param time_rep Time representation to be tested.
        \param time_rep_name Name of time representation under test, to be used in an error message.
        \param data_description Character string to represent this test, to be used in an error message.
        \param exception_expected Set to true if an exception is expected to be thrown. Set to false otherwise.
    */
    template <typename TimeRepType>
    void testOneBadTimeRep(const TimeFormat<TimeRepType> & time_format, const TimeRepType & time_rep, const std::string & time_rep_name,
      const std::string & data_description, bool exception_expected = true);

    /** \brief Helper method for testTimeFormat, to test detection of invalid time string.
        \param time_format Time format to be tested.
        \param time_string Character string representing time to be tested.
        \param time_rep_name Name of time representation under test, to be used in an error message.
        \param data_description Character string to represent this test, to be used in an error message.
        \param exception_expected Set to true if an exception is expected to be thrown. Set to false otherwise.
    */
    template <typename TimeRepType>
    void testOneBadTimeString(const TimeFormat<TimeRepType> & time_format, const std::string & time_string,
      const std::string & time_rep_name, bool exception_expected = true);
};

TimeSystemTestApp::TimeSystemTestApp(): PulsarTestApp("timeSystem") {
  setName("test_timeSystem");
  setVersion(s_cvs_id);
}

void TimeSystemTestApp::runTest() {
  // Test Duration class.
  testDuration();

  // Test TimeSystem class and subclasses.
  testTimeSystem();

  // Test AbsoluteTime class.
  testAbsoluteTime();

  // Test ElapsedTime class.
  testElapsedTime();

  // Test TimeInterval class.
  testTimeInterval();

  // Test TimeFormat class.
  testTimeFormat();

  // Test IntFracUtility class.
  testIntFracUtility();

  // Test SourcePosition class.
  testSourcePosition();

  // Test BaryTimeComputer class.
  testBaryTimeComputer();

  // Test EventTimeHandlerFactory class.
  testEventTimeHandlerFactory();

  // Test C functions in glastscorbit.c.
  testglastscorbit();

  // Test GlastTimeHandler class.
  testGlastTimeHandler();

  // Test TimeCorrectorApp class.
  testTimeCorrectorApp();
}

void TimeSystemTestApp::testDurationGetter(long day, double sec, const std::string & time_unit_name, long int_part, double frac_part,
  double tolerance_high, double tolerance_low) {
  // Test the getter that takes a long variable, a double variable, and a time unit name.
  long result_int = 0;
  double result_frac = 0.;
  Duration(day, sec).get(time_unit_name, result_int, result_frac);
  if (!(int_part == result_int && std::fabs(frac_part - result_frac) < tolerance_high)) {
    err() << "Duration(" << day << ", " << sec << ").get(int_part, frac_part, " << time_unit_name <<
      ") returned (int_part, frac_part) = (" << result_int << ", " << result_frac << "), not (" << int_part << ", " <<
      frac_part << ") as expected." << std::endl;
  }

  // Test the getter that takes a double variable and a time unit name.
  double result_double = 0.;
  Duration(day, sec).get(time_unit_name, result_double);
  double expected_double = int_part + frac_part;
  if (std::fabs(expected_double - result_double) > tolerance_low) {
    err() << "Duration(" << day << ", " << sec << ").get(result_double, " << time_unit_name << ") returned result_double = " <<
      result_double << ", not " << expected_double << " as expected." << std::endl;
  }

  // Test the getter that takes a time unit name only.
  result_double = Duration(day, sec).get(time_unit_name);
  expected_double = int_part + frac_part;
  if (std::fabs(expected_double - result_double) > tolerance_low) {
    err() << "Duration(" << day << ", " << sec << ").get(" << time_unit_name << ") returned " <<
      result_double << ", not " << expected_double << " as expected." << std::endl;
  }
}

void TimeSystemTestApp::testDurationConstructor(const std::string & time_unit_name, long int_part, double frac_part,
  const Duration & expected_result, const Duration & tolerance_high, const Duration & tolerance_low) {
  // Test the constructor that takes a pair of long and double variables.
  Duration result(int_part, frac_part, time_unit_name);
  if (!result.equivalentTo(expected_result, tolerance_high)) {
    err() << "Duration(" << int_part << ", " << frac_part << ", \"" << time_unit_name <<
      "\") created Duration of " << result << ", not equivalent to Duration of " << expected_result <<
      " with tolerance of " << tolerance_high << "." << std::endl;
  }

  // Test the constructor that takes a double variable.
  result = Duration(int_part + frac_part, time_unit_name);
  if (!result.equivalentTo(expected_result, tolerance_low)) {
    err() << "Duration(" << int_part + frac_part << ", \"" << time_unit_name <<
      "\") created Duration of " << result << ", not equivalent to Duration of " << expected_result <<
      " with tolerance of " << tolerance_low << "." << std::endl;
  }
}

void TimeSystemTestApp::testOneComparison(const std::string & comparator, const Duration & dur1, const Duration & dur2,
  bool expected_result) {
  bool result;
  if      ("!=" == comparator) result = (dur1 != dur2);
  else if ("==" == comparator) result = (dur1 == dur2);
  else if ("<"  == comparator) result = (dur1 <  dur2);
  else if ("<=" == comparator) result = (dur1 <= dur2);
  else if (">"  == comparator) result = (dur1 >  dur2);
  else if (">=" == comparator) result = (dur1 >= dur2);
  else return;
  std::string result_string = (result ? "true" : "false");
  std::string expected_result_string = (expected_result ? "true" : "false");
  if (result != expected_result) {
    err() << "Comparison Duration(" << dur1 << ") " << comparator << " Duration(" << dur2 << ") returned " <<
      result_string << ", not " << expected_result_string << " as expected." << std::endl;
  }
}

void TimeSystemTestApp::testOneComputation(const std::string & computation, const Duration & dur1, const Duration & dur2,
  const Duration & expected_result, const Duration & tolerance) {
  Duration result;
  if      ("+"  == computation) { result = dur1 + dur2; }
  else if ("+=" == computation) { result = dur1; result += dur2; }
  else if ("-"  == computation) { result = dur1 - dur2; }
  else if ("-=" == computation) { result = dur1; result -= dur2; }
  else if ("u-" == computation) { result = -dur1; }
  else return;
  if (!result.equivalentTo(expected_result, tolerance)) {
    if ("u-" == computation) {
      err() << "Operation -Duration(" << dur1 << ")" <<
        " returned Duration(" << result << "), not equivalent to Duration(" << expected_result <<
        ") with tolerance of " << tolerance << "." << std::endl;
    } else {
      err() << "Operator Duration("<< dur1 << ") " << computation << " Duration(" << dur2 << ")" <<
        " returned Duration(" << result << "), not equivalent to Duration(" << expected_result <<
        ") with tolerance of " << tolerance << "." << std::endl;
    }
  }
}

void TimeSystemTestApp::testDuration() {
  setMethod("testDuration");

  // Set the smallest number of seconds that can be correctly expressed by a Duration object.
  double tol_sec = std::numeric_limits<double>::epsilon() * 10. * 86400.;

  // Set the smallest difference in the unit of days expressible by a single number that expresses 6 days.
  double tol_6day = std::numeric_limits<double>::epsilon() * 10. * 6.;

  // Set the smallest difference in the unit of days expressible by a single number that expresses 1 day.
  double tol_1day = std::numeric_limits<double>::epsilon() * 10. * 1.;

  // Set the smallest difference in the unit of days expressible by a single number that expresses 6 days and 6 seconds.
  double tol_6day6sec = std::numeric_limits<double>::epsilon() * 10. * (6. + 6./86400.);

  // For tests of Duration getters for duration of +6 days.
  testDurationGetter(6, 0., "Day",  6,         0., tol_sec / 86400., tol_6day);
  testDurationGetter(6, 0., "Hour", 6 * 24,    0., tol_sec / 3600.,  tol_6day * 24.);
  testDurationGetter(6, 0., "Min",  6 * 1440,  0., tol_sec / 60.,    tol_6day * 1440.);
  testDurationGetter(6, 0., "Sec",  6 * 86400, 0., tol_sec,          tol_6day * 86400.);

  // For tests of Duration::getters for duration of +6 seconds.
  testDurationGetter(0, 6., "Day",  0, 6. / 86400., tol_sec / 86400., tol_1day);
  testDurationGetter(0, 6., "Hour", 0, 6. / 3600.,  tol_sec / 3600.,  tol_1day * 24.);
  testDurationGetter(0, 6., "Min",  0, 6. / 60.,    tol_sec / 60.,    tol_1day * 1440.);
  testDurationGetter(0, 6., "Sec",  6, 0.,          tol_sec,          tol_1day * 86400.);

  // For tests of Duration::getters for duration of +6 days +6 seconds.
  testDurationGetter(6, 6., "Day",  6,             6. / 86400., tol_sec / 86400., tol_6day6sec);
  testDurationGetter(6, 6., "Hour", 6 * 24,        6. / 3600.,  tol_sec / 3600.,  tol_6day6sec * 24.);
  testDurationGetter(6, 6., "Min",  6 * 1440,      6. / 60.,    tol_sec / 60.,    tol_6day6sec * 1440.);
  testDurationGetter(6, 6., "Sec",  6 * 86400 + 6, 0.,          tol_sec,          tol_6day6sec * 86400.);

  // For tests of Duration getters for duration of -6 days.
  testDurationGetter(-6, 0., "Day",  -6,         0., tol_sec / 86400., tol_6day);
  testDurationGetter(-6, 0., "Hour", -6 * 24,    0., tol_sec / 3600.,  tol_6day * 24.);
  testDurationGetter(-6, 0., "Min",  -6 * 1440,  0., tol_sec / 60.,    tol_6day * 1440.);
  testDurationGetter(-6, 0., "Sec",  -6 * 86400, 0., tol_sec,          tol_6day * 86400.);

  // For tests of Duration::getters for duration of -6 seconds.
  testDurationGetter(0, -6., "Day",   0, -6. / 86400., tol_sec / 86400., tol_1day);
  testDurationGetter(0, -6., "Hour",  0, -6. / 3600.,  tol_sec / 3600.,  tol_1day * 24.);
  testDurationGetter(0, -6., "Min",   0, -6. / 60.,    tol_sec / 60.,    tol_1day * 1440.);
  testDurationGetter(0, -6., "Sec",  -6,  0.,          tol_sec,          tol_1day * 86400.);

  // For tests of Duration::getters for duration of -6 days -6 seconds.
  testDurationGetter(-6, -6., "Day",  -6,             -6. / 86400., tol_sec / 86400., tol_6day6sec);
  testDurationGetter(-6, -6., "Hour", -6 * 24,        -6. / 3600.,  tol_sec / 3600.,  tol_6day6sec * 24.);
  testDurationGetter(-6, -6., "Min",  -6 * 1440,      -6. / 60.,    tol_sec / 60.,    tol_6day6sec * 1440.);
  testDurationGetter(-6, -6., "Sec",  -6 * 86400 - 6,  0.,          tol_sec,          tol_6day6sec * 86400.);

  // Tests of constructors.
  long int_part = 3456789;
  double frac_part = .56789567895678956789;
  Duration tol_high(0, 1.e-9); // 1 nanosecond.
  Duration tol_low(0, 1.e-3); // 1 millisecond.
  testDurationConstructor("Day", int_part, frac_part, Duration(int_part, frac_part*86400.), tol_high, tol_low);
  testDurationConstructor("Hour", int_part, frac_part, Duration(int_part/24, (int_part%24 + frac_part)*3600.), tol_high, tol_low);
  testDurationConstructor("Min", int_part, frac_part, Duration(int_part/1440, (int_part%1440 + frac_part)*60.), tol_high, tol_low);
  testDurationConstructor("Sec", int_part, frac_part, Duration(int_part/86400, int_part%86400 + frac_part), tol_high, tol_low);

  // Tests of equality and inequality operators.
  Duration six_sec(0, 6.);
  Duration seven_sec(0, 7.);
  if (six_sec != seven_sec) {
    // Should be true.
  } else {
    err() << "After Duration seven_sec(0, 7.), operator != returned false when comparing " << six_sec <<
      " to " << seven_sec << std::endl;
  }

  if (six_sec == seven_sec) {
    err() << "After Duration seven_sec(0, 7.), operator == returned true when comparing " << six_sec <<
      " to " << seven_sec << std::endl;
  } else {
    // Should be true.
  }

  Duration about_seven(0, 7.1);
  // Make comparisons which fail.
  Duration tight_tol(0, .099999);
  if (about_seven.equivalentTo(seven_sec, tight_tol))
    err() << "After Duration about_seven(0, 7.1), about_seven.equivalentTo returned true for " << seven_sec <<
      " with tolerance of " << tight_tol << ", not false as expected." << std::endl;
  if (seven_sec.equivalentTo(about_seven, tight_tol))
    err() << "After Duration seven_sec(0, 7.), seven_sec.equivalentTo returned true for " << about_seven <<
      " with tolerance of " << tight_tol << ", not false as expected." << std::endl;

  // Make comparisons which succeed.
  Duration loose_tol(0, .1);
  if (!about_seven.equivalentTo(seven_sec, loose_tol))
    err() << "After Duration about_seven(0, 7.1), about_seven.equivalentTo returned false for " << seven_sec <<
      " with tolerance of " << loose_tol << ", not true as expected." << std::endl;
  if (!seven_sec.equivalentTo(about_seven, loose_tol))
    err() << "After Duration seven_sec(0, 7.1), seven_sec.equivalentTo returned false for " << about_seven <<
      " with tolerance of " << loose_tol << ", not true as expected." << std::endl;

  // Test of the constructor that takes the integer and the fractional parts for detection of bad fractional parts.
  try {
    Duration(+1, -0.1, "Day");
    err() << "Duration constructor did not throw an exception for Duration(+1, -0.1, \"Day\")" << std::endl;
  } catch (const std::exception &) {
  }
  try {
    Duration(+1, +1.0, "Day");
    err() << "Duration constructor did not throw an exception for Duration(+1, +1.0, \"Day\")" << std::endl;
  } catch (const std::exception &) {
  }
  try {
    Duration(-1, +0.1, "Day");
    err() << "Duration constructor did not throw an exception for Duration(-1, +0.1, \"Day\")" << std::endl;
  } catch (const std::exception &) {
  }
  try {
    Duration(-1, -1.0, "Day");
    err() << "Duration constructor did not throw an exception for Duration(-1, -1.0, \"Day\")" << std::endl;
  } catch (const std::exception &) {
  }
  try {
    Duration(0, +1.0, "Day");
    err() << "Duration constructor did not throw an exception for Duration(0, +1.0, \"Day\")" << std::endl;
  } catch (const std::exception &) {
  }
  try {
    Duration(0, -1.0, "Day");
    err() << "Duration constructor did not throw an exception for Duration(0, -1.0, \"Day\")" << std::endl;
  } catch (const std::exception &) {
  }

  // Test for detections of overflow/underflow: the basic constructor that takes days and seconds.
  double large_day = std::numeric_limits<long>::max() * 0.9;
  double small_day = std::numeric_limits<long>::min() * 0.9;
  double overflow_day = std::numeric_limits<long>::max() * 1.1;
  double underflow_day = std::numeric_limits<long>::min() * 1.1;

  // --- Case which should *not* overflow, but is close to overflowing.
  double sec = large_day * SecPerDay();
  try {
    Duration(0, sec);
  } catch (const std::exception & x) {
    err() << "Duration constructor threw an exception for " << sec << " seconds unexpectedly: " << x.what() << std::endl;
  }
  // --- Case which should *not* underflow, but is close to underflowing.
  sec = small_day * SecPerDay();
  try {
    Duration(0, sec);
  } catch (const std::exception & x) {
    err() << "Duration constructor threw an exception for " << sec << " seconds unexpectedly: " << x.what() << std::endl;
  }
  // --- Case which should overflow.
  sec = overflow_day * SecPerDay();
  try {
    Duration(0, sec);
    err() << "Duration constructor did not throw an exception for " << sec << " seconds." << std::endl;
  } catch (const std::exception &) {
  }
  // --- Case which should underflow.
  sec = underflow_day * SecPerDay();
  try {
    Duration(0, sec);
    err() << "Duration constructor did not throw an exception for " << sec << " seconds." << std::endl;
  } catch (const std::exception &) {
  }

  // Test for detections of overflow/underflow: the constructors that take a single number and a time unit.
  std::map<std::string, long> unit_per_day;
  unit_per_day["Day"] = 1;
  unit_per_day["Hour"] = HourPerDay();
  unit_per_day["Min"] = MinPerDay();
  unit_per_day["Sec"] = SecPerDay();
  for (std::map<std::string, long>::const_iterator itor = unit_per_day.begin(); itor != unit_per_day.end(); ++itor) {
    const std::string & time_unit_name(itor->first);
    long conversion_factor = itor->second;

    // --- Case which should *not* overflow, but is close to overflowing.
    double target_time = large_day * conversion_factor;
    try {
      Duration(target_time, time_unit_name);
    } catch (const std::exception & x) {
      err() << "Duration constructor threw an exception for Duration(" << target_time << ", \"" << time_unit_name <<
        "\") unexpectedly: " << x.what() << std::endl;
    }

    // --- Case which should *not* underflow, but is close to underflowing.
    target_time = small_day * conversion_factor;
    try {
      Duration(target_time, time_unit_name);
    } catch (const std::exception & x) {
      err() << "Duration constructor threw an exception for Duration(" << target_time << ", \"" << time_unit_name <<
        "\") unexpectedly: " << x.what() << std::endl;
    }

    // --- Case which should overflow.
    target_time = overflow_day * conversion_factor;
    try {
      Duration(target_time, time_unit_name);
      err() << "Duration constructor did not throw an exception for Duration(" << target_time << ", \"" << time_unit_name <<
        "\")." << std::endl;
    } catch (const std::exception &) {
    }

    // --- Case which should underflow.
    target_time = underflow_day * conversion_factor;
    try {
      Duration(target_time, time_unit_name);
      err() << "Duration constructor did not throw an exception for Duration(" << target_time << ", \"" << time_unit_name <<
        "\")." << std::endl;
    } catch (const std::exception &) {
    }
  }

  // Test for detections of overflow/underflow: the getters that computes a pair of integer and fractional parts.
  double large_number = std::numeric_limits<long>::max() * 0.9;
  double small_number = std::numeric_limits<long>::min() * 0.9;
  double overflow_number = std::numeric_limits<long>::max() * 1.1;
  double underflow_number = std::numeric_limits<long>::min() * 1.1;

  // Note: Impossible to test "Day" because it is impossible to set a value that cannot be read out in units of day.
  std::map<std::string, long> sec_per_unit;
  sec_per_unit["hours"] = SecPerHour();
  sec_per_unit["minutes"] = SecPerMin();
  sec_per_unit["seconds"] = 1;
  for (std::map<std::string, long>::const_iterator itor = sec_per_unit.begin(); itor != sec_per_unit.end(); ++itor) {
    const std::string & time_unit_string(itor->first);
    long conversion_factor = itor->second;
    long int_part = 0;
    double frac_part = 0.;

    // --- Case which should *not* overflow, but is close to overflowing.
    Duration dur(0, large_number * conversion_factor);
    try {
      dur.get(time_unit_string, int_part, frac_part);
    } catch (const std::exception & x) {
      err() << "Duration getter that computes an integer-fraction pair threw an exception for a time duration of " <<
        large_number << " " << time_unit_string << " unexpectedly: " << x.what() << std::endl;
    }

    // --- Case which should *not* underflow, but is close to underflowing.
    dur = Duration(0, small_number * conversion_factor);
    try {
      dur.get(time_unit_string, int_part, frac_part);
    } catch (const std::exception & x) {
      err() << "Duration getter that computes an integer-fraction pair threw an exception for a time duration of " <<
        small_number << " " << time_unit_string << " unexpectedly: " << x.what() << std::endl;
    }

    // --- Case which should overflow.
    dur = Duration(0, overflow_number * conversion_factor);
    try {
      dur.get(time_unit_string, int_part, frac_part);
      err() << "Duration getter that computes an integer-fraction pair did not throw an exception for a time duration of " <<
        overflow_number << " " << time_unit_string << "." << std::endl;
    } catch (const std::exception &) {
    }

    // --- Case which should underflow.
    dur = Duration(0, underflow_number * conversion_factor);
    try {
      dur.get(time_unit_string, int_part, frac_part);
      err() << "Duration getter that computes an integer-fraction pair did not throw an exception for a time duration of " <<
        underflow_number << " " << time_unit_string << "." << std::endl;
    } catch (const std::exception &) {
    }
  }

  // Test for NO detections of overflow/underflow: the getters that computes a single number.
  Duration large_dur(0, large_day * SecPerDay());
  Duration small_dur(0, small_day * SecPerDay());

  std::list<std::string> time_unit_list;
  time_unit_list.push_back("Day");
  time_unit_list.push_back("Hour");
  time_unit_list.push_back("Min");
  time_unit_list.push_back("Sec");
  for (std::list<std::string>::const_iterator itor = time_unit_list.begin(); itor != time_unit_list.end(); ++itor) {
    const std::string & time_unit(*itor);
    double time_value;

    try {
      large_dur.get(time_unit, time_value);
    } catch (const std::exception & x) {
      err() << "Duration getter that computes a single number threw an exception in getting a time duration of " <<
        large_day << " days for time unit \"" << time_unit << "\" unexpectedly: " << x.what() << std::endl;
    }

    try {
      time_value = large_dur.get(time_unit);
    } catch (const std::exception & x) {
      err() << "Duration getter that returns a single number threw an exception in getting a time duration of " <<
        large_day << " days for time unit \"" << time_unit << "\" unexpectedly: " << x.what() << std::endl;
    }

    try {
      small_dur.get(time_unit, time_value);
    } catch (const std::exception & x) {
      err() << "Duration getter that computes a single number threw an exception in getting a time duration of " <<
        small_day << " days for time unit \"" << time_unit << "\" unexpectedly: " << x.what() << std::endl;
    }

    try {
      time_value = small_dur.get(time_unit);
    } catch (const std::exception & x) {
      err() << "Duration getter that returns a single number threw an exception in getting a time duration of " <<
        small_day << " days for time unit \"" << time_unit << "\" unexpectedly: " << x.what() << std::endl;
    }
  }

  // Test for detections of overflow/underflow: addition and subtraction.
  // Note: Negation (unary minus operator) cannot be tested because it is impossible to set a value that would cause
  //       overflow/underflow by negation, in a computer system where min_long == -max_long - 1.  The condition happens
  //       to match the current implementation of Duration class that keeps its day part as one less than the integer
  //       part of it for computational advantages.
  large_dur = Duration(std::numeric_limits<long>::max(), SecPerDay() - 1.);
  try {
    large_dur + Duration(0, 2.);
    err() << "Adding Duration(0, 2.) to a time duration of " << large_day << " days did not throw an exception." << std::endl;
  } catch (const std::exception &) {
  }
  small_dur = Duration(std::numeric_limits<long>::min(), 1.);
  try {
    small_dur - Duration(0, 2.);
    err() << "Subtracting Duration(0, 2.) from a time duration of " << small_day << " days did not throw an exception." << std::endl;
  } catch (const std::exception &) {
  }

  // Test for NO detections of overflow/underflow for close cases: addition and subtraction.
  large_dur = Duration(std::numeric_limits<long>::max(), SecPerDay() - 2.);
  try {
    large_dur + Duration(0, 1.);
  } catch (const std::exception &) {
    err() << "Adding Duration(0, 1.) to a time duration of " << large_day << " days threw an exception." << std::endl;
  }
  small_dur = Duration(std::numeric_limits<long>::min(), 2.);
  try {
    small_dur - Duration(0, 1.);
  } catch (const std::exception &) {
    err() << "Subtracting Duration(0, 1.) from a time duration of " << small_day << " days threw an exception." << std::endl;
  }

  // Test for NO detections of overflow/underflow for tricky cases: addition and subtraction.
  large_dur = Duration(std::numeric_limits<long>::max(), SecPerDay() - 2.);
  try {
    large_dur - Duration(-1, SecPerDay() - 1.);
  } catch (const std::exception &) {
    err() << "Adding Duration(-1, 86399.) to a time duration of " << large_day << " days did not throw an exception." << std::endl;
  }
  small_dur = Duration(std::numeric_limits<long>::min(), 2.);
  try {
    small_dur + Duration(-1, SecPerDay() - 1.);
  } catch (const std::exception &) {
    err() << "Subtracting Duration(-1, 86399.) from a time duration of " << small_day << " days threw an exception." << std::endl;
  }

  // Test comparison operators: !=, ==, <, <=, >, and >=.
  std::list<std::pair<Duration, int> > test_input;
  Duration dur0(234, 345.678);
  test_input.push_back(std::make_pair(Duration(123, 234.567), -1));
  test_input.push_back(std::make_pair(Duration(123, 345.678), -1));
  test_input.push_back(std::make_pair(Duration(123, 456.789), -1));
  test_input.push_back(std::make_pair(Duration(234, 234.567), -1));
  test_input.push_back(std::make_pair(Duration(234, 345.678),  0));
  test_input.push_back(std::make_pair(Duration(234, 456.789), +1));
  test_input.push_back(std::make_pair(Duration(345, 234.567), +1));
  test_input.push_back(std::make_pair(Duration(345, 345.678), +1));
  test_input.push_back(std::make_pair(Duration(345, 456.789), +1));

  for (std::list<std::pair<Duration, int> >::iterator itor = test_input.begin(); itor != test_input.end(); itor++) {
    testOneComparison("!=", itor->first, dur0, (itor->second != 0));
    testOneComparison("==", itor->first, dur0, (itor->second == 0));
    testOneComparison("<",  itor->first, dur0, (itor->second <  0));
    testOneComparison("<=", itor->first, dur0, (itor->second <= 0));
    testOneComparison(">",  itor->first, dur0, (itor->second >  0));
    testOneComparison(">=", itor->first, dur0, (itor->second >= 0));
  }

  // Test computation operators: +, +=, -, -=, and unary .-
  Duration dur1(321, 654.321);
  Duration dur2(123, 123.456);
  Duration tolerance(0, 1.e-9); // 1 nanosecond.
  testOneComputation("+",  dur1, dur2, Duration( 444,   777.777), tolerance);
  testOneComputation("+=", dur1, dur2, Duration( 444,   777.777), tolerance);
  testOneComputation("-",  dur1, dur2, Duration( 198,   530.865), tolerance);
  testOneComputation("-=", dur1, dur2, Duration( 198,   530.865), tolerance);
  testOneComputation("u-", dur1, dur2, Duration(-322, 85745.679), tolerance);

  // Test proper handling of small difference in second part when two Duration's are added.
  double epsilon = std::numeric_limits<double>::epsilon() * 10.;
  std::string result = (Duration(0, 86399.) + Duration(0, 1. - epsilon)).describe();
  std::string expected("Duration(1, 0)");
  if (result != expected) {
    err() << "Duration(0, 86399.) + Duration(0, 1. - " << epsilon << ") returned " << result << ", not " << expected <<
      " as expected." << std::endl;
  }
  result = (Duration(0, 1. - epsilon) + Duration(0, 86399.)).describe();
  if (result != expected) {
    err() << "Duration(0, 1. - " << epsilon <<") + Duration(0, 86399.) returned " << result << ", not " << expected <<
      " as expected." << std::endl;
  }

  // Test operator /.
  double quotient = dur1 / dur2;
  double expected_quotient = 2.60978735011036818488;

  if (std::fabs(quotient / expected_quotient - 1.) > epsilon) {
    err() << "Operator Duration(" << dur1 << ") / Duration(" << dur2 << ") returned " << quotient << ", not " <<
      expected_quotient << ", as expected." << std::endl;
  }

  // Test limits of addition and subtraction.
  Duration one(1, 0.);
  const Duration & zero(Duration::zero());
  Duration min(std::numeric_limits<long>::min(), 0.);
  Duration max(std::numeric_limits<long>::max(), 0.);
  Duration max_minus_one(std::numeric_limits<long>::max() - 1, 0.);
  Duration min_plus_one(std::numeric_limits<long>::min() + 1, 0.);

  // Additive identity.
  testOneComputation("+",  max, zero, max, tolerance);
  testOneComputation("+",  zero, max, max, tolerance);
  testOneComputation("+",  min, zero, min, tolerance);
  testOneComputation("+",  zero, min, min, tolerance);
  testOneComputation("-",  max, zero, max, tolerance);
  testOneComputation("-",  min, zero, min, tolerance);

  // Test addition of two numbers adding up to max.
  testOneComputation("+",  max_minus_one, one, max, tolerance);
  testOneComputation("+",  one, max_minus_one, max, tolerance);
  testOneComputation("+",  min, one, min_plus_one, tolerance);
  testOneComputation("+",  one, min, min_plus_one, tolerance);
  testOneComputation("-",  max, one, max_minus_one, tolerance);
  testOneComputation("-",  min_plus_one, one, min, tolerance);

  // Test printing the positive time duration.
  Duration positive_duration(12, 34567.89);
  std::string result_string;
  {
    std::ostringstream os;
    os << positive_duration;
    result_string = os.str();
  }
  std::string expected_string = "12 days 34567.89 seconds";
  if (expected_string != result_string) {
    err() << "Duration object wrote \"" << result_string << "\", not \"" << expected_string <<
    "\" as expected." << std::endl;
  }

  // Test printing the negative time duration.
  {
    std::ostringstream os;
    os << -positive_duration;
    result_string = os.str();
  }
  expected_string = "-12 days -34567.89 seconds";
  if (expected_string != result_string) {
    err() << "Duration object wrote \"" << result_string << "\", not \"" << expected_string <<
    "\" as expected." << std::endl;
  }

  // Test printing the time duration with zero length.
  {
    std::ostringstream os;
    os << zero;
    result_string = os.str();
  }
  expected_string = "0 seconds";
  if (expected_string != result_string) {
    err() << "Duration object wrote \"" << result_string << "\", not \"" << expected_string <<
    "\" as expected." << std::endl;
  }

  // Test printing the positive time duration, less than 1 day.
  positive_duration = Duration(0, 34567.89);
  {
    std::ostringstream os;
    os << positive_duration;
    result_string = os.str();
  }
  expected_string = "34567.89 seconds";
  if (expected_string != result_string) {
    err() << "Duration object wrote \"" << result_string << "\", not \"" << expected_string <<
    "\" as expected." << std::endl;
  }

  // Test printing the negative time duration, less than 1 day.
  {
    std::ostringstream os;
    os << -positive_duration;
    result_string = os.str();
  }
  expected_string = "-34567.89 seconds";
  if (expected_string != result_string) {
    err() << "Duration object wrote \"" << result_string << "\", not \"" << expected_string <<
    "\" as expected." << std::endl;
  }
}

void TimeSystemTestApp::testOneConversion(const std::string & src_system_name, const moment_type & src_moment,
  const std::string & dest_system_name, const moment_type & expected_moment, double tolerance) {
  const TimeSystem & src_sys(TimeSystem::getSystem(src_system_name));
  const TimeSystem & dest_sys(TimeSystem::getSystem(dest_system_name));
  moment_type dest_moment = dest_sys.convertFrom(src_sys, src_moment);
  Duration tol_dur(tolerance, "Sec");
  if (dest_moment.first != expected_moment.first || !expected_moment.second.equivalentTo(dest_moment.second, tol_dur)) {
    err() << "Converting from " << src_sys << " to " << dest_sys << ", moment_type(" << src_moment.first << ", " <<
      src_moment.second << ") was converted to moment_type(" << dest_moment.first << ", " << dest_moment.second <<
      "), not equivalent to moment_type(" << expected_moment.first << ", " << expected_moment.second << ") with tolerance of " <<
      tol_dur << "." << std::endl;
  }
}

void TimeSystemTestApp::testOneSubtraction(const moment_type & moment1, const moment_type & moment2, double difference,
  double difference_utc) {
  std::map<std::string, Duration> expected_diff;
  expected_diff["TAI"] = Duration(difference, "Sec");
  expected_diff["TDB"] = Duration(difference, "Sec");
  expected_diff["TT"]  = Duration(difference, "Sec");
  expected_diff["UTC"] = Duration(difference_utc, "Sec");

  Duration tolerance(1.e-9, "Sec"); // 1 nanosecond.

  for (std::map<std::string, Duration>::iterator itor_exp = expected_diff.begin(); itor_exp != expected_diff.end(); ++itor_exp) {
    std::string time_system_name = itor_exp->first;
    const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
    Duration time_diff = time_system.computeTimeDifference(moment1, moment2);
    if (!time_diff.equivalentTo(expected_diff[time_system_name], tolerance)) {
      err() << "computeTimeDifference(moment1, moment2) of " << time_system_name << " returned " << time_diff <<
        " for moment1 = moment_type(" << moment1.first << ", " << moment1.second << ") and moment2 = moment_type(" <<
        moment2.first << ", " << moment2.second << "), not equivalent to the expected result, " <<
        expected_diff[time_system_name] << ", with tolerance of " << tolerance << "." << std::endl;
    }
  }
}

void TimeSystemTestApp::testOneDateTimeComputation(const moment_type & moment, const datetime_type & datetime,
  const datetime_type & datetime_utc) {
  std::list<std::string> time_system_name_list;
  time_system_name_list.push_back("TAI");
  time_system_name_list.push_back("TDB");
  time_system_name_list.push_back("TT");
  time_system_name_list.push_back("UTC");
  double tolerance = 1.e-9;

  for (std::list<std::string>::const_iterator itor_sys = time_system_name_list.begin(); itor_sys != time_system_name_list.end();
    ++itor_sys) {
    std::string time_system_name = *itor_sys;
    const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
    datetime_type expected_datetime(datetime);
    if ("UTC" == time_system_name) expected_datetime = datetime_utc;
    datetime_type result_datetime = time_system.computeDateTime(moment);

    if (result_datetime.first != expected_datetime.first || std::fabs(result_datetime.second - expected_datetime.second) > tolerance) {
      err() << "computeDateTime of " << time_system_name << " returned datetime_type(" << result_datetime.first <<
        ", " << result_datetime.second << ") for moment_type(" << moment.first << ", " << moment.second <<
        "), not equivalent to the expected result of datetime_type(" << expected_datetime.first << ", " << expected_datetime.second <<
        "), with tolerance of " << tolerance << " seconds." << std::endl;
    }
  }
}

void TimeSystemTestApp::testTimeSystem() {
  setMethod("testTimeSystem");

  // Set the default leap second file to a local copy of an actual leap second table.
  std::string test_leap = prependDataPath("testls.fits");
  TimeSystem::setDefaultLeapSecFileName(test_leap);

  // Get access to all time systems which should exist.
  TimeSystem::getSystem("TDB");
  TimeSystem::getSystem("tAi");
  TimeSystem::getSystem("tt");
  TimeSystem::getSystem("utc");

  // Verify that accessing a non-existent time system fails.
  try {
    TimeSystem::getSystem("Not a time system");
  } catch (const std::exception &) {
    // Expected.
  }

  // This number was arbitrarily chosen such that the shift is approximately at its maximum.
  long ref_day = 51910 + 365/4;
  double tai_ref_sec = 100. - 32.184;
  double tt_ref_sec  = 100.;
  double tdb_ref_sec = 100. + 0.001634096289;
  double utc_ref_sec = 100. - 32. - 32.184;

  moment_type tai_ref_moment(moment_type(ref_day, Duration(tai_ref_sec, "Sec")));
  moment_type tt_ref_moment(moment_type(ref_day, Duration(tt_ref_sec, "Sec")));
  moment_type tdb_ref_moment(moment_type(ref_day, Duration(tdb_ref_sec, "Sec")));
  moment_type utc_ref_moment(moment_type(ref_day, Duration(utc_ref_sec, "Sec")));

  double tdb_tolerance = 1.e-7; // 100 ns is the accuracy of algorithms involving TDB.

  // Test conversions, reflexive cases.
  testOneConversion("TAI", tai_ref_moment, "TAI", tai_ref_moment);
  testOneConversion("TDB", tdb_ref_moment, "TDB", tdb_ref_moment);
  testOneConversion("TT",  tt_ref_moment,  "TT",  tt_ref_moment);
  testOneConversion("UTC", utc_ref_moment, "UTC", utc_ref_moment);

  // Test conversions from TAI to...
  testOneConversion("TAI", tai_ref_moment, "TDB", tdb_ref_moment, tdb_tolerance);
  testOneConversion("TAI", tai_ref_moment, "TT",  tt_ref_moment);
  testOneConversion("TAI", tai_ref_moment, "UTC", utc_ref_moment);

  // Test conversions from TDB to...
  testOneConversion("TDB", tdb_ref_moment, "TAI", tai_ref_moment, tdb_tolerance);
  testOneConversion("TDB", tdb_ref_moment, "TT",  tt_ref_moment, tdb_tolerance);
  testOneConversion("TDB", tdb_ref_moment, "UTC", utc_ref_moment, tdb_tolerance);

  // Test conversions from TT to...
  testOneConversion("TT",  tt_ref_moment, "TAI", tai_ref_moment);
  testOneConversion("TT",  tt_ref_moment, "TDB", tdb_ref_moment, tdb_tolerance);
  testOneConversion("TT",  tt_ref_moment, "UTC", utc_ref_moment);

  // Test conversions from UTC to...
  testOneConversion("UTC", utc_ref_moment, "TAI", tai_ref_moment);
  testOneConversion("UTC", utc_ref_moment, "TDB", tdb_ref_moment, tdb_tolerance);
  testOneConversion("UTC", utc_ref_moment, "TT",  tt_ref_moment);

  // Use three leap seconds for generating tests.
  double diff0 = 31.;
  double diff1 = 32.;
  double diff2 = 33.;

  // Use times for three leap seconds for generating tests.
  // long leap0 = 50630;
  long leap1 = 51179;
  long leap2 = 53736;
  long delta_leap = leap2 - leap1;

  // To ensure UTC->TAI is handled correctly, do some tougher conversions, i.e. times which are close to
  // times when leap seconds are inserted.
  // --- At an exact time of leap second insertion.
  testOneConversion("UTC", moment_type(leap1, Duration(0.,    "Sec")),
                    "TAI", moment_type(leap1, Duration(diff1, "Sec")));
  // --- Slightly before a leap second is inserted.
  testOneConversion("UTC", moment_type(leap1 - 1, Duration(SecPerDay() - .001,         "Sec")),
                    "TAI", moment_type(leap1 - 1, Duration(SecPerDay() - .001 + diff0, "Sec")));
  // --- Same as above, but with a large elapsed time.
  //     Although the total time (origin + elapsed) is large enough to cross two leap second boundaries, still
  //     the earliest leap second should be used because the choice of leap second is based only on the origin time.
  testOneConversion("UTC", moment_type(leap1 - 1, Duration(delta_leap + 1, 0., "Day") + Duration(-.001 + 2.002,         "Sec")),
                    "TAI", moment_type(leap1 - 1, Duration(delta_leap + 1, 0., "Day") + Duration(-.001 + 2.002 + diff0, "Sec")));

  // To ensure TAI->UTC is handled correctly, do some tougher conversions, i.e. times which are close to
  // times when leap seconds are inserted.
  // --- At the end of a leap second.
  testOneConversion("TAI", moment_type(leap1, Duration(diff1, "Sec")),
                    "UTC", moment_type(leap1, Duration(0.,    "Sec")));
  // --- During a leap second.
  testOneConversion("TAI", moment_type(leap1, Duration(-0.3 + diff1, "Sec")),
                    "UTC", moment_type(leap1, Duration(-0.3,         "Sec")));
  // --- At the beginning of a leap second.
  testOneConversion("TAI", moment_type(leap1, Duration(-1.0 + diff1, "Sec")),
                    "UTC", moment_type(leap1, Duration(-1.0,         "Sec")));
  // --- After the end of a leap second.
  testOneConversion("TAI", moment_type(leap1, Duration(+0.3 + diff1, "Sec")),
                    "UTC", moment_type(leap1, Duration(+0.3,         "Sec")));
  // --- Before the beginning of a leap second.
  testOneConversion("TAI", moment_type(leap1, Duration(-1.3 + diff1, "Sec")),
                    "UTC", moment_type(leap1, Duration(-1.3,         "Sec")));

  // Test that conversion uses table keyed by TAI times, not by UTC.
  testOneConversion("TAI", moment_type(leap1 - 1, Duration(SecPerDay() - 2.,         "Sec")),
                    "UTC", moment_type(leap1 - 1, Duration(SecPerDay() - 2. - diff0, "Sec")));

  // Test case before first time covered by the current UTC definition. This is "undefined" in the current scheme.
  long diff_oldest = 10;
  try {
    testOneConversion("UTC", moment_type(0, Duration(0., "Sec")), "TAI", moment_type(0, Duration(diff_oldest, "Sec")));
    err() << "Conversion of time 0. MJD UTC to TAI did not throw an exception." << std::endl;
  } catch (const std::exception &) {
    // That's OK!
  }
  try {
    testOneConversion("TAI", moment_type(0, Duration(diff_oldest, "Sec")), "UTC", moment_type(0, Duration(0., "Sec")));
    err() << "Conversion of time 0. MJD TAI to UTC did not throw an exception." << std::endl;
  } catch (const std::exception &) {
    // That's OK!
  }

  // Test undefined UTC time, with the time origin covered by the current definition.
  long oldest_mjd = 41317;
  try {
    testOneConversion("UTC", moment_type(oldest_mjd + 1, Duration(-2, 0., "Day")),
                      "TAI", moment_type(oldest_mjd + 1, Duration(-2, 0., "Day") + Duration(diff_oldest, "Sec")));
    err() << "Conversion of moment_type(" << oldest_mjd + 1 << ", " << Duration(-2, 0., "Day") <<
      ") from UTC to TAI did not throw an exception." << std::endl;
  } catch (const std::exception &) {
    // That's OK!
  }
  try {
    testOneConversion("TAI", moment_type(oldest_mjd + 1, Duration(-2, 0., "Day") + Duration(diff_oldest, "Sec")),
                      "UTC", moment_type(oldest_mjd + 1, Duration(-2, 0., "Day")));
    err() << "Conversion of moment_type(" << oldest_mjd + 1 << ", " << Duration(-2, 0., "Day") + Duration(diff_oldest, "Sec") <<
      ") from TAI to UTC did not throw an exception." << std::endl;
  } catch (const std::exception &) {
    // That's OK!
  }

  // Try loading non-existent file for leap second table.
  try {
    TimeSystem::loadLeapSeconds("non-existent-file.fits");
    err() << "TimeSystem::loadLeapSeconds(\"non-existent-file.fits\") did not fail." << std::endl;
  } catch (const std::exception &) {
    // That's OK!
  }

  // Try loading non-existent file for leap second table, by first setting the default file name to something wrong.
  try {
    TimeSystem::setDefaultLeapSecFileName("non-existent-file.fits");
    TimeSystem::loadLeapSeconds("deFAULT");
    err() << "After TimeSystem::setDefaultLeapSecFileName(\"non-existent-file.fits\"), loadLeapSeconds(\"deFAULT\") did not fail."
      << std::endl;
  } catch (const std::exception &) {
    // That's OK!
  }

  // Set the bogus leap second file name to a local variable. This file contains a removal of a leap second.
  std::string bogus_leap = prependDataPath("bogusls.fits");

  // Test loading specific file for leap second table, by first setting the default file name.
  TimeSystem::setDefaultLeapSecFileName(bogus_leap);

  if (bogus_leap != TimeSystem::getDefaultLeapSecFileName()) 
    err() << "After setting default leap second file name, default leap second file name was " <<
      TimeSystem::getDefaultLeapSecFileName() << ", not " << bogus_leap << " as expected." << std::endl;

  TimeSystem::loadLeapSeconds();

  // Test case after last time covered by the current UTC definition.
  long leap_last = 53737;
  double diff_last = 31.;
  testOneConversion("UTC", moment_type(leap_last, Duration(100., "Sec")),
    "TAI", moment_type(leap_last, Duration(100. + diff_last, "Sec")));

  // Reset default leap second file name.
  TimeSystem::setDefaultLeapSecFileName("");

  // Finally, test loading the real leap seconds file.
  TimeSystem::loadLeapSeconds(bogus_leap);

  // Test case after last time covered by the current UTC definition.
  leap_last = 53737;
  diff_last = 31.;
  testOneConversion("UTC", moment_type(leap_last, Duration(100., "Sec")),
    "TAI", moment_type(leap_last, Duration(100. + diff_last, "Sec")));

  // Use three leap seconds for generating tests.
  diff0 = 30.;
  diff1 = 29.;
  diff2 = 30.;

  // Use times for three leap seconds for generating tests.
  // leap0 = 50083;
  leap1 = 50630;
  leap2 = 51179;
  delta_leap = leap2 - leap1;

  // Test now the case where leap second is negative (Earth speeds up).
  // To ensure TAI->UTC is handled correctly, do some tougher conversions, i.e. times which are close to
  // times when leap seconds are removed.
  // --- At an exact time of leap second removal.
  testOneConversion("TAI", moment_type(leap1, Duration(diff1, "Sec")),
                    "UTC", moment_type(leap1, Duration(0.,    "Sec")));
  // --- Slightly before a leap second is removed.
  testOneConversion("TAI", moment_type(leap1, Duration(-.001 + diff1, "Sec")),
                    "UTC", moment_type(leap1, Duration(-.001,         "Sec")));
  // --- Same as above, but with a large elapsed time.
  //     Although the total time (origin + elapsed) is large enough to cross two leap second boundaries, still
  //     the earliest leap second should be used because the choice of leap second is based only on the origin time.
  testOneConversion("TAI", moment_type(leap1, Duration(delta_leap, 0., "Day") + Duration(-.001 + .002,         "Sec")),
                    "UTC", moment_type(leap1, Duration(delta_leap, 0., "Day") + Duration(-.001 + .002 - diff1, "Sec")));

  // To ensure UTC->TAI is handled correctly, do some tougher conversions, i.e. times which are close to
  // times when leap seconds are removed.
  // --- At the end of a leap second.
  testOneConversion("UTC", moment_type(leap1, Duration(0.,    "Sec")),
                    "TAI", moment_type(leap1, Duration(diff1, "Sec")));
  // --- During a leap second
  // Note: As of May 29th, 2008, the design doesn't allow/need this test due to the improved robustness.
  // --- At the beginning of a leap second.
  testOneConversion("UTC", moment_type(leap1 - 1, Duration(SecPerDay() - 1.0,         "Sec")),
                    "TAI", moment_type(leap1 - 1, Duration(SecPerDay() - 1.0 + diff0, "Sec")));
  // --- After the end of a leap second.
  testOneConversion("UTC", moment_type(leap1, Duration(0.3,         "Sec")),
                    "TAI", moment_type(leap1, Duration(diff1 + 0.3, "Sec")));
  // --- Before the beginning of a leap second.
  testOneConversion("UTC", moment_type(leap1 - 1, Duration(SecPerDay() - 1.3,         "Sec")),
                    "TAI", moment_type(leap1 - 1, Duration(SecPerDay() - 1.3 + diff0, "Sec")));

  // Test computeTimeDifference method.
  // Middle of nowhere
  testOneSubtraction(moment_type(51910, Duration(120., "Sec")), moment_type(51910, Duration(100., "Sec")),  20., 20.);
  // Leap second insertion
  testOneSubtraction(moment_type(leap2, Duration(+10., "Sec")), moment_type(leap2 - 1, Duration(SecPerDay() - 10., "Sec")), 20., 21.);
  // leap second removal
  testOneSubtraction(moment_type(leap1, Duration(+10., "Sec")), moment_type(leap1 - 1, Duration(SecPerDay() - 10., "Sec")), 20., 19.);
  // Non-existing time in UTC
  // Note: As of May 29th, 2008, the design doesn't allow/need this test due to the improved robustness.

  // Test computeDateTime method.
  double deltat = 20.;
  // Middle of nowhere
  testOneDateTimeComputation(moment_type(51910, Duration(100., "Sec")), datetime_type(51910, 100.), datetime_type(51910, 100.));
  // Middle of nowhere, with a negative elapsed time
  testOneDateTimeComputation(moment_type(51910, Duration(-100., "Sec")),
     datetime_type(51909, SecPerDay() - 100.), datetime_type(51909, SecPerDay() - 100.));
  // Across leap second insertion in UTC
  testOneDateTimeComputation(moment_type(leap2 - 1, Duration(1, 0., "Day") + Duration(deltat - 10., "Sec")),
    datetime_type(leap2, deltat - 10.), datetime_type(leap2, deltat - 10. - 1.));
  // Across leap second removal in UTC
  testOneDateTimeComputation(moment_type(leap1 - 1, Duration(1, 0., "Day") + Duration(deltat - 10., "Sec")),
    datetime_type(leap1, deltat - 10.), datetime_type(leap1, deltat - 10. + 1.));
  // Non-existing time in UTC
  // Note: As of May 29th, 2008, the design doesn't allow/need this test due to the improved robustness.

  // Tests at times close to times when leap seconds are inserted (in UTC).
  // Note: As of May 29th, 2008, the design isn't sensitive to those tests due to the improved robustness.
  // --- Before the beginning of a leap second.
  testOneDateTimeComputation(moment_type(leap2 - 1, Duration(1, 0., "Day") + Duration(deltat - 0.3, "Sec")),
    datetime_type(leap2, deltat - 0.3), datetime_type(leap2, deltat - 1.3));
  // --- At the beginning of a leap second.
  testOneDateTimeComputation(moment_type(leap2 - 1, Duration(1, 0., "Day") + Duration(deltat, "Sec")),
    datetime_type(leap2, deltat), datetime_type(leap2, deltat - 1.0));
  // --- During a leap second.
  testOneDateTimeComputation(moment_type(leap2 - 1, Duration(1, 0., "Day") + Duration(deltat + 0.3, "Sec")),
    datetime_type(leap2, deltat + 0.3), datetime_type(leap2, deltat - 0.7));
  // --- At the end of a leap second.
  testOneDateTimeComputation(moment_type(leap2 - 1, Duration(1, 0., "Day") + Duration(deltat + 1.0, "Sec")),
    datetime_type(leap2, deltat + 1.0), datetime_type(leap2, deltat));
  // --- After the end of a leap second.
  testOneDateTimeComputation(moment_type(leap2 - 1, Duration(1, 0., "Day") + Duration(deltat + 1.3, "Sec")),
    datetime_type(leap2, deltat + 1.3), datetime_type(leap2, deltat + 0.3));

  // Tests at times close to times when leap seconds are removed (in UTC).
  // Note: As of May 29th, 2008, the design isn't sensitive to those tests due to the improved robustness.
  // --- Before the beginning of a leap second.
  testOneDateTimeComputation(moment_type(leap1 - 1, Duration(1, 0., "Day") + Duration(deltat - 1.3, "Sec")),
    datetime_type(leap1, deltat - 1.3), datetime_type(leap1, deltat - 0.3));
  // --- At the beginning of a leap second.
  testOneDateTimeComputation(moment_type(leap1 - 1, Duration(1, 0., "Day") + Duration(deltat - 1.0, "Sec")),
    datetime_type(leap1, deltat - 1.0), datetime_type(leap1, deltat));
  // --- During a leap second.
  // Note: As of May 29th, 2008, the design doesn't allow/need this test due to the improved robustness.
  // --- At the end of a leap second.
  // Note: As of May 29th, 2008, the design doesn't allow/need this test due to the improved robustness.
  // --- After the end of a leap second.
  testOneDateTimeComputation(moment_type(leap1 - 1, Duration(1, 0., "Day") + Duration(deltat - 0.7, "Sec")),
    datetime_type(leap1, deltat - 0.7), datetime_type(leap1, deltat + 0.3));

  // Test computeMoment method.
  std::list<std::string> time_system_name_list;
  time_system_name_list.push_back("TAI");
  time_system_name_list.push_back("TDB");
  time_system_name_list.push_back("TT");
  time_system_name_list.push_back("UTC");
  for (std::list<std::string>::const_iterator itor_sys = time_system_name_list.begin(); itor_sys != time_system_name_list.end();
    ++itor_sys) {
    std::string time_system_name = *itor_sys;
    const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));

    // Test conversions in the middle of nowhere.
    datetime_type input_datetime(51910, 100.);
    moment_type expected_moment(51910, Duration(100., "Sec"));
    moment_type result_moment = time_system.computeMoment(input_datetime);
    Duration tolerance(1.e-9, "Sec");
    if (result_moment.first != expected_moment.first || !result_moment.second.equivalentTo(expected_moment.second, tolerance)) {
      err() << "computeMoment of " << time_system_name << " returned moment_type(" << result_moment.first <<
        ", " << result_moment.second << ") for datetime_type(" << input_datetime.first << ", " << input_datetime.second <<
        "), not equivalent to the expected result of moment_type(" << expected_moment.first << ", " << expected_moment.second <<
        "), with tolerance of " << tolerance << " seconds." << std::endl;
    }

    // Test detections of the time part that is out of bounds, below the lower boundary.
    input_datetime = datetime_type(51910, -.001);
    try {
      time_system.computeMoment(input_datetime);
      err() << "computeMoment of " << time_system_name << " did not throw an exception for for datetime_type(" <<
        input_datetime.first << ", " << input_datetime.second << ")." << std::endl;
    } catch (const std::exception &) {
    }

    // Test detections of the time part that is out of bounds, at the upper boundary.
    input_datetime = datetime_type(51910, SecPerDay());
    try {
      time_system.computeMoment(input_datetime);
      err() << "computeMoment of " << time_system_name << " did not throw an exception for for datetime_type(" <<
        input_datetime.first << ", " << input_datetime.second << ")." << std::endl;
    } catch (const std::exception &) {
    }

    if ("UTC" == time_system_name) {
      // Test NO detections of the time part that is out of bounds, during an inserted leap second.
      input_datetime = datetime_type(leap2 - 1, SecPerDay() + .999);
      try {
        time_system.computeMoment(input_datetime);
      } catch (const std::exception &) {
        err() << "computeMoment of " << time_system_name << " threw an exception for for datetime_type(" <<
          input_datetime.first << ", " << input_datetime.second << ")." << std::endl;
      }

      // Test detections of the time part that is out of bounds, after an inserted leap second.
      input_datetime = datetime_type(leap2 - 1, SecPerDay() + 1.001);
      try {
        time_system.computeMoment(input_datetime);
        err() << "computeMoment of " << time_system_name << " did not throw an exception for for datetime_type(" <<
          input_datetime.first << ", " << input_datetime.second << ")." << std::endl;
      } catch (const std::exception &) {
      }

      // Test detections of the time part that is out of bounds, during a removed leap second.
      input_datetime = datetime_type(leap1 - 1, SecPerDay() + -.999);
      try {
        time_system.computeMoment(input_datetime);
        err() << "computeMoment of " << time_system_name << " did not throw an exception for for datetime_type(" <<
          input_datetime.first << ", " << input_datetime.second << ")." << std::endl;
      } catch (const std::exception &) {
      }
    }
  }

  // Note: The following test became unnecessary as of May 29th, 2008 due to the improved robustness of the design.
#if 0
  // Test proper handling of origin of UTC Moment. It should be adjusted to an existing MJD time in UTC and
  // special caution is needed for cases of leap second removal. In the test below, note that MJD Duration(leap1, -.7)
  // does NOT exist in UTC system because it expresses the "removed" second, which requires some adjustment for UTC origin.
  Duration mjd_tai(leap1, -.7);
  Duration mjd_utc(leap1, -diff0 -.7);
  testOneConversion("UTC", mjd_utc, Duration(0, 0.), "TAI", mjd_tai, Duration(0, 0.)); // easy test.
  testOneConversion("TAI", mjd_tai, Duration(0, 0.), "UTC", mjd_utc, Duration(0, 0.)); // tougher test, need handle with care.

  // Another tricky test.  MJD Duration(leap1, 0.) DOES exist in UTC system, too, but it is immediately after the leap
  // second removal. So, below needs a different kind of careful handling of UTC origin than above.
  mjd_tai = Duration(leap1, 0.);
  mjd_utc = Duration(leap1, -diff0);
  testOneConversion("UTC", mjd_utc, Duration(0, 0.), "TAI", mjd_tai, Duration(0, 0.)); // easy test.
  testOneConversion("TAI", mjd_tai, Duration(0, 0.), "UTC", mjd_utc, Duration(0, 0.)); // tougher test, need handle with care.
#endif

  // Test of conversion condition of TAI-to-UTC conversion.
  // Below must produce identical result for test input of 100. seconds after 51910.0 MJD (TAI).
  long tai_day = 0;
  double tai_sec = 100. + 51910. * SecPerDay();
  try {
    testOneConversion("TAI", moment_type(tai_day, Duration(tai_sec, "Sec")),
                      "UTC", moment_type(oldest_mjd, Duration(51910 - oldest_mjd, 0., "Day") + Duration(100. - diff_oldest, "Sec")));
  } catch (const std::exception &) {
    err() << "Conversion of TAI to UTC for moment_type(" << tai_day << ", " << tai_sec << ") threw an exception." << std::endl;
  }

  // Test of conversion condition of TAI-to-UTC conversion.
  // Below must throw an exception because resulting UTC cannot be expressed properly.
  tai_day = 51910;
  tai_sec = 100. - 51910. * SecPerDay();
  try {
    testOneConversion("TAI", moment_type(tai_day, Duration(tai_sec, "Sec")),
                      "UTC", moment_type(0, Duration(100. - diff2, "Sec")));
    err() << "Conversion of TAI to UTC for moment_type(" << tai_day << ", " << tai_sec << ") did not throw an exception." << std::endl;
  } catch (const std::exception &) {
    // That's OK!
  }

  // Test of conversion condition of UTC-to-TAI conversion.
  // Below must throw an exception because originating UTC cannot be interpreted properly.
  long utc_day = 0;
  double utc_sec = 100. + 51910. * SecPerDay() - diff2;
  try {
    testOneConversion("UTC", moment_type(utc_day, Duration(utc_sec, "Sec")),
                      "TAI", moment_type(51910, Duration(100., "Sec")));
    err() << "Conversion of UTC to TAI for moment_type(" << utc_day << ", " << utc_sec << ") did not throw an exception." << std::endl;
  } catch (const std::exception &) {
    // That's OK!
  }

  // Test of conversion condition of UTC-to-TAI conversion.
  // Below must throw an exception because originating UTC time cannot be interpreted properly.
  utc_day = 51910;
  utc_sec = 100. - 51910. * SecPerDay() - diff2;
  try {
    testOneConversion("UTC", moment_type(utc_day, Duration(utc_sec, "Sec")),
                      "TAI", moment_type(utc_day, Duration(-utc_day, 0., "Day") + Duration(100., "Sec")));
    err() << "Conversion of UTC to TAI for moment_type(" << utc_day << ", " << utc_sec << ") did not throw an exception." << std::endl;
  } catch (const std::exception &) {
    // That's OK!
  }

  // Note: As of May 29th, 2008, the design doesn't allow/need this test due to the improved robustness.
#if 0
  // Test computeMjd method of UTC for a Moment object whose origin is during a leap second being removed.
  utc_moment = Moment(Duration(leap1 - 1, SecPerDay() - 1./3.), Duration(0, 0.));
  result = TimeSystem::getSystem("UTC").computeMjd(utc_moment);
  expected_result = Duration(leap1, 0.);
  if (result != expected_result) {
    err() << "UTC system's computeMjd(" << utc_moment.first << ", " << utc_moment.second << ") returned " <<
      result << ", not exactly equal to " << expected_result << " as expected." << std::endl;
  }

  // Test computeTimeDifference method of UTC for two Moment objects during a leap second being removed.
  Duration utc_mjd1 = Duration(leap1 - 1, SecPerDay() - 1./3.);
  Duration utc_mjd2 = Duration(leap1 - 1, SecPerDay() - 2./3.);
  result = TimeSystem::getSystem("UTC").computeTimeDifference(utc_mjd1, utc_mjd2);
  expected_result = Duration(0, 0.);
  if (result != expected_result) {
    err() << "UTC system's computeTimeDifference method computed a time difference between " << utc_mjd1.getValue(Day) <<
      " MJD and " << utc_mjd2.getValue(Day) << " MJD as " << result << ", not " << expected_result << " as expected." << std::endl;
  }
#endif

  // Test of origin for UTC time that must be after MJD 41317.0 (January 1st, 1972) by definition.
  moment_type tai_moment(oldest_mjd - 1, Duration(1, 0., "Day") + Duration(1. + diff_oldest, "Sec"));
  moment_type utc_moment = TimeSystem::getSystem("UTC").convertFrom(TimeSystem::getSystem("TAI"), tai_moment);
  if (oldest_mjd > utc_moment.first) {
    err() << "Conversion from TAI to UTC for input Moment(" << tai_moment.first << ", " << tai_moment.second <<
    ") returned Moment(" << utc_moment.first << ", " << utc_moment.second <<
      "), which is earlier than the beginning of the current UTC definition " << oldest_mjd << " MJD." << std::endl;
  }
}

void TimeSystemTestApp::compareAbsoluteTime(const AbsoluteTime & abs_time, const AbsoluteTime & later_time) {
  // Test operator >.
  if (abs_time > later_time) err() << "AbsoluteTime::operator > returned true for \"" << abs_time << "\" > \"" <<
    later_time << "\"" << std::endl;
  if (!(later_time > abs_time)) err() << "AbsoluteTime::operator > returned false for \"" << later_time << "\" > \"" <<
    abs_time << "\"" << std::endl;
  if (abs_time > abs_time) err() << "AbsoluteTime::operator > returned true for \"" << abs_time << "\" > \"" <<
    abs_time << "\"" << std::endl;

  // Test operator >=.
  if (abs_time >= later_time) err() << "AbsoluteTime::operator >= returned true for \"" << abs_time << "\" >= \"" <<
    later_time << "\"" << std::endl;
  if (!(later_time >= abs_time)) err() << "AbsoluteTime::operator >= returned false for \"" << later_time << "\" >= \"" <<
    abs_time << "\"" << std::endl;
  if (!(abs_time >= abs_time)) err() << "AbsoluteTime::operator >= returned false for \"" << abs_time << "\" >= \"" <<
    abs_time << "\"" << std::endl;

  // Test operator <.
  if (!(abs_time < later_time)) err() << "AbsoluteTime::operator < returned false for \"" << abs_time << "\" < \"" <<
    later_time << "\"" << std::endl;
  if (later_time < abs_time) err() << "AbsoluteTime::operator < returned true for \"" << later_time << "\" < \"" <<
    abs_time << "\"" << std::endl;
  if (abs_time < abs_time) err() << "AbsoluteTime::operator < returned true for \"" << abs_time << "\" < \"" <<
    abs_time << "\"" << std::endl;

  // Test operator <=.
  if (!(abs_time <= later_time)) err() << "AbsoluteTime::operator <= returned false for \"" << abs_time << "\" <= \"" <<
    later_time << "\"" << std::endl;
  if (later_time <= abs_time) err() << "AbsoluteTime::operator <= returned true for \"" << later_time << "\" <= \"" <<
    abs_time << "\"" << std::endl;
  if (!(abs_time <= abs_time)) err() << "AbsoluteTime::operator <= returned false for \"" << abs_time << "\" <= \"" <<
    abs_time << "\"" << std::endl;
}

void TimeSystemTestApp::testAbsoluteTime() {
  setMethod("testAbsoluteTime");

  // Use the bogus leap second table for this unit test.
  std::string bogus_leap = prependDataPath("bogusls.fits");
  TimeSystem::loadLeapSeconds(bogus_leap);

  // Prepare test parameters.
  long mjd_day = 54321;
  double mjd_sec = 12345.;
  Mjd expected_mjd(mjd_day, mjd_sec / SecPerDay());
  Mjd1 expected_mjd1(mjd_day + mjd_sec / SecPerDay());

  // Test the basic constructor and the getter for high-precision MJD.
  AbsoluteTime abs_time("TT", mjd_day, mjd_sec);
  Mjd result_mjd(0, 0.);
  abs_time.get("TT", result_mjd);
  double double_tol = 100.e-9 / SecPerDay(); // 100 nanoseconds in days.
  if (expected_mjd.m_int != result_mjd.m_int || std::fabs(expected_mjd.m_frac - result_mjd.m_frac) > double_tol) {
    err() << "After abs_time = AbsoluteTime(\"TT\", " << mjd_day << ", " << mjd_sec <<
      "), abs_time.get(\"TT\", result_mjd) gave result_mjd = (" << result_mjd.m_int << ", " << result_mjd.m_frac <<
      "), not (" << expected_mjd.m_int << ", " << expected_mjd.m_frac << ") as expected." << std::endl;
  }

  // Test the getter for low-precision MJD.
  abs_time = AbsoluteTime("TT", mjd_day, mjd_sec);
  Mjd1 result_mjd1(0.);
  abs_time.get("TT", result_mjd1);
  double_tol = 10.e-6 / SecPerDay(); // 10 microseconds in days.
  if (std::fabs(expected_mjd1.m_day - result_mjd1.m_day) > double_tol) {
    err() << "After abs_time = AbsoluteTime(\"TT\", " << mjd_day << ", " << mjd_sec <<
      "), abs_time.get(\"TT\", result_mjd1) gave result_mjd1.m_day = " << result_mjd1.m_day << ", not " <<
      expected_mjd1.m_day << " as expected." << std::endl;
  }

  // Test the getter for high-precision MJD, with a different time system.
  abs_time = AbsoluteTime("TT", mjd_day, mjd_sec);
  result_mjd = Mjd(0, 0.);
  abs_time.get("TAI", result_mjd);
  const double tai_minus_tt = -32.184;
  Mjd expected_mjd_tai(mjd_day, (mjd_sec + tai_minus_tt) / SecPerDay());
  double_tol = 100.e-9 / SecPerDay(); // 100 nanoseconds in days.
  if (expected_mjd_tai.m_int != result_mjd.m_int || std::fabs(expected_mjd_tai.m_frac - result_mjd.m_frac) > double_tol) {
    err() << "After abs_time = AbsoluteTime(\"TT\", " << mjd_day << ", " << mjd_sec <<
      "), abs_time.get(\"TAI\", result_mjd) gave result_mjd = (" << result_mjd.m_int << ", " << result_mjd.m_frac <<
      "), not (" << expected_mjd_tai.m_int << ", " << expected_mjd_tai.m_frac << ") as expected." << std::endl;
  }

  // Test the getter for high-precision MJD, during an inserted leap second.
  long mjd_day_leap = 51178;
  double mjd_sec_leap = 86400.3;
  abs_time = AbsoluteTime("UTC", mjd_day_leap, mjd_sec_leap);
  result_mjd = Mjd(0, 0.);
  try {
    abs_time.get("UTC", result_mjd);
    err() << "After abs_time = AbsoluteTime(\"UTC\", " << mjd_day_leap << ", " << mjd_sec_leap <<
      "), abs_time.get(\"UTC\", result_mjd) did not throw an exception." << std::endl;
  } catch (const std::exception &) {
  }

  // Test the getter for high-precision MJD, during an inserted leap second, with a different time system.
  abs_time = AbsoluteTime("UTC", mjd_day_leap, mjd_sec_leap);
  result_mjd = Mjd(0, 0.);
  abs_time.get("TAI", result_mjd);
  double tai_minus_utc = 29.;
  Mjd expected_mjd_leap(mjd_day_leap + 1, (mjd_sec_leap - SecPerDay() + tai_minus_utc) / SecPerDay());
  double_tol = 100.e-9 / SecPerDay(); // 100 nanoseconds in days.
  if (expected_mjd_leap.m_int != result_mjd.m_int
      || std::fabs(expected_mjd_leap.m_frac - result_mjd.m_frac) > double_tol) {
    err() << "After abs_time = AbsoluteTime(\"UTC\", " << mjd_day_leap << ", " << mjd_sec_leap <<
      "), abs_time.get(\"TAI\", result_mjd) gave result_mjd = (" << result_mjd.m_int << ", " << result_mjd.m_frac <<
      "), not (" << expected_mjd_leap.m_int << ", " << expected_mjd_leap.m_frac << ") as expected." << std::endl;
  }

  // Test the getter for high-precision MJD, during an inserted leap second, set by a different time system.
  abs_time = AbsoluteTime("TAI", mjd_day_leap + 1, mjd_sec_leap - SecPerDay() + tai_minus_utc);
  result_mjd = Mjd(0, 0.);
  try {
    abs_time.get("UTC", result_mjd);
    err() << "After abs_time = AbsoluteTime(\"TAI\", " << mjd_day_leap + 1 << ", " <<
      mjd_sec_leap - SecPerDay() + tai_minus_utc << "), abs_time.get(\"UTC\", result_mjd) did not throw an exception." << std::endl;
  } catch (const std::exception &) {
  }

  // Test the printer (represent method).
  abs_time = AbsoluteTime("TT", mjd_day, mjd_sec);
  std::string result_string = abs_time.represent("TT", MjdFmt);
  std::string expected_string("54321.142881944444444 MJD (TT)");
  if (expected_string != result_string) {
    err() << "After abs_time = AbsoluteTime(\"TT\", " << mjd_day << ", " << mjd_sec <<
      "), abs_time.represent(\"TT\", MjdFmt) returned \"" << result_string << "\", not \"" << expected_string <<
      "\" as expected." << std::endl;
  }

  // Test the printer (represent method), with a different time system.
  abs_time = AbsoluteTime("TT", mjd_day, mjd_sec);
  result_string = abs_time.represent("TAI", MjdFmt);
  expected_string = "54321.142509444444445 MJD (TAI)";
  if (expected_string != result_string) {
    err() << "After abs_time = AbsoluteTime(\"TT\", " << mjd_day << ", " << mjd_sec <<
      "), abs_time.represent(\"TAI\", MjdFmt) returned \"" << result_string << "\", not \"" << expected_string <<
      "\" as expected." << std::endl;
  }

  // Test detection of exception by the printer (represent method), during an inserted leap second, with UTC system.
  abs_time = AbsoluteTime("UTC", mjd_day_leap, mjd_sec_leap);
  try {
    abs_time.represent("UTC", MjdFmt);
    err() << "After abs_time = AbsoluteTime(\"UTC\", " << mjd_day_leap << ", " << mjd_sec_leap <<
      "), abs_time.represent(\"UTC\", MjdFmt) did not throw an exception." << std::endl;
  } catch (const std::exception &) {
  }

  // Test NO detection of exception by the printer (represent method), during an inserted leap second, with a different time system.
  abs_time = AbsoluteTime("UTC", mjd_day_leap, mjd_sec_leap);
  try {
    abs_time.represent("TT", MjdFmt);
  } catch (const std::exception &) {
    err() << "After abs_time = AbsoluteTime(\"TT\", " << mjd_day_leap << ", " << mjd_sec_leap <<
      "), abs_time.represent(\"TT\", \"MJD\") threw an exception." << std::endl;
  }

  // Test the constructor taking a high-precision MJD.
  abs_time = AbsoluteTime("TT", Mjd(mjd_day, mjd_sec / SecPerDay()));
  result_mjd = Mjd(0, 0.);
  abs_time.get("TT", result_mjd);
  double_tol = 100.e-9 / SecPerDay(); // 100 nanoseconds in days.
  if (expected_mjd.m_int != result_mjd.m_int || std::fabs(expected_mjd.m_frac - result_mjd.m_frac) > double_tol) {
    err() << "After abs_time = AbsoluteTime(\"TT\", Mjd(" << mjd_day << ", " << mjd_sec / SecPerDay() <<
      "))), abs_time.get(\"TT\", result_mjd) gave result_mjd = (" << result_mjd.m_int << ", " << result_mjd.m_frac <<
      "), not (" << expected_mjd.m_int << ", " << expected_mjd.m_frac << ") as expected." << std::endl;
  }

  // Test the constructor taking a low-precision MJD.
  abs_time = AbsoluteTime("TT", Mjd1(mjd_day + mjd_sec / SecPerDay()));
  result_mjd1 = Mjd1(0.);
  abs_time.get("TT", result_mjd);
  double_tol = 10.e-6 / SecPerDay(); // 10 microseconds in days.
  if (expected_mjd.m_int != result_mjd.m_int || std::fabs(expected_mjd.m_frac - result_mjd.m_frac) > double_tol) {
    err() << "After abs_time = AbsoluteTime(\"TT\", Mjd1(" << mjd_day + mjd_sec / SecPerDay() <<
      "))), abs_time.get(\"TT\", result_mjd) gave result_mjd = (" << result_mjd.m_int << ", " << result_mjd.m_frac <<
      "), not (" << expected_mjd.m_int << ", " << expected_mjd.m_frac << ") as expected." << std::endl;
  }

  // Test the constructor taking an MJD string.
  std::string mjd_string("54321.142881944444444");
  abs_time = AbsoluteTime("TT", MjdFmt, mjd_string);
  result_mjd = Mjd(0, 0.);
  abs_time.get("TT", result_mjd);
  double_tol = 100.e-9 / SecPerDay(); // 100 nanoseconds in days.
  if (expected_mjd.m_int != result_mjd.m_int || std::fabs(expected_mjd.m_frac - result_mjd.m_frac) > double_tol) {
    err() << "After abs_time = AbsoluteTime(\"TT\", MjdFmt, \"" << mjd_string <<
      "\"), abs_time.get(\"TT\", result_mjd) gave result_mjd = (" << result_mjd.m_int << ", " << result_mjd.m_frac <<
      "), not (" << expected_mjd.m_int << ", " << expected_mjd.m_frac << ") as expected." << std::endl;
  }

  // Test the setter taking a high-precision MJD.
  abs_time.set("TT", Mjd(mjd_day, mjd_sec / SecPerDay()));
  result_mjd = Mjd(0, 0.);
  abs_time.get("TT", result_mjd);
  double_tol = 100.e-9 / SecPerDay(); // 100 nanoseconds in days.
  if (expected_mjd.m_int != result_mjd.m_int || std::fabs(expected_mjd.m_frac - result_mjd.m_frac) > double_tol) {
    err() << "After abs_time.set(\"TT\", Mjd(" << mjd_day << ", " << mjd_sec / SecPerDay() <<
      "))), abs_time.get(\"TT\", result_mjd) gave result_mjd = (" << result_mjd.m_int << ", " << result_mjd.m_frac <<
      "), not (" << expected_mjd.m_int << ", " << expected_mjd.m_frac << ") as expected." << std::endl;
  }

  // Test the setter taking a low-precision MJD.
  abs_time.set("TT", Mjd1(mjd_day + mjd_sec / SecPerDay()));
  result_mjd1 = Mjd1(0.);
  abs_time.get("TT", result_mjd);
  double_tol = 10.e-6 / SecPerDay(); // 10 microseconds in days.
  if (expected_mjd.m_int != result_mjd.m_int || std::fabs(expected_mjd.m_frac - result_mjd.m_frac) > double_tol) {
    err() << "After abs_time.set(\"TT\", Mjd1(" << mjd_day + mjd_sec / SecPerDay() <<
      "))), abs_time.get(\"TT\", result_mjd) gave result_mjd = (" << result_mjd.m_int << ", " << result_mjd.m_frac <<
      "), not (" << expected_mjd.m_int << ", " << expected_mjd.m_frac << ") as expected." << std::endl;
  }

  // Test the setter taking an MJD string.
  abs_time.set("TT", MjdFmt, mjd_string);
  result_mjd = Mjd(0, 0.);
  abs_time.get("TT", result_mjd);
  double_tol = 100.e-9 / SecPerDay(); // 100 nanoseconds in days.
  if (expected_mjd.m_int != result_mjd.m_int || std::fabs(expected_mjd.m_frac - result_mjd.m_frac) > double_tol) {
    err() << "After abs_time.set(\"TT\", MjdFmt, \"" << mjd_string <<
      "\"), abs_time.get(\"TT\", result_mjd) gave result_mjd = (" << result_mjd.m_int << ", " << result_mjd.m_frac <<
      "), not (" << expected_mjd.m_int << ", " << expected_mjd.m_frac << ") as expected." << std::endl;
  }

  // Create an absolute time corresponding to MET 1000. s.
  mjd_day = 51910;
  mjd_sec = 1000.;
  abs_time = AbsoluteTime("TDB", mjd_day, mjd_sec);

  // Test printing the absolute time, with the shift operator.
  {
    std::ostringstream os;
    os << abs_time;
    result_string = os.str();
  }
  expected_string = "1000 seconds after 51910.0 MJD (TDB)";
  if (expected_string != result_string) {
    err() << "AbsoluteTime object wrote \"" << result_string << "\", not \"" << expected_string <<
    "\" as expected." << std::endl;
  }

  // Test printing the absolute time, with AbsoluteTime::describe method.
  result_string = abs_time.describe();
  expected_string = "AbsoluteTime(TDB, 51910, Duration(0, 1000))";
  if (expected_string != result_string) {
    err() << "AbsoluteTime object wrote \"" << result_string << "\", not \"" << expected_string <<
    "\" as expected." << std::endl;
  }

  // Test printing the absolute time, with a negative elapsed time, with the shift operator.
  abs_time = AbsoluteTime("TDB", mjd_day, -mjd_sec);
  {
    std::ostringstream os;
    os << abs_time;
    result_string = os.str();
  }
  expected_string = "85400 seconds after 51909.0 MJD (TDB)";
  if (expected_string != result_string) {
    err() << "AbsoluteTime object wrote \"" << result_string << "\", not \"" << expected_string <<
    "\" as expected." << std::endl;
  }

  // Test printing the absolute time, with a negative elapsed time, with AbsoluteTime::describe method.
  abs_time = AbsoluteTime("TDB", mjd_day, -mjd_sec);
  result_string = abs_time.describe();
  expected_string = "AbsoluteTime(TDB, 51910, Duration(-1, 85400))";
  if (expected_string != result_string) {
    err() << "AbsoluteTime object wrote \"" << result_string << "\", not \"" << expected_string <<
    "\" as expected." << std::endl;
  }

  // Test printing the absolute time, with a zero elapsed time, with the shift operator.
  abs_time = AbsoluteTime("TDB", mjd_day, Duration());
  {
    std::ostringstream os;
    os << abs_time;
    result_string = os.str();
  }
  expected_string = "51910.0 MJD (TDB)";
  if (expected_string != result_string) {
    err() << "AbsoluteTime object wrote \"" << result_string << "\", not \"" << expected_string <<
    "\" as expected." << std::endl;
  }

  // Test printing the absolute time, with a zero elapsed time, with AbsoluteTime::describe method.
  abs_time = AbsoluteTime("TDB", mjd_day, Duration());
  result_string = abs_time.describe();
  expected_string = "AbsoluteTime(TDB, 51910, Duration(0, 0))";
  if (expected_string != result_string) {
    err() << "AbsoluteTime object wrote \"" << result_string << "\", not \"" << expected_string <<
    "\" as expected." << std::endl;
  }

  // Test adding an elapsed time to this time.
  // Create an absolute time corresponding to 100. s MET TDB to verify adding an elapsed time.
  abs_time = AbsoluteTime("TDB", mjd_day, mjd_sec);
  double delta_t = 100.;
  ElapsedTime elapsed_time("TDB", Duration(delta_t, "Sec"));
  AbsoluteTime result = abs_time + elapsed_time;
  AbsoluteTime expected_result("TDB", mjd_day, mjd_sec + delta_t);
  ElapsedTime epsilon("TDB", Duration(1.e-9, "Sec")); // 1 nanosecond.
  if (!result.equivalentTo(expected_result, epsilon))
    err() << "Sum of absolute time and elapsed time using operator + was " << result << ", not " <<
      expected_result << " as expected." << std::endl;

  // Test AbsoluteTime::operator +=.
  result = abs_time;
  result += elapsed_time;
  if (!result.equivalentTo(expected_result, epsilon))
    err() << "Sum of absolute time and elapsed time using operator += was " << result << ", not " <<
      expected_result << " as expected." << std::endl;

  // Test AbsoluteTime::operator -=.
  result = abs_time;
  result -= elapsed_time;
  expected_result = AbsoluteTime("TDB", mjd_day, mjd_sec - delta_t);
  if (!result.equivalentTo(expected_result, epsilon))
    err() << "Sum of absolute time and elapsed time using operator -= was " << result << ", not " <<
      expected_result << " as expected." << std::endl;

  // Test adding in reverse order.
  result = elapsed_time + abs_time;
  expected_result = AbsoluteTime("TDB", mjd_day, mjd_sec + delta_t);
  if (!result.equivalentTo(expected_result, epsilon))
    err() << "Sum of elapsed time and absolute time in that order was " << result << ", not " <<
      expected_result << " as expected." << std::endl;

  // Test subtraction of elapsed time from absolute time.
  result = abs_time - elapsed_time;
  expected_result = AbsoluteTime("TDB", mjd_day, mjd_sec - delta_t);
  if (!result.equivalentTo(expected_result, epsilon))
    err() << "Elapsed time subtracted from absolute time gave " << result << ", not " <<
      expected_result << " as expected." << std::endl;

  // Make a test time which is later than the first time.
  AbsoluteTime later_time("TDB", mjd_day, mjd_sec + 100.);

  // Test comparison operators: >, >=, <, and <=.
  compareAbsoluteTime(abs_time, later_time);

  // Test comparison operators (>, >=, <, and <=) in UTC system.
  long mjd_leap = 51179;
  AbsoluteTime abs_time_utc("UTC", mjd_leap - 1, 86400.8);
  AbsoluteTime later_time_utc("UTC", mjd_leap, 0.2);
  compareAbsoluteTime(abs_time_utc, later_time_utc);

  // Test equivalentTo.
  // Test situations where they are not equivalent.
  ElapsedTime tight_tol("TDB", Duration(99.9999, "Sec"));
  if (abs_time.equivalentTo(later_time, tight_tol))
    err() << "After AbsoluteTime abs_time(TDB, 51910, 1000.), abs_time.equivalentTo returned true for \"" << later_time <<
      "\" with tolerance of " << tight_tol << ", not false as expected." << std::endl;
  if (later_time.equivalentTo(abs_time, tight_tol))
    err() << "After AbsoluteTime later_time(TDB, 51910, 1100.), later_time.equivalentTo returned true for \"" << abs_time <<
      "\" with tolerance of " << tight_tol << ", not false as expected." << std::endl;

  // Test situations where they are equivalent.
  ElapsedTime loose_tol("TDB", Duration(100., "Sec"));
  if (!(abs_time.equivalentTo(later_time, loose_tol)))
    err() << "After AbsoluteTime abs_time(TDB, 51910, 1000.), abs_time.equivalentTo returned false for \"" << later_time <<
      "\" with tolerance of " << loose_tol << ", not true as expected." << std::endl;
  if (!(later_time.equivalentTo(abs_time, loose_tol)))
    err() << "After AbsoluteTime later_time(TDB, 51910, 1100.), later_time.equivalentTo returned false for \"" << abs_time <<
      "\" with tolerance of " << loose_tol << ", not true as expected." << std::endl;

  // Test subtraction of absolute time from absolute time.
  Duration expected_diff(100., "Sec");
  Duration tolerance(1.e-9, "Sec"); // 1 ns.
  Duration difference = (later_time - abs_time).computeElapsedTime("TDB").getDuration();
  if (!expected_diff.equivalentTo(difference, tolerance))
    err() << "Absolute time [" << abs_time << "] subtracted from absolute time [" << later_time << "] gave " << difference <<
      ", not " << expected_diff << " as expected." << std::endl;

  // Test subtraction of absolute time from absolute time in UTC system
  expected_diff = Duration(.4, "Sec");
  difference = (later_time_utc - abs_time_utc).computeElapsedTime("UTC").getDuration();
  if (!expected_diff.equivalentTo(difference, tolerance))
    err() << "Absolute time [" << abs_time_utc << "] subtracted from absolute time [" << later_time_utc << "] gave " << difference <<
      ", not " << expected_diff << " as expected." << std::endl;
}

void TimeSystemTestApp::testElapsedTime() {
  setMethod("testElapsedTime");

  // Test of the getter that returns a Duration object.
  Duration original_dur = Duration(1, 0., "Day") + Duration(SecPerDay() * 0.125, "Sec");
  Duration expected_dur = original_dur;
  Duration tolerance(1.e-9, "Sec"); // 1 ns.
  ElapsedTime elapsed("TDB", expected_dur);
  Duration returned_dur = elapsed.getDuration();
  if (!returned_dur.equivalentTo(expected_dur, tolerance)) {
    err() << "After ElapsedTime elapsed(\"TDB\", " << original_dur << "), its getDuration() returned " << returned_dur <<
      ", not equivalent to " << expected_dur << " with tolerance of " << tolerance << "." << std::endl;
  }

  // Test of the getter that returns a TimeSystem object.
  std::string returned_system = elapsed.getSystem().getName();
  if (returned_system != "TDB") {
    err() << "After ElapsedTime elapsed(\"TDB\", " << original_dur << "), its getSystem() returned " << returned_system <<
      ", not TDB." << std::endl;
  }

  // Test of negate operator.
  ElapsedTime negative_elapsed = -elapsed;
  expected_dur = Duration(-1, -SecPerDay() * 0.125);
  returned_dur = negative_elapsed.getDuration();
  if (!returned_dur.equivalentTo(expected_dur, tolerance)) {
    err() << "After ElapsedTime negative_elapsed = -elapsed, where elapsed = " << elapsed <<
      ", its getDuration() returned " << returned_dur << ", not equivalent to " << expected_dur <<
      " with tolerance of " << tolerance << "." << std::endl;
  }

  // Test of the getter that returns a double variable.
  double expected_dbl = SecPerDay() * 1.125;
  double returned_dbl = elapsed.getDuration("Sec");
  double tolerance_dbl = 1.e-9; // 1 ns.
  if (std::fabs(expected_dbl - returned_dbl) > tolerance_dbl) {
    err() << "After ElapsedTime elapsed(\"TDB\", " << original_dur << "), its getDuration(\"Sec\") returned " << returned_dbl <<
      ", not equivalent to " << expected_dbl << " with tolerance of " << tolerance_dbl << "." << std::endl;
  }

  // Test of the getter that returns a double variable in the argument list.
  returned_dbl = 0.;
  elapsed.getDuration("Sec", returned_dbl);
  if (std::fabs(expected_dbl - returned_dbl) > tolerance_dbl) {
    err() << "After ElapsedTime elapsed(\"TDB\", " << original_dur << "), its getDuration(\"Sec\", returned_dbl) returned " <<
      "returned_dbl = " << returned_dbl << ", not equivalent to " << expected_dbl << " with tolerance of " << tolerance_dbl <<
      "." << std::endl;
  }

  // Test of the high-precision getter that returns an integer part and a fractional part separately.
  long returned_int = 0;
  double returned_frac = 0.;
  elapsed.getDuration("Day", returned_int, returned_frac);
  long expected_int = 1;
  double expected_frac = .125;
  tolerance_dbl /= SecPerDay(); // Still 1 ns.
  if (expected_int != returned_int || std::fabs(expected_frac - returned_frac) > tolerance_dbl) {
    err() << "After ElapsedTime elapsed(\"TDB\", " << original_dur << "), its getDuration(\"Sec\", int_part, frac_part) returned " <<
      "(int_part, frac_part) = (" <<  returned_int << ", " << returned_frac << "), not equivalent to (" << expected_int <<
      ", " << expected_frac << ") with tolerance of " << tolerance_dbl << "." << std::endl;
  }

  // Test of the high-precision getter that returns an integer part and a fractional part separately, with a negative elapsed time.
  returned_int = 0;
  returned_frac = 0.;
  negative_elapsed.getDuration("Day", returned_int, returned_frac);
  expected_int = -1;
  expected_frac = -.125;
  if (expected_int != returned_int || std::fabs(expected_frac - returned_frac) > tolerance_dbl) {
    err() << "After ElapsedTime negative_elapsed = -elapsed, where elapsed = " << elapsed <<
      ", its getDuration(\"Sec\", int_part, frac_part) returned " <<
      "(int_part, frac_part) = (" <<  returned_int << ", " << returned_frac << "), not equivalent to (" << expected_int <<
      ", " << expected_frac << ") with tolerance of " << tolerance_dbl << "." << std::endl;
  }
}

void TimeSystemTestApp::testTimeInterval() {
  setMethod("testTimeInterval");

  // Create some test inputs.
  AbsoluteTime time1("TDB", 51910, 1000.);
  AbsoluteTime time2("TDB", 51910, 2000.123456789);

  // Test creating a time interval directly from two absolute times.
  TimeInterval interval_a(time1, time2);

  // Test creating a time interval by subtracting two absolute times.
  TimeInterval interval_b = time2 - time1;

  // Compare the durations as computed in the TDB system.
  Duration dur_a = interval_a.computeElapsedTime("TDB").getDuration();
  Duration dur_b = interval_b.computeElapsedTime("TDB").getDuration();

  // Make sure they agree.
  if (dur_a != dur_b) {
    err() << "After creating interval_a and interval_b, they are not the same." << std::endl;
  }

  // Test of the getter that returns a Duration object.
  Duration expected_dur = Duration(1000.123456789, "Sec");
  Duration result_dur = interval_a.computeDuration("TDB");
  Duration tolerance_dur(1.e-9, "Sec"); // 1 ns.
  if (!expected_dur.equivalentTo(result_dur, tolerance_dur)) {
    err() << "TimeInterval(" << time1 << ", " << time2 << ").computeDuration(\"TDB\") returned " << result_dur << ", not " <<
      expected_dur << " with tolerance of " << tolerance_dur << "." << std::endl;
  }

  // Test of the getter that returns a double variable.
  double expected_dbl = 1000.123456789;
  double result_dbl = interval_a.computeDuration("TDB", "Sec");
  double tolerance_dbl = 1.e-9; // 1 ns.
  if (std::fabs(expected_dbl - result_dbl) > tolerance_dbl) {
    err() << "TimeInterval(" << time1 << ", " << time2 << ").computeDuration(\"TDB\", \"Sec\") returned " << result_dbl << ", not " <<
      expected_dbl << " with tolerance of " << tolerance_dbl << "." << std::endl;
  }

  // Test of the getter that returns a double variable in the argument list.
  expected_dbl = 1000.123456789;
  result_dbl = 0.;
  interval_a.computeDuration("TDB", "Sec", result_dbl);
  if (std::fabs(expected_dbl - result_dbl) > tolerance_dbl) {
    err() << "TimeInterval(" << time1 << ", " << time2 << ").computeDuration(\"TDB\", \"Sec\", result_dbl) returned " << result_dbl <<
      ", not " << expected_dbl << " with tolerance of " << tolerance_dbl << "." << std::endl;
  }

  // Test of the high-precision getter that returns an integer part and a fractional part separately.
  long expected_int = 1000;
  double expected_frac = .123456789;
  long result_int = 0;
  double result_frac = 0.;
  interval_a.computeDuration("TDB", "Sec", result_int, result_frac);
  if (expected_int != result_int || std::fabs(expected_frac - result_frac) > tolerance_dbl) {
    err() << "TimeInterval(" << time1 << ", " << time2 << ").computeDuration(\"TDB\", \"Sec\", int_part, frac_part) returned " <<
      "(int_part, frac_part) = " << result_int << ", " << result_frac << "), not (" << expected_int << ", " << expected_frac <<
      ") with tolerance of " << tolerance_dbl << "." << std::endl;
  }

  // Test of the high-precision getter that returns an integer part and a fractional part separately, with a negative elapsed time.
  expected_int = -1000;
  expected_frac = -.123456789;
  result_int = 0;
  result_frac = 0.;
  TimeInterval interval_c(time2, time1);
  interval_c.computeDuration("TDB", "Sec", result_int, result_frac);
  if (expected_int != result_int || std::fabs(expected_frac - result_frac) > tolerance_dbl) {
    err() << "TimeInterval(" << time2 << ", " << time1 << ").computeDuration(\"TDB\", \"Sec\", int_part, frac_part) returned " <<
      "(int_part, frac_part) = " << result_int << ", " << result_frac << "), not (" << expected_int << ", " << expected_frac <<
      ") with tolerance of " << tolerance_dbl << "." << std::endl;
  }
}

void TimeSystemTestApp::testOneCalendarDate(long mjd, long calendar_year, long month, long month_day, long iso_year, long week_number,
  long weekday_number, long ordinal_date) {
  // Test conversion from a calendar date to an MJD.
  const TimeFormat<Calendar> & calendar_format(TimeFormatFactory<Calendar>::getFormat());
  datetime_type datetime = calendar_format.convert(Calendar(calendar_year, month, month_day, 0, 0, 0.));
  if (mjd != datetime.first) {
    err() << "TimeFormat<Calendar>::convert method converted Calendar(" << calendar_year << ", " << month << ", " << month_day <<
      ", 0, 0, 0.) into " << datetime.first << " MJD, not " << mjd << " MJD as expected." << std::endl;
  }

  // Test conversion from an ISO week date to an MJD.
  const TimeFormat<IsoWeek> & iso_week_format(TimeFormatFactory<IsoWeek>::getFormat());
  datetime = iso_week_format.convert(IsoWeek(iso_year, week_number, weekday_number, 0, 0, 0.));
  if (mjd != datetime.first) {
    err() << "TimeFormat<IsoWeek>::convert method converted IsoWeek(" << iso_year << ", " << week_number << ", " << weekday_number <<
      ", 0, 0, 0.) into " << datetime.first << " MJD, not " << mjd << " MJD as expected." << std::endl;
  }

  // Test conversion from an ordinal date to an MJD.
  const TimeFormat<Ordinal> & ordinal_format(TimeFormatFactory<Ordinal>::getFormat());
  datetime = ordinal_format.convert(Ordinal(calendar_year, ordinal_date, 0, 0, 0.));
  if (mjd != datetime.first) {
    err() << "TimeFormat<Ordinal>::convert method converted Ordinal(" << iso_year << ", " << ordinal_date << ", 0, 0, 0.) into " <<
      datetime.first << " MJD, not " << mjd << " MJD as expected." << std::endl;
  }

  // Test conversion from an MJD to a calendar date.
  Calendar result_calendar = calendar_format.convert(datetime_type(mjd, 0.));
  if (calendar_year != result_calendar.m_year || month != result_calendar.m_mon || month_day != result_calendar.m_day) {
    err() << "TimeFormat<Calendar>::convert method converted " << mjd << " MJD into Calendar(" << result_calendar.m_year << ", " <<
      result_calendar.m_mon << ", " << result_calendar.m_day << ", 0, 0, 0.), not Calendar(" << calendar_year << ", " << month <<
      ", " << month_day << ", 0, 0, 0.) as expected." << std::endl;
  }

  // Test conversion from an MJD to an ISO week date.
  IsoWeek result_iso_week = iso_week_format.convert(datetime_type(mjd, 0.));
  if (iso_year != result_iso_week.m_year || week_number != result_iso_week.m_week || weekday_number != result_iso_week.m_day) {
    err() << "TimeFormat<IsoWeek>::convert method converted " << mjd << " MJD into IsoWeek(" << result_iso_week.m_year << ", " <<
      result_iso_week.m_week << ", " << result_iso_week.m_day << ", 0, 0, 0.), not IsoWeek(" << iso_year << ", " << week_number <<
      ", " << weekday_number << ", 0, 0, 0.) as expected." << std::endl;
  }

  // Test conversion from an MJD to an ordinal date.
  Ordinal result_ordinal = ordinal_format.convert(datetime_type(mjd, 0.));
  if (calendar_year != result_ordinal.m_year || ordinal_date != result_ordinal.m_day) {
    err() << "TimeFormat<Ordinal>::convert method converted " << mjd << " MJD into Ordinal(" << result_ordinal.m_year << ", " <<
      result_ordinal.m_day << ", 0, 0, 0.), not Ordinal(" << calendar_year << ", " << ordinal_date << ", 0, 0, 0.) as expected." <<
      std::endl;
  }
}

template <typename TimeRepType>
void TimeSystemTestApp::testOneBadDateTime(const TimeFormat<TimeRepType> & time_format, const datetime_type & datetime,
  const std::string & time_rep_name, const std::string & data_description, bool exception_expected) {
  // Call TimeFormat<TimeRepType>::convert method.
  bool exception_thrown = false;
  try {
    time_format.convert(datetime);
  } catch (const std::exception &) {
    exception_thrown = true;
  }

  // Check if exception thrown/not thrown as expected.
  if (exception_expected != exception_thrown) {
    if (exception_expected) {
      err() << "TimeFormat<" << time_rep_name << ">::convert method did not throw an exception for " << data_description << std::endl;
    } else {
      err() << "TimeFormat<" << time_rep_name << ">::convert method threw an exception for " << data_description << std::endl;
    }
  }
}

template <typename TimeRepType>
void TimeSystemTestApp::testOneBadTimeRep(const TimeFormat<TimeRepType> & time_format, const TimeRepType & time_rep,
  const std::string & time_rep_name, const std::string & data_description, bool exception_expected) {
  // Call TimeFormat<TimeRepType>::convert method.
  bool exception_thrown = false;
  try {
    time_format.convert(time_rep);
  } catch (const std::exception &) {
    exception_thrown = true;
  }

  // Check if exception thrown/not thrown as expected.
  if (exception_expected != exception_thrown) {
    if (exception_expected) {
      err() << "TimeFormat<" << time_rep_name << ">::convert method did not throw an exception for " << data_description << std::endl;
    } else {
      err() << "TimeFormat<" << time_rep_name << ">::convert method threw an exception for " << data_description << std::endl;
    }
  }

  // Call TimeFormat<TimeRepType>::format method.
  exception_thrown = false;
  try {
    time_format.format(time_rep);
  } catch (const std::exception &) {
    exception_thrown = true;
  }

  // Check if exception thrown/not thrown as expected.
  if (exception_expected != exception_thrown) {
    if (exception_expected) {
      err() << "TimeFormat<" << time_rep_name << ">::format method did not throw an exception for " << data_description << std::endl;
    } else {
      err() << "TimeFormat<" << time_rep_name << ">::format method threw an exception for " << data_description << std::endl;
    }
  }
}

template <typename TimeRepType>
void TimeSystemTestApp::testOneBadTimeString(const TimeFormat<TimeRepType> & time_format, const std::string & time_string,
  const std::string & time_rep_name, bool exception_expected) {
  // Call TimeFormat<TimeRepType>::parse method.
  bool exception_thrown = false;
  try {
    time_format.parse(time_string);
  } catch (const std::exception &) {
    exception_thrown = true;
  }

  // Check if exception thrown/not thrown as expected.
  if (exception_expected != exception_thrown) {
    if (exception_expected) {
      err() << "TimeFormat<" << time_rep_name << ">::parse method did not throw an exception for " << time_string << std::endl;
    } else {
      err() << "TimeFormat<" << time_rep_name << ">::parse method threw an exception for " << time_string << std::endl;
    }
  }
}

struct NoSuchTimeRep {};

void TimeSystemTestApp::testTimeFormat() {
  setMethod("testTimeFormat");

  // Test detecting unsupported time representations.
  try {
    TimeFormatFactory<NoSuchTimeRep>::getFormat();
    err() << "TimeFormatFactory<NoSuchTimeRep>::getFormat() did not throw an exception." << std::endl;
  } catch (const std::exception &) {
  }

  // Prepare test parameters for Mjd and Mjd1 classes.
  const TimeFormat<Mjd> & mjd_format(TimeFormatFactory<Mjd>::getFormat());
  const TimeFormat<Mjd1> & mjd1_format(TimeFormatFactory<Mjd1>::getFormat());
  const TimeFormat<Jd> & jd_format(TimeFormatFactory<Jd>::getFormat());
  const TimeFormat<Jd1> & jd1_format(TimeFormatFactory<Jd1>::getFormat());
  datetime_type expected_datetime(51910, 64.814 + SecPerDay() / 2);
  Mjd expected_mjd(expected_datetime.first, expected_datetime.second / SecPerDay());
  Mjd1 expected_mjd1(expected_mjd.m_int + expected_mjd.m_frac);
  Jd expected_jd(expected_mjd.m_int + 2400001, expected_mjd.m_frac - 0.5);
  Jd1 expected_jd1(expected_jd.m_int + expected_jd.m_frac);

  // Test conversion from an Mjd object (that holds integer part and fractional part of MJD) to a datetime_type object.
  datetime_type datetime = mjd_format.convert(expected_mjd);
  double tolerance = 100.e-9; // 100 nanoseconds.
  if (expected_datetime.first != datetime.first || tolerance < std::fabs(expected_datetime.second - datetime.second)) {
    err() << "TimeFormat<Mjd>::convert method converted (" << expected_mjd.m_int << " + " << expected_mjd.m_frac <<
      ") MJD into datetime_type pair (" << datetime.first << ", " << datetime.second << "), not (" <<
      expected_datetime.first << ", " << expected_datetime.second << ") as expected." << std::endl;
  }

  // Test conversion from an Mjd1 object (that holds a single MJD number of double type) to a datetime_type object.
  datetime = mjd_format.convert(expected_mjd);
  tolerance = 10.e-6; // 10 microseconds.
  if (expected_datetime.first != datetime.first || tolerance < std::fabs(expected_datetime.second - datetime.second)) {
    err() << "TimeFormat<Mjd>::convert method converted " << expected_mjd1.m_day << " MJD into datetime_type pair (" <<
      datetime.first << ", " << datetime.second << "), not (" << expected_datetime.first << ", " << expected_datetime.second <<
      ") as expected." << std::endl;
  }

  // Test conversion from an Jd object (that holds integer part and fractional part of JD) to a datetime_type object.
  datetime = jd_format.convert(expected_jd);
  tolerance = 100.e-9; // 100 nanoseconds.
  if (expected_datetime.first != datetime.first || tolerance < std::fabs(expected_datetime.second - datetime.second)) {
    err() << "TimeFormat<Jd>::convert method converted (" << expected_jd.m_int << " + " << expected_jd.m_frac <<
      ") JD into datetime_type pair (" << datetime.first << ", " << datetime.second << "), not (" <<
      expected_datetime.first << ", " << expected_datetime.second << ") as expected." << std::endl;
  }

  // Test conversion from an Jd1 object (that holds a single JD number of double type) to a datetime_type object.
  datetime = jd_format.convert(expected_jd);
  tolerance = 10.e-6; // 10 microseconds.
  if (expected_datetime.first != datetime.first || tolerance < std::fabs(expected_datetime.second - datetime.second)) {
    err() << "TimeFormat<Jd>::convert method converted " << expected_jd1.m_day << " JD into datetime_type pair (" <<
      datetime.first << ", " << datetime.second << "), not (" << expected_datetime.first << ", " << expected_datetime.second <<
      ") as expected." << std::endl;
  }

  // Test conversion from a datetime_type object to an Mjd object (that holds integer part and fractional part of MJD).
  Mjd result_mjd = mjd_format.convert(expected_datetime);
  tolerance = 100.e-9 / SecPerDay(); // 100 nanoseconds in units of day.
  if (expected_mjd.m_int != result_mjd.m_int || tolerance < std::fabs(expected_mjd.m_frac - result_mjd.m_frac)) {
    err() << "TimeFormat<Mjd>::convert method converted datetime_type pair (" << expected_datetime.first << ", " <<
      expected_datetime.second << ") into (" << result_mjd.m_int << " + " << result_mjd.m_frac << ") MJD, not (" <<
      expected_mjd.m_int << " + " << expected_mjd.m_frac << ") MJD as expected." << std::endl;
  }

  // Test conversion from a datetime_type object to an Mjd1 object (that holds a single MJD number of double type).
  Mjd1 result_mjd1 = mjd1_format.convert(expected_datetime);
  tolerance = 100.e-9 / SecPerDay(); // 100 nanoseconds in units of day.
  if (tolerance < std::fabs(expected_mjd1.m_day - result_mjd1.m_day)) {
    err() << "TimeFormat<Mjd1>::convert method converted datetime_type pair (" << expected_datetime.first << ", " <<
      expected_datetime.second << ") into " << result_mjd1.m_day << " MJD, not " << expected_mjd1.m_day << " MJD as expected." <<
      std::endl;
  }

  // Test conversion from a datetime_type object to an Jd object (that holds integer part and fractional part of JD).
  Jd result_jd = jd_format.convert(expected_datetime);
  tolerance = 100.e-9 / SecPerDay(); // 100 nanoseconds in units of day.
  if (expected_jd.m_int != result_jd.m_int || tolerance < std::fabs(expected_jd.m_frac - result_jd.m_frac)) {
    err() << "TimeFormat<Jd>::convert method converted datetime_type pair (" << expected_datetime.first << ", " <<
      expected_datetime.second << ") into (" << result_jd.m_int << " + " << result_jd.m_frac << ") JD, not (" <<
      expected_jd.m_int << " + " << expected_jd.m_frac << ") JD as expected." << std::endl;
  }

  // Test conversion from a datetime_type object to an Jd1 object (that holds a single JD number of double type).
  Jd1 result_jd1 = jd1_format.convert(expected_datetime);
  tolerance = 100.e-9 / SecPerDay(); // 100 nanoseconds in units of day.
  if (tolerance < std::fabs(expected_jd1.m_day - result_jd1.m_day)) {
    err() << "TimeFormat<Jd1>::convert method converted datetime_type pair (" << expected_datetime.first << ", " <<
      expected_datetime.second << ") into " << result_jd1.m_day << " JD, not " << expected_jd1.m_day << " JD as expected." <<
      std::endl;
  }

  // Test formatting into string with a TimeFormat<Mjd> object.
  std::string test_mjd_string = "51910.500750162037037";
  std::string expected_mjd_string = test_mjd_string + " MJD";
  std::string result_mjd_string = mjd_format.format(expected_mjd);
  if (expected_mjd_string != result_mjd_string) {
    err() << "TimeFormat<Mjd>::format method formatted (" << expected_mjd.m_int << " + " << expected_mjd.m_frac << ") MJD into \"" <<
      result_mjd_string << "\", not \"" << expected_mjd_string << "\" as expected." << std::endl;
  }

  // Test formatting into string with a TimeFormat<Mjd> object, with decimal precision specified.
  expected_mjd_string = "51910.5007502 MJD";
  result_mjd_string = mjd_format.format(expected_mjd, 7);
  if (expected_mjd_string != result_mjd_string) {
    err() << "TimeFormat<Mjd>::format method formatted (" << expected_mjd.m_int << " + " << expected_mjd.m_frac << ") MJD into \"" <<
      result_mjd_string << "\", not \"" << expected_mjd_string << "\" as expected." << std::endl;
  }

  // Test parsing a string with a TimeFormat<Mjd> object.
  result_mjd = mjd_format.parse(test_mjd_string);
  tolerance = 100.e-9 / SecPerDay(); // 100 nanoseconds in units of day.
  if (expected_mjd.m_int != result_mjd.m_int || tolerance < std::fabs(expected_mjd.m_frac - result_mjd.m_frac)) {
    err() << "TimeFormat<Mjd>::parse method parsed \"" << test_mjd_string << "\" into (" << result_mjd.m_int << " + " <<
      result_mjd.m_frac << ") MJD, not (" << expected_mjd.m_int << " + " << expected_mjd.m_frac << ") MJD as expected." << std::endl;
  }

  // Test formatting into string with a TimeFormat<Mjd1> object, with decimal precision specified.
  // Note: Need to specify the number of digits to avoid failing this test due to unimportant rouding errors.
  expected_mjd_string = "51910.5007502 MJD";
  result_mjd_string = mjd1_format.format(expected_mjd1, 7);
  if (expected_mjd_string != result_mjd_string) {
    err() << "TimeFormat<Mjd1>::format method formatted " << expected_mjd1.m_day << " MJD into \"" << result_mjd_string <<
      "\", not \"" << expected_mjd_string << "\" as expected." << std::endl;
  }

  // Test parsing a string with a TimeFormat<Mjd1> object.
  result_mjd1 = mjd1_format.parse(test_mjd_string);
  tolerance = 10.e-6 / SecPerDay(); // 10 microseconds in units of day.
  if (tolerance < std::fabs(expected_mjd1.m_day - result_mjd1.m_day)) {
    err() << "TimeFormat<Mjd1>::parse method parsed \"" << test_mjd_string << "\" into " << result_mjd1.m_day << " MJD, not " <<
      expected_mjd1.m_day << " MJD as expected." << std::endl;
  }

  // Test formatting into string with a TimeFormat<Jd> object.
  std::string test_jd_string = "2451911.000750162037037";
  std::string expected_jd_string = test_jd_string + " JD";
  std::string result_jd_string = jd_format.format(expected_jd);
  if (expected_jd_string != result_jd_string) {
    err() << "TimeFormat<Jd>::format method formatted (" << expected_jd.m_int << " + " << expected_jd.m_frac << ") JD into \"" <<
      result_jd_string << "\", not \"" << expected_jd_string << "\" as expected." << std::endl;
  }

  // Test formatting into string with a TimeFormat<Jd> object, with decimal precision specified.
  expected_jd_string = "2451911.0007502 JD";
  result_jd_string = jd_format.format(expected_jd, 7);
  if (expected_jd_string != result_jd_string) {
    err() << "TimeFormat<Jd>::format method formatted (" << expected_jd.m_int << " + " << expected_jd.m_frac << ") JD into \"" <<
      result_jd_string << "\", not \"" << expected_jd_string << "\" as expected." << std::endl;
  }

  // Test parsing a string with a TimeFormat<Jd> object.
  result_jd = jd_format.parse(test_jd_string);
  tolerance = 100.e-9 / SecPerDay(); // 100 nanoseconds in units of day.
  if (expected_jd.m_int != result_jd.m_int || tolerance < std::fabs(expected_jd.m_frac - result_jd.m_frac)) {
    err() << "TimeFormat<Jd>::parse method parsed \"" << test_jd_string << "\" into (" << result_jd.m_int << " + " <<
      result_jd.m_frac << ") JD, not (" << expected_jd.m_int << " + " << expected_jd.m_frac << ") JD as expected." << std::endl;
  }

  // Test formatting into string with a TimeFormat<Jd1> object, with decimal precision specified.
  // Note: Need to specify the number of digits to avoid failing this test due to unimportant rouding errors.
  expected_jd_string = "2451911.0007502 JD";
  result_jd_string = jd1_format.format(expected_jd1, 7);
  if (expected_jd_string != result_jd_string) {
    err() << "TimeFormat<Jd1>::format method formatted " << expected_jd1.m_day << " JD into \"" << result_jd_string <<
      "\", not \"" << expected_jd_string << "\" as expected." << std::endl;
  }

  // Test parsing a string with a TimeFormat<Jd1> object.
  result_jd1 = jd1_format.parse(test_jd_string);
  tolerance = 10.e-6 / SecPerDay(); // 10 microseconds in units of day.
  if (tolerance < std::fabs(expected_jd1.m_day - result_jd1.m_day)) {
    err() << "TimeFormat<Jd1>::parse method parsed \"" << test_jd_string << "\" into " << result_jd1.m_day << " JD, not " <<
      expected_jd1.m_day << " JD as expected." << std::endl;
  }

  // Test detections of bad times of the day.
  testOneBadDateTime(mjd_format, datetime_type(51910, -0.001), "Mjd", "a time of the day: -0.001");
  testOneBadDateTime(mjd_format, datetime_type(51910, SecPerDay() + 0.001), "Mjd", "a time of the day: 86400.001");
  testOneBadDateTime(mjd1_format, datetime_type(51910, -0.001), "Mjd1", "a time of the day: -0.001");
  testOneBadDateTime(mjd1_format, datetime_type(51910, SecPerDay() + 0.001), "Mjd1", "a time of the day: 86400.001");
  testOneBadDateTime(jd_format, datetime_type(51910, -0.001), "Jd", "a time of the day: -0.001");
  testOneBadDateTime(jd_format, datetime_type(51910, SecPerDay() + 0.001), "Jd", "a time of the day: 86400.001");
  testOneBadDateTime(jd1_format, datetime_type(51910, -0.001), "Jd1", "a time of the day: -0.001");
  testOneBadDateTime(jd1_format, datetime_type(51910, SecPerDay() + 0.001), "Jd1", "a time of the day: 86400.001");

  // Test detections of bad MJD numbers.
  testOneBadTimeRep(mjd_format, Mjd(+1, -.001), "Mjd", "Mjd(+1, -0.001)");
  testOneBadTimeRep(mjd_format, Mjd(+1, +1.), "Mjd", "Mjd(+1, +1.)");
  testOneBadTimeRep(mjd_format, Mjd(0, +1.), "Mjd", "Mjd(0, +1.)");
  testOneBadTimeRep(mjd_format, Mjd(0, -1.), "Mjd", "Mjd(0, -1.)");
  testOneBadTimeRep(mjd_format, Mjd(-1, +.001), "Mjd", "Mjd(-1, +0.001)");
  testOneBadTimeRep(mjd_format, Mjd(-1, -1.), "Mjd", "Mjd(-1, -1.)");

  // Test detections of bad JD numbers.
  testOneBadTimeRep(jd_format, Jd(+1, -.001), "Jd", "Jd(+1, -0.001)");
  testOneBadTimeRep(jd_format, Jd(+1, +1.), "Jd", "Jd(+1, +1.)");
  testOneBadTimeRep(jd_format, Jd(0, +1.), "Jd", "Jd(0, +1.)");
  testOneBadTimeRep(jd_format, Jd(0, -1.), "Jd", "Jd(0, -1.)");
  testOneBadTimeRep(jd_format, Jd(-1, +.001), "Jd", "Jd(-1, +0.001)");
  testOneBadTimeRep(jd_format, Jd(-1, -1.), "Jd", "Jd(-1, -1.)");

  // Test detections of bad MJD/JD strings.
  testOneBadTimeString(mjd_format, "Not A Number", "Mjd");
  testOneBadTimeString(mjd1_format, "Not A Number", "Mjd1");
  testOneBadTimeString(jd_format, "Not A Number", "Jd");
  testOneBadTimeString(jd1_format, "Not A Number", "Jd1");

  // Prepare test parameters for Calendar, IsoWeek, and Ordinal classes.
  const TimeFormat<Calendar> & calendar_format(TimeFormatFactory<Calendar>::getFormat());
  const TimeFormat<IsoWeek> & iso_week_format(TimeFormatFactory<IsoWeek>::getFormat());
  const TimeFormat<Ordinal> & ordinal_format(TimeFormatFactory<Ordinal>::getFormat());
  expected_datetime = datetime_type(54634, 45296.789);
  Calendar expected_calendar(2008, 6, 17, 12, 34, 56.789);
  IsoWeek expected_iso_week(2008, 25, 2, 12, 34, 56.789);
  Ordinal expected_ordinal(2008, 169, 12, 34, 56.789);
  tolerance = 100.e-9; // 100 nanoseconds.

  // Test conversion from a Calendar object to a datetime_type object.
  datetime = calendar_format.convert(expected_calendar);
  if (expected_datetime.first != datetime.first || tolerance < std::fabs(expected_datetime.second - datetime.second)) {
    err() << "TimeFormat<Calendar>::convert method converted Calendar(" << expected_calendar.m_year << ", " <<
      expected_calendar.m_mon << ", " << expected_calendar.m_day << ", " << expected_calendar.m_hour << ", " <<
      expected_calendar.m_min << ", " << expected_calendar.m_sec << ") into datetime_type pair (" << datetime.first << ", " <<
      datetime.second << "), not (" << expected_datetime.first << ", " << expected_datetime.second << ") as expected." << std::endl;
  }

  // Test conversion from an IsoWeek object to a datetime_type object.
  datetime = iso_week_format.convert(expected_iso_week);
  if (expected_datetime.first != datetime.first || tolerance < std::fabs(expected_datetime.second - datetime.second)) {
    err() << "TimeFormat<IsoWeek>::convert method converted IsoWeek(" << expected_iso_week.m_year << ", " <<
      expected_iso_week.m_week << ", " << expected_iso_week.m_day << ", " << expected_iso_week.m_hour << ", " <<
      expected_iso_week.m_min << ", " << expected_iso_week.m_sec << ") into datetime_type pair (" << datetime.first << ", " <<
      datetime.second << "), not (" << expected_datetime.first << ", " << expected_datetime.second << ") as expected." << std::endl;
  }

  // Test conversion from an Ordinal object to a datetime_type object.
  datetime = ordinal_format.convert(expected_ordinal);
  if (expected_datetime.first != datetime.first || tolerance < std::fabs(expected_datetime.second - datetime.second)) {
    err() << "TimeFormat<Ordinal>::convert method converted Ordinal(" << expected_ordinal.m_year << ", " <<
      expected_ordinal.m_day << ", " << expected_ordinal.m_hour << ", " << expected_ordinal.m_min << ", " << expected_ordinal.m_sec <<
      ") into datetime_type pair (" << datetime.first << ", " << datetime.second << "), not (" << expected_datetime.first << ", " <<
      expected_datetime.second << ") as expected." << std::endl;
  }

  // Test conversion from a datetime_type object to a Calendar object.
  Calendar result_calendar = calendar_format.convert(expected_datetime);
  if (expected_calendar.m_year != result_calendar.m_year || expected_calendar.m_mon != result_calendar.m_mon ||
      expected_calendar.m_day != result_calendar.m_day || expected_calendar.m_hour != result_calendar.m_hour ||
      expected_calendar.m_min != result_calendar.m_min || tolerance < std::fabs(expected_calendar.m_sec - result_calendar.m_sec)) {
    err() << "TimeFormat<Calendar>::convert method converted datetime_type pair (" << expected_datetime.first << ", " <<
      expected_datetime.second << ") into Calendar(" << result_calendar.m_year << ", " << result_calendar.m_mon << ", " <<
      result_calendar.m_day << ", " << result_calendar.m_hour << ", " << result_calendar.m_min << ", " << result_calendar.m_sec <<
      "), not Calendar(" << expected_calendar.m_year << ", " << expected_calendar.m_mon << ", " << expected_calendar.m_day << ", " <<
      expected_calendar.m_hour << ", " << expected_calendar.m_min << ", " << expected_calendar.m_sec << ") as expected." << std::endl;
  }

  // Test conversion from a datetime_type object to an IsoWeek object.
  IsoWeek result_iso_week = iso_week_format.convert(expected_datetime);
  if (expected_iso_week.m_year != result_iso_week.m_year || expected_iso_week.m_week != result_iso_week.m_week ||
      expected_iso_week.m_day != result_iso_week.m_day || expected_iso_week.m_hour != result_iso_week.m_hour ||
      expected_iso_week.m_min != result_iso_week.m_min || tolerance < std::fabs(expected_iso_week.m_sec - result_iso_week.m_sec)) {
    err() << "TimeFormat<IsoWeek>::convert method converted datetime_type pair (" << expected_datetime.first << ", " <<
      expected_datetime.second << ") into IsoWeek(" << result_iso_week.m_year << ", " << result_iso_week.m_week << ", " <<
      result_iso_week.m_day << ", " << result_iso_week.m_hour << ", " << result_iso_week.m_min << ", " << result_iso_week.m_sec <<
      "), not IsoWeek(" << expected_iso_week.m_year << ", " << expected_iso_week.m_week << ", " << expected_iso_week.m_day << ", " <<
      expected_iso_week.m_hour << ", " << expected_iso_week.m_min << ", " << expected_iso_week.m_sec << ") as expected." << std::endl;
  }

  // Test conversion from a datetime_type object to an Ordinal object.
  Ordinal result_ordinal = ordinal_format.convert(expected_datetime);
  if (expected_ordinal.m_year != result_ordinal.m_year || expected_ordinal.m_day != result_ordinal.m_day ||
      expected_ordinal.m_hour != result_ordinal.m_hour || expected_ordinal.m_min != result_ordinal.m_min ||
      tolerance < std::fabs(expected_ordinal.m_sec - result_ordinal.m_sec)) {
    err() << "TimeFormat<Ordinal>::convert method converted datetime_type pair (" << expected_datetime.first << ", " <<
      expected_datetime.second << ") into Ordinal(" << result_ordinal.m_year << ", " << result_ordinal.m_day << ", " <<
      result_ordinal.m_hour << ", " << result_ordinal.m_min << ", " << result_ordinal.m_sec << "), not Ordinal(" <<
      expected_ordinal.m_year << ", " << expected_ordinal.m_day << ", " << expected_ordinal.m_hour << ", " <<
      expected_ordinal.m_min << ", " << expected_ordinal.m_sec << ") as expected." << std::endl;
  }

  // Test conversion from a datetime_type object to a Calendar object, during an inserted leap second.
  result_calendar = calendar_format.convert(datetime_type(expected_datetime.first, SecPerDay() + 0.3));
  if (expected_calendar.m_year != result_calendar.m_year || expected_calendar.m_mon != result_calendar.m_mon ||
      expected_calendar.m_day != result_calendar.m_day || 23 != result_calendar.m_hour ||
      59 != result_calendar.m_min || tolerance < std::fabs(60.3 - result_calendar.m_sec)) {
    err() << "TimeFormat<Calendar>::convert method converted datetime_type pair (" << expected_datetime.first << ", " <<
      SecPerDay() + 0.3 << ") into Calendar(" << result_calendar.m_year << ", " << result_calendar.m_mon << ", " <<
      result_calendar.m_day << ", " << result_calendar.m_hour << ", " << result_calendar.m_min << ", " << result_calendar.m_sec <<
      "), not Calendar(" << expected_calendar.m_year << ", " << expected_calendar.m_mon << ", " << expected_calendar.m_day <<
      ", 23, 59, 60.3) as expected." << std::endl;
  }

  // Test conversion from a datetime_type object to an IsoWeek object, during an inserted leap second.
  result_iso_week = iso_week_format.convert(datetime_type(expected_datetime.first, SecPerDay() + 0.3));
  if (expected_iso_week.m_year != result_iso_week.m_year || expected_iso_week.m_week != result_iso_week.m_week ||
      expected_iso_week.m_day != result_iso_week.m_day || 23 != result_iso_week.m_hour ||
      59 != result_iso_week.m_min || tolerance < std::fabs(60.3 - result_iso_week.m_sec)) {
    err() << "TimeFormat<IsoWeek>::convert method converted datetime_type pair (" << expected_datetime.first << ", " <<
      SecPerDay() + 0.3 << ") into IsoWeek(" << result_iso_week.m_year << ", " << result_iso_week.m_week << ", " <<
      result_iso_week.m_day << ", " << result_iso_week.m_hour << ", " << result_iso_week.m_min << ", " << result_iso_week.m_sec <<
      "), not IsoWeek(" << expected_iso_week.m_year << ", " << expected_iso_week.m_week << ", " << expected_iso_week.m_day <<
      ", 23, 59, 60.3) as expected." << std::endl;
  }

  // Test conversion from a datetime_type object to an Ordinal object, during an inserted leap second.
  result_ordinal = ordinal_format.convert(datetime_type(expected_datetime.first, SecPerDay() + 0.3));
  if (expected_ordinal.m_year != result_ordinal.m_year || expected_ordinal.m_day != result_ordinal.m_day ||
      23 != result_ordinal.m_hour || 59 != result_ordinal.m_min || tolerance < std::fabs(60.3 - result_ordinal.m_sec)) {
    err() << "TimeFormat<Ordinal>::convert method converted datetime_type pair (" << expected_datetime.first << ", " <<
      SecPerDay() + 0.3 << ") into Ordinal(" << result_ordinal.m_year << ", " << result_ordinal.m_day << ", " <<
      result_ordinal.m_hour << ", " << result_ordinal.m_min << ", " << result_ordinal.m_sec << "), not Ordinal(" <<
      expected_ordinal.m_year << ", " << expected_ordinal.m_day << ", 23, 59, 60.3) as expected." << std::endl;
  }

  // Test formatting into string.
  // Note: Need to specify the number of digits to avoid failing this test due to unimportant rouding errors.
  std::string expected_calendar_string = "2008-06-17T12:34:56.789";
  std::string result_calendar_string = calendar_format.format(expected_calendar, 3);
  if (expected_calendar_string != result_calendar_string) {
    err() << "TimeFormat<Calenadr>::format method formatted Calendar(" << expected_calendar.m_year << ", " <<
      expected_calendar.m_mon << ", " << expected_calendar.m_day << ", " << expected_calendar.m_hour << ", " <<
      expected_calendar.m_min << ", " << expected_calendar.m_sec << ") into \"" << result_calendar_string << "\", not \"" <<
      expected_calendar_string << "\" as expected." << std::endl;
  }

  // Test formatting into string, which should include leading zeros.
  expected_calendar_string = "2008-06-17T00:00:00.0";
  result_calendar_string = calendar_format.format(Calendar(expected_calendar.m_year, expected_calendar.m_mon, expected_calendar.m_day,
    0, 0, 0.), 1);
  if (expected_calendar_string != result_calendar_string) {
    err() << "TimeFormat<Calenadr>::format method formatted Calendar(" << expected_calendar.m_year << ", " <<
      expected_calendar.m_mon << ", " << expected_calendar.m_day << ", 0, 0, 0.) into \"" << result_calendar_string <<
      "\", not \"" << expected_calendar_string << "\" as expected." << std::endl;
  }

  // Test parsing a string.
  expected_calendar_string = "2008-06-17T12:34:56.789";
  result_calendar = calendar_format.parse(expected_calendar_string);
  tolerance = 100.e-9; // 100 nanoseconds.
  if (expected_calendar.m_year != result_calendar.m_year || expected_calendar.m_mon != result_calendar.m_mon ||
      expected_calendar.m_day != result_calendar.m_day || expected_calendar.m_hour != result_calendar.m_hour ||
      expected_calendar.m_min != result_calendar.m_min || tolerance < std::fabs(expected_calendar.m_sec - result_calendar.m_sec)) {
    err() << "TimeFormat<Calendar>::parse method parsed \"" << expected_calendar_string << "\" into Calendar(" <<
      result_calendar.m_year << ", " << result_calendar.m_mon << ", " << result_calendar.m_day << ", " <<
      result_calendar.m_hour << ", " << result_calendar.m_min << ", " << result_calendar.m_sec << "), not Calendar(" <<
      expected_calendar.m_year << ", " << expected_calendar.m_mon << ", " << expected_calendar.m_day << ", " <<
      expected_calendar.m_hour << ", " << expected_calendar.m_min << ", " << expected_calendar.m_sec << ") as expected." << std::endl;
  }

  // Test formatting into string.
  // Note: Need to specify the number of digits to avoid failing this test due to unimportant rouding errors.
  std::string expected_iso_week_string = "2008-W25-2T12:34:56.789";
  std::string result_iso_week_string = iso_week_format.format(expected_iso_week, 3);
  if (expected_iso_week_string != result_iso_week_string) {
    err() << "TimeFormat<IsoWeek>::format method formatted IsoWeek(" << expected_iso_week.m_year << ", " <<
      expected_iso_week.m_week << ", " << expected_iso_week.m_day << ", " << expected_iso_week.m_hour << ", " <<
      expected_iso_week.m_min << ", " << expected_iso_week.m_sec << ") into \"" << result_iso_week_string << "\", not \"" <<
      expected_iso_week_string << "\" as expected." << std::endl;
  }

  // Test formatting into string, which should include leading zeros.
  expected_iso_week_string = "2008-W25-2T00:00:00.0";
  result_iso_week_string = iso_week_format.format(IsoWeek(expected_iso_week.m_year, expected_iso_week.m_week,
    expected_iso_week.m_day, 0, 0, 0.), 1);
  if (expected_iso_week_string != result_iso_week_string) {
    err() << "TimeFormat<IsoWeek>::format method formatted IsoWeek(" << expected_iso_week.m_year << ", " <<
      expected_iso_week.m_week << ", " << expected_iso_week.m_day << ", 0, 0, 0.) into \"" << result_iso_week_string <<
      "\", not \"" << expected_iso_week_string << "\" as expected." << std::endl;
  }

  // Test parsing a string.
  expected_iso_week_string = "2008-W25-2T12:34:56.789";
  result_iso_week = iso_week_format.parse(expected_iso_week_string);
  tolerance = 100.e-9; // 100 nanoseconds.
  if (expected_iso_week.m_year != result_iso_week.m_year || expected_iso_week.m_week != result_iso_week.m_week ||
      expected_iso_week.m_day != result_iso_week.m_day || expected_iso_week.m_hour != result_iso_week.m_hour ||
      expected_iso_week.m_min != result_iso_week.m_min || tolerance < std::fabs(expected_iso_week.m_sec - result_iso_week.m_sec)) {
    err() << "TimeFormat<IsoWeek>::parse method parsed \"" << expected_iso_week_string << "\" into IsoWeek(" <<
      result_iso_week.m_year << ", " << result_iso_week.m_week << ", " << result_iso_week.m_day << ", " <<
      result_iso_week.m_hour << ", " << result_iso_week.m_min << ", " << result_iso_week.m_sec << "), not IsoWeek(" <<
      expected_iso_week.m_year << ", " << expected_iso_week.m_week << ", " << expected_iso_week.m_day << ", " <<
      expected_iso_week.m_hour << ", " << expected_iso_week.m_min << ", " << expected_iso_week.m_sec << ") as expected." << std::endl;
  }

  // Test formatting into string.
  // Note: Need to specify the number of digits to avoid failing this test due to unimportant rouding errors.
  std::string expected_ordinal_string = "2008-169T12:34:56.789";
  std::string result_ordinal_string = ordinal_format.format(expected_ordinal, 3);
  if (expected_ordinal_string != result_ordinal_string) {
    err() << "TimeFormat<Ordinal>::format method formatted Ordinal(" << expected_ordinal.m_year << ", " <<
      expected_ordinal.m_day << ", " << expected_ordinal.m_hour << ", " << expected_ordinal.m_min << ", " <<
      expected_ordinal.m_sec << ") into \"" << result_ordinal_string << "\", not \"" << expected_ordinal_string <<
      "\" as expected." << std::endl;
  }

  // Test formatting into string, which should include leading zeros.
  expected_ordinal_string = "2008-169T00:00:00.0";
  result_ordinal_string = ordinal_format.format(Ordinal(expected_ordinal.m_year, expected_ordinal.m_day, 0, 0, 0.), 1);
  if (expected_ordinal_string != result_ordinal_string) {
    err() << "TimeFormat<Ordinal>::format method formatted Ordinal(" << expected_ordinal.m_year << ", " <<
      expected_ordinal.m_day << ", 0, 0, 0.) into \"" << result_ordinal_string << "\", not \"" << expected_ordinal_string <<
      "\" as expected." << std::endl;
  }

  // Test parsing a string.
  expected_ordinal_string = "2008-169T12:34:56.789";
  result_ordinal = ordinal_format.parse(expected_ordinal_string);
  tolerance = 100.e-9; // 100 nanoseconds.
  if (expected_ordinal.m_year != result_ordinal.m_year || expected_ordinal.m_day != result_ordinal.m_day ||
      expected_ordinal.m_hour != result_ordinal.m_hour || expected_ordinal.m_min != result_ordinal.m_min ||
      tolerance < std::fabs(expected_ordinal.m_sec - result_ordinal.m_sec)) {
    err() << "TimeFormat<Ordinal>::parse method parsed \"" << expected_ordinal_string << "\" into Ordinal(" <<
      result_ordinal.m_year << ", " << result_ordinal.m_day << ", " <<
      result_ordinal.m_hour << ", " << result_ordinal.m_min << ", " << result_ordinal.m_sec << "), not Ordinal(" <<
      expected_ordinal.m_year << ", " << expected_ordinal.m_day << ", " <<
      expected_ordinal.m_hour << ", " << expected_ordinal.m_min << ", " << expected_ordinal.m_sec << ") as expected." << std::endl;
  }

  // Test detections of bad times of the day.
  testOneBadDateTime(calendar_format, datetime_type(51910, -0.001), "Calendar", "a time of the day: -0.001");
  testOneBadDateTime(iso_week_format, datetime_type(51910, -0.001), "IsoWeek", "a time of the day: -0.001");
  testOneBadDateTime(ordinal_format, datetime_type(51910, -0.001), "Ordinal", "a time of the day: -0.001");

  // Test detections of non-existing month.
  testOneBadTimeRep(calendar_format, Calendar(2008,  0, 1, 0, 0, 0.), "Calendar", "a bad calendar month: 0.");
  testOneBadTimeRep(calendar_format, Calendar(2008, 13, 1, 0, 0, 0.), "Calendar", "a bad calendar month: 13.");

  testOneBadTimeString(calendar_format, "2008-00-01T00:00:00.0", "Calendar");
  testOneBadTimeString(calendar_format, "2008-13-01T00:00:00.0", "Calendar");

  // Test detections of non-existing day of month.
  testOneBadTimeRep(calendar_format, Calendar(2008, 1,  0, 0, 0, 0.), "Calendar", "a non-existing calendar date: 2008-01-00.");
  testOneBadTimeRep(calendar_format, Calendar(2008, 1, 32, 0, 0, 0.), "Calendar", "a non-existing calendar date: 2008-01-32.");
  testOneBadTimeRep(calendar_format, Calendar(2008, 1, 31, 0, 0, 0.), "Calendar", "an existing calendar date: 2008-01-31.", false);
  testOneBadTimeRep(calendar_format, Calendar(2008, 4, 31, 0, 0, 0.), "Calendar", "a non-existing calendar date: 2008-04-31.");

  testOneBadTimeString(calendar_format, "2008-01-00T00:00:00.0", "Calendar");
  testOneBadTimeString(calendar_format, "2008-01-32T00:00:00.0", "Calendar");
  testOneBadTimeString(calendar_format, "2008-01-31T00:00:00.0", "Calendar", false);
  testOneBadTimeString(calendar_format, "2008-04-31T00:00:00.0", "Calendar");

  // Test detections of non-existing day near the end of February.
  testOneBadTimeRep(calendar_format, Calendar(2008, 2, 30, 0, 0, 0.), "Calendar", "a non-existing calendar date: 2008-02-30.");
  testOneBadTimeRep(calendar_format, Calendar(2008, 2, 29, 0, 0, 0.), "Calendar", "an existing calendar date: 2008-02-29.", false);
  testOneBadTimeRep(calendar_format, Calendar(2009, 2, 29, 0, 0, 0.), "Calendar", "a non-existing calendar date: 2009-02-29.");
  testOneBadTimeRep(calendar_format, Calendar(2100, 2, 29, 0, 0, 0.), "Calendar", "a non-existing calendar date: 2100-02-29.");
  testOneBadTimeRep(calendar_format, Calendar(2000, 2, 29, 0, 0, 0.), "Calendar", "an existing calendar date: 2000-02-29.", false);

  testOneBadTimeString(calendar_format, "2008-02-30T00:00:00.0", "Calendar");
  testOneBadTimeString(calendar_format, "2008-02-29T00:00:00.0", "Calendar", false);
  testOneBadTimeString(calendar_format, "2009-02-29T00:00:00.0", "Calendar");
  testOneBadTimeString(calendar_format, "2100-02-29T00:00:00.0", "Calendar");
  testOneBadTimeString(calendar_format, "2000-02-29T00:00:00.0", "Calendar", false);

  // Test detections of non-existing time.
  testOneBadTimeRep(calendar_format, Calendar(2008, 6, 17, -1,  0,  0.), "Calendar", "a non-existing hour: -1.");
  testOneBadTimeRep(calendar_format, Calendar(2008, 6, 17, 24,  0,  0.), "Calendar", "a non-existing hour: 24.");
  testOneBadTimeRep(calendar_format, Calendar(2008, 6, 17,  0, -1,  0.), "Calendar", "a non-existing minute: -1.");
  testOneBadTimeRep(calendar_format, Calendar(2008, 6, 17,  0, 60,  0.), "Calendar", "a non-existing minute: 60.");
  testOneBadTimeRep(calendar_format, Calendar(2008, 6, 17,  0,  0, -1.), "Calendar", "a non-existing second: -1.");

  testOneBadTimeString(calendar_format, "2008-06-17T-1:00:00.0", "Calendar");
  testOneBadTimeString(calendar_format, "2008-06-17T24:00:00.0", "Calendar");
  testOneBadTimeString(calendar_format, "2008-06-17T00:-1:00.0", "Calendar");
  testOneBadTimeString(calendar_format, "2008-06-17T00:60:00.0", "Calendar");
  testOneBadTimeString(calendar_format, "2008-06-17T00:00:-1.0", "Calendar");
  // Note: TimeFormat<Calendar> cannot detect the second part exceeding its maximum because of a possible leap second insertion,
  //       and TimeSystem class should test it instead.

  // Test detections of non-existing week number.
  testOneBadTimeRep(iso_week_format, IsoWeek(2008,  0, 1, 0,  0,  0.), "IsoWeek", "a non-existing week number: 0.");
  testOneBadTimeRep(iso_week_format, IsoWeek(2008, 53, 1, 0,  0,  0.), "IsoWeek", "a non-existing week number: 53.");
  testOneBadTimeRep(iso_week_format, IsoWeek(2009, 53, 1, 0,  0,  0.), "IsoWeek", "an existing week number: 53.", false);
  testOneBadTimeRep(iso_week_format, IsoWeek(2009, 54, 1, 0,  0,  0.), "IsoWeek", "a non-existing week number: 54.");

  testOneBadTimeString(iso_week_format, "2008-W00-1T00:00:00.0", "IsoWeek");
  testOneBadTimeString(iso_week_format, "2008-W53-1T00:00:00.0", "IsoWeek");
  testOneBadTimeString(iso_week_format, "2009-W53-1T00:00:00.0", "IsoWeek", false);
  testOneBadTimeString(iso_week_format, "2009-W54-1T00:00:00.0", "IsoWeek");

  // Test detections of non-existing day of week.
  testOneBadTimeRep(iso_week_format, IsoWeek(2008, 25, 0, 0,  0,  0.), "IsoWeek", "a non-existing day of the week: 0.");
  testOneBadTimeRep(iso_week_format, IsoWeek(2008, 25, 8, 0,  0,  0.), "IsoWeek", "a non-existing day of the week: 8.");

  testOneBadTimeString(iso_week_format, "2008-W25-0T00:00:00.0", "IsoWeek");
  testOneBadTimeString(iso_week_format, "2008-W25-8T00:00:00.0", "IsoWeek");

  // Test detections of non-existing time.
  testOneBadTimeRep(iso_week_format, IsoWeek(2008, 25, 2, -1,  0,  0.), "IsoWeek", "a non-existing hour: -1.");
  testOneBadTimeRep(iso_week_format, IsoWeek(2008, 25, 2, 24,  0,  0.), "IsoWeek", "a non-existing hour: 24.");
  testOneBadTimeRep(iso_week_format, IsoWeek(2008, 25, 2,  0, -1,  0.), "IsoWeek", "a non-existing minute: -1.");
  testOneBadTimeRep(iso_week_format, IsoWeek(2008, 25, 2,  0, 60,  0.), "IsoWeek", "a non-existing minute: 60.");
  testOneBadTimeRep(iso_week_format, IsoWeek(2008, 25, 2,  0,  0, -1.), "IsoWeek", "a non-existing second: -1.");

  testOneBadTimeString(iso_week_format, "2008-W25-2T-1:00:00.0", "IsoWeek");
  testOneBadTimeString(iso_week_format, "2008-W25-2T24:00:00.0", "IsoWeek");
  testOneBadTimeString(iso_week_format, "2008-W25-2T00:-1:00.0", "IsoWeek");
  testOneBadTimeString(iso_week_format, "2008-W25-2T00:60:00.0", "IsoWeek");
  testOneBadTimeString(iso_week_format, "2008-W25-2T00:00:-1.0", "IsoWeek");
  // Note: TimeFormat<IsoWeek> cannot detect the second part exceeding its maximum because of a possible leap second insertion,
  //       and TimeSystem class should test it instead.

  // Test detections of non-existing ordinal day.
  testOneBadTimeRep(ordinal_format, Ordinal(2008, 367, 0, 0, 0.), "Ordinal", "a non-existing ordinal date: 2008-367.");
  testOneBadTimeRep(ordinal_format, Ordinal(2008, 366, 0, 0, 0.), "Ordinal", "an existing ordinal date: 2008-366.", false);
  testOneBadTimeRep(ordinal_format, Ordinal(2009, 366, 0, 0, 0.), "Ordinal", "a non-existing ordinal date: 2009-366.");
  testOneBadTimeRep(ordinal_format, Ordinal(2100, 366, 0, 0, 0.), "Ordinal", "a non-existing ordinal date: 2100-366.");
  testOneBadTimeRep(ordinal_format, Ordinal(2000, 366, 0, 0, 0.), "Ordinal", "an existing ordinal date: 2000-366.", false);

  testOneBadTimeString(ordinal_format, "2008-367T00:00:00.0", "Ordinal");
  testOneBadTimeString(ordinal_format, "2008-366T00:00:00.0", "Ordinal", false);
  testOneBadTimeString(ordinal_format, "2009-366T00:00:00.0", "Ordinal");
  testOneBadTimeString(ordinal_format, "2100-366T00:00:00.0", "Ordinal");
  testOneBadTimeString(ordinal_format, "2000-366T00:00:00.0", "Ordinal", false);

  // Test detections of non-existing time.
  testOneBadTimeRep(ordinal_format, Ordinal(2008, 169, -1,  0,  0.), "Ordinal", "a non-existing hour: -1.");
  testOneBadTimeRep(ordinal_format, Ordinal(2008, 169, 24,  0,  0.), "Ordinal", "a non-existing hour: 24.");
  testOneBadTimeRep(ordinal_format, Ordinal(2008, 169,  0, -1,  0.), "Ordinal", "a non-existing minute: -1.");
  testOneBadTimeRep(ordinal_format, Ordinal(2008, 169,  0, 60,  0.), "Ordinal", "a non-existing minute: 60.");
  testOneBadTimeRep(ordinal_format, Ordinal(2008, 169,  0,  0, -1.), "Ordinal", "a non-existing second: -1.");

  testOneBadTimeString(ordinal_format, "2008-169T-1:00:00.0", "Ordinal");
  testOneBadTimeString(ordinal_format, "2008-169T24:00:00.0", "Ordinal");
  testOneBadTimeString(ordinal_format, "2008-169T00:-1:00.0", "Ordinal");
  testOneBadTimeString(ordinal_format, "2008-169T00:60:00.0", "Ordinal");
  testOneBadTimeString(ordinal_format, "2008-169T00:00:-1.0", "Ordinal");
  // Note: TimeFormat<Ordinal> cannot detect the second part exceeding its maximum because of a possible leap second insertion,
  //       and TimeSystem class should test it instead.

  // Test detections of bad Calendar, IsoWeek, and Ordinal strings.
  testOneBadTimeString(calendar_format, "Not A Number", "Calendar");
  testOneBadTimeString(iso_week_format, "Not A Number", "IsoWeek");
  testOneBadTimeString(ordinal_format, "Not A Number", "Ordinal");

  // Test detections of wrong kinds of calendar-like strings to parse.
  testOneBadTimeString(calendar_format, expected_iso_week_string, "Calendar");
  testOneBadTimeString(calendar_format, expected_ordinal_string, "Calendar");
  testOneBadTimeString(iso_week_format, expected_calendar_string, "IsoWeek");
  testOneBadTimeString(iso_week_format, expected_ordinal_string, "IsoWeek");
  testOneBadTimeString(ordinal_format, expected_calendar_string, "Ordinal");
  testOneBadTimeString(ordinal_format, expected_iso_week_string, "Ordinal");

  // Test date conversions in year 1995, where the calendar year is not divisible by 4, 100, nor 400.
  testOneCalendarDate(49776, 1995,  2, 28, 1995,  9, 2,  59);
  testOneCalendarDate(49777, 1995,  3,  1, 1995,  9, 3,  60);
  testOneCalendarDate(50082, 1995, 12, 31, 1995, 52, 7, 365);
  testOneCalendarDate(50083, 1996,  1,  1, 1996,  1, 1,   1);

  // Test date conversions in year 1996, where the calendar year is divisible by 4, but not by 100.
  testOneCalendarDate(50141, 1996,  2, 28, 1996,  9, 3,  59);
  testOneCalendarDate(50142, 1996,  2, 29, 1996,  9, 4,  60);
  testOneCalendarDate(50143, 1996,  3,  1, 1996,  9, 5,  61);
  testOneCalendarDate(50448, 1996, 12, 31, 1997,  1, 2, 366);
  testOneCalendarDate(50449, 1997,  1,  1, 1997,  1, 3,   1);

  // Test date conversions in year 2100, where the calendar year is divisible by 100, but not by 400.
  testOneCalendarDate(88127, 2100,  2, 28, 2100,  8, 7,  59);
  testOneCalendarDate(88128, 2100,  3,  1, 2100,  9, 1,  60);
  testOneCalendarDate(88433, 2100, 12, 31, 2100, 52, 5, 365);
  testOneCalendarDate(88434, 2101,  1,  1, 2100, 52, 6,   1);

  // Test date conversions in year 2000, where the calendar year is divisible by 400.
  testOneCalendarDate(51602, 2000,  2, 28, 2000,  9, 1,  59);
  testOneCalendarDate(51603, 2000,  2, 29, 2000,  9, 2,  60);
  testOneCalendarDate(51604, 2000,  3,  1, 2000,  9, 3,  61);
  testOneCalendarDate(51909, 2000, 12, 31, 2000, 52, 7, 366);
  testOneCalendarDate(51910, 2001,  1,  1, 2001,  1, 1,   1);

  // Test date conversions near the beginning of 2005, when an ISO year starts after a calendar year.
  testOneCalendarDate(53370, 2004, 12, 31, 2004, 53, 5, 366);
  testOneCalendarDate(53371, 2005,  1,  1, 2004, 53, 6,   1);
  testOneCalendarDate(53372, 2005,  1,  2, 2004, 53, 7,   2);
  testOneCalendarDate(53373, 2005,  1,  3, 2005,  1, 1,   3);

  // Test date conversions near the beginning of 2007, when an ISO year starts with a calendar year.
  testOneCalendarDate(54100, 2006, 12, 31, 2006, 52, 7, 365);
  testOneCalendarDate(54101, 2007,  1,  1, 2007,  1, 1,   1);
  testOneCalendarDate(54102, 2007,  1,  2, 2007,  1, 2,   2);

  // Test date conversions near the beginning of 2008, when an ISO year starts before a calendar year.
  testOneCalendarDate(54464, 2007, 12, 30, 2007, 52, 7, 364);
  testOneCalendarDate(54465, 2007, 12, 31, 2008,  1, 1, 365);
  testOneCalendarDate(54466, 2008,  1,  1, 2008,  1, 2,   1);

  // Test date conversions near the beginning of 2009, when the ISO year is three days into the previous Gregorian year.
  testOneCalendarDate(54828, 2008, 12, 28, 2008, 52, 7, 363);
  testOneCalendarDate(54829, 2008, 12, 29, 2009,  1, 1, 364);
  testOneCalendarDate(54830, 2008, 12, 30, 2009,  1, 2, 365);
  testOneCalendarDate(54831, 2008, 12, 31, 2009,  1, 3, 366);
  testOneCalendarDate(54832, 2009,  1,  1, 2009,  1, 4,   1);

  // Test date conversions near the beginning of 2010, when the ISO year is three days into the next Gregorian year.
  testOneCalendarDate(55196, 2009, 12, 31, 2009, 53, 4, 365);
  testOneCalendarDate(55197, 2010,  1,  1, 2009, 53, 5,   1);
  testOneCalendarDate(55198, 2010,  1,  2, 2009, 53, 6,   2);
  testOneCalendarDate(55199, 2010,  1,  3, 2009, 53, 7,   3);
  testOneCalendarDate(55200, 2010,  1,  4, 2010,  1, 1,   4);
}

void TimeSystemTestApp::testIntFracUtility() {
  setMethod("testIntFracUtility");

  // Get the utility.
  const IntFracUtility & utility(IntFracUtility::getUtility());

  // Test checking the integer and the fractional parts of a number.
  try {
    utility.check(100, .56789567895678956789);
  } catch (const std::exception &) {
    err() << "IntFracUtility::check(100, .56789567895678956789) threw an exception." << std::endl;
  }

  typedef std::pair<long, double> pair_type;
  std::list<pair_type> pair_list;
  pair_list.push_back(pair_type(+1, -.1));
  pair_list.push_back(pair_type(+1, +1.));
  pair_list.push_back(pair_type(-1, +.1));
  pair_list.push_back(pair_type(-1, -1.));
  pair_list.push_back(pair_type( 0, +1.));
  pair_list.push_back(pair_type( 0, -1.));
  for (std::list<pair_type>::const_iterator itor = pair_list.begin(); itor != pair_list.end(); ++itor) {
    try {
      utility.check(itor->first, itor->second);
      err() << "IntFracUtility::check(" << itor->first << ", " << itor->second << ") did not throw an exception." << std::endl;
    } catch (const std::exception &) {
      // That's good.
    }
  }

  // Prepare variables to use in the tests below.
  std::string sval("");
  long expected_int_part = 0;
  double expected_frac_part = 0.;
  long result_int_part = 0;
  double result_frac_part = 0.;
  double tolerance = 0.;
  int num_nine = 0;

  // Test parsing a character string.
  typedef std::map<std::string, std::pair<long, double> > map_type;
  map_type expected_pair;
  expected_pair["00050089.56789567895678956789"] = map_type::mapped_type(50089, .56789567895678956789);
  expected_pair["-00050089.56789567895678956789"] = map_type::mapped_type(-50089, -.56789567895678956789);
  expected_pair["  +1e+3  "] = map_type::mapped_type(1000, 0.);
  expected_pair["  -2e+3"] = map_type::mapped_type(-2000, 0.);
  expected_pair["3e-2  "] = map_type::mapped_type(0, 0.03);
  expected_pair["\t.004e+05\f"] = map_type::mapped_type(400, 0.);
  expected_pair["\v50000e-006\r\n"] = map_type::mapped_type(0, 0.05);
  expected_pair["6e+00"] = map_type::mapped_type(6, 0.);
  expected_pair["7e-000"] = map_type::mapped_type(7, 0.);
  expected_pair["+0e-1"] = map_type::mapped_type(0, 0.);
  expected_pair["-0.e+2"] = map_type::mapped_type(0, 0.);
  expected_pair["+.0e-03"] = map_type::mapped_type(0, 0.);
  expected_pair["0.e+004"] = map_type::mapped_type(0, 0.);
  expected_pair["-0.0e-0005"] = map_type::mapped_type(0, 0.);
  expected_pair["000e00000"] = map_type::mapped_type(0, 0.);
  expected_pair["+00000e-0000000"] = map_type::mapped_type(0, 0.);
  expected_pair["-00.000e+0000000"] = map_type::mapped_type(0, 0.);
  num_nine = std::numeric_limits<double>::digits10 + 5;
  expected_pair["5678." + std::string(num_nine, '9')] = map_type::mapped_type(5679, 0.);
  expected_pair["-5678." + std::string(num_nine, '9')] = map_type::mapped_type(-5679, 0.);
  num_nine = std::numeric_limits<double>::digits10 - 3;
  expected_pair["9999999." + std::string(num_nine, '9')] = map_type::mapped_type(9999999, 1. - std::pow(0.1, num_nine));
  expected_pair["-9999999." + std::string(num_nine, '9')] = map_type::mapped_type(-9999999, -1. + std::pow(0.1, num_nine));

  tolerance = std::numeric_limits<double>::epsilon() * 10.;
  for (map_type::const_iterator itor = expected_pair.begin(); itor != expected_pair.end(); ++itor) {
    sval = itor->first;
    expected_int_part = (itor->second).first;
    expected_frac_part = (itor->second).second;
    result_int_part = 0;
    result_frac_part = 0.;
    utility.parse(sval, result_int_part, result_frac_part);
    if (result_int_part != expected_int_part || tolerance < std::fabs(result_frac_part - expected_frac_part)) {
      err() << "IntFracUtility::parse(\"" << sval << "\", int_part, frac_part) returned (int_part, frac_part) = (" <<
        result_int_part << ", " << result_frac_part << "), not (" << expected_int_part << ", " << expected_frac_part <<
        ") as expected." << std::endl;
    }
  }

  // Test errors resulting from bad string conversions.
  std::list<std::string> sval_list;
  sval_list.push_back("! 1.e6");
  sval_list.push_back("1.e6 0");
  sval_list.push_back("1..e6");
  sval_list.push_back("e6");
  sval_list.push_back("+-1e6");
  sval_list.push_back("-+1e6");
  sval_list.push_back("1 + 6");
  sval_list.push_back("0. 0.");
  sval_list.push_back(" ");
  sval_list.push_back("");
  sval_list.push_back("12,345");
  sval_list.push_back("0x1234");
  num_nine = std::numeric_limits<double>::digits10 + 5;
  {
    std::ostringstream os;
    os << std::numeric_limits<long>::max() << ".";
    sval = os.str();
  }
  sval += std::string(num_nine, '9');
  sval_list.push_back(sval);
  {
    std::ostringstream os;
    os << std::numeric_limits<long>::min() << ".";
    sval = os.str();
  }
  sval += std::string(num_nine, '9');
  sval_list.push_back(sval);
  for (std::list<std::string>::const_iterator itor = sval_list.begin(); itor != sval_list.end(); ++itor) {
    sval = *itor;
    result_int_part = 0;
    result_frac_part = 0.;
    try {
      utility.parse(sval, result_int_part, result_frac_part);
      err() << "IntFracUtility::parse(\"" << sval << "\", int_part, frac_part) did not throw an exception." << std::endl;
    } catch (const std::exception &) {
      // That's good.
    }
  }

  // Test of parsing near integer overflow/underflow.
  for (long distance = 0; distance < 10; ++distance) {
    tolerance = std::numeric_limits<double>::epsilon() * 10.;
    bool exception_thrown = false;

    expected_int_part = std::numeric_limits<long>::max() - distance;
    expected_frac_part = .56789567895678956789;
    std::ostringstream os1;
    os1 << expected_int_part << ".56789567895678956789";
    sval = os1.str();
    result_int_part = 0;
    result_frac_part = 0.;
    exception_thrown = false;
    try {
      utility.parse(sval, result_int_part, result_frac_part);
    } catch (const std::exception &) {
      err() << "IntFracUtility::parse(\"" << sval << "\", int_part, frac_part) threw an exception." << std::endl;
      exception_thrown = true;
    }
    if (!exception_thrown) {
      if (result_int_part != expected_int_part || tolerance < std::fabs(result_frac_part - expected_frac_part)) {
        err() << "IntFracUtility::parse(\"" << sval << "\", int_part, frac_part) returned (int_part, frac_part) = (" <<
          result_int_part << ", " << result_frac_part << "), not (" << expected_int_part << ", " << expected_frac_part <<
          ") as expected." << std::endl;
      }
    }

    expected_int_part = std::numeric_limits<long>::min() + distance;
    expected_frac_part = -.56789567895678956789;
    std::ostringstream os2;
    os2 << expected_int_part << ".56789567895678956789";
    sval = os2.str();
    result_int_part = 0;
    result_frac_part = 0.;
    exception_thrown = false;
    try {
      utility.parse(sval, result_int_part, result_frac_part);
    } catch (const std::exception &) {
      err() << "IntFracUtility::parse(\"" << sval << "\", int_part, frac_part) threw an exception." << std::endl;
      exception_thrown = true;
    }
    if (!exception_thrown) {
      if (result_int_part != expected_int_part || tolerance < std::fabs(result_frac_part - expected_frac_part)) {
        err() << "IntFracUtility::parse(\"" << sval << "\", int_part, frac_part) returned (int_part, frac_part) = (" <<
          result_int_part << ", " << result_frac_part << "), not (" << expected_int_part << ", " << expected_frac_part <<
          ") as expected." << std::endl;
      }
    }
  }

  // Test for detections of overflow/underflow in parsing.
  long large_div_10 = std::numeric_limits<long>::max() / 10;
  long large_mod_10 = std::numeric_limits<long>::max() - 10*large_div_10 + 1;
  if (large_mod_10 >= 10) {
    large_div_10 += 1;
    large_mod_10 -= 10;
  }
  {
    std::ostringstream os;
    os << large_div_10 << large_mod_10 << ".56789567895678956789";
    sval = os.str();
  }
  try {
    utility.parse(sval, result_int_part, result_frac_part);
    err() << "IntFracUtility::parse(\"" << sval << "\", int_part, frac_part) did not throw an exception." << std::endl;
  } catch (const std::exception &) {
    // That's good.
  }

  long small_div_10 = std::numeric_limits<long>::min() / 10;
  long small_mod_10 = -(std::numeric_limits<long>::min() - 10*small_div_10) + 1;
  if (small_mod_10 >= 10) {
    small_div_10 -= 1;
    small_mod_10 -= 10;
  }
  {
    std::ostringstream os;
    os << small_div_10 << small_mod_10 << ".56789567895678956789";
    sval = os.str();
  }
  try {
    utility.parse(sval, result_int_part, result_frac_part);
    err() << "IntFracUtility::parse(\"" << sval << "\", int_part, frac_part) did not throw an exception." << std::endl;
  } catch (const std::exception &) {
    // That's good.
  }

  // Prepare variables to use in the tests below.
  std::string expected_string("");
  std::string result_string("");
  std::string::size_type compare_size = 1 + std::numeric_limits<long>::digits10 + 1 + std::numeric_limits<double>::digits10;
  // Note: 1's are for a sign and a decimal point.

  // Test formatting a pair of the integer and the fractional parts of a number.
  expected_string = "1234.56789567895678956789;";
  result_string = utility.format(1234, .56789567895678956789);
  compare_size = 0 + 4 + 1 + std::numeric_limits<double>::digits10 - 2;
  // Note: 0 for a sign, 4 for the number of digits in the integer part, 1 for a decimal point, and -2 for tolerance.
  if (result_string.compare(0, compare_size, expected_string, 0, compare_size)) {
    err() << "IntFracUtility::format(1234, .56789567895678956789) returned \"" << result_string << "\", not \"" << expected_string <<
      "\" as expected." << std::endl;
  }

  expected_string = "-1234.56789567895678956789;";
  result_string = utility.format(-1234, -.56789567895678956789);
  compare_size = 1 + 4 + 1 + std::numeric_limits<double>::digits10 - 2;
  // Note: 1 for a sign, 4 for the number of digits in the integer part, 1 for a decimal point, and -2 for tolerance.
  if (result_string.compare(0, compare_size, expected_string, 0, compare_size)) {
    err() << "IntFracUtility::format(-1234, -.56789567895678956789) returned \"" << result_string << "\", not \"" << expected_string <<
      "\" as expected." << std::endl;
  }

  // Prepare variables to use in the tests below.
  double dval = 0.;

  // Test splitting a double into the integer and the fractional parts.
  dval = 56789.56789567895678956789;
  expected_int_part = 56789;
  expected_frac_part = .56789567895678956789;
  result_int_part = 0;
  result_frac_part = 0.;
  utility.split(dval, result_int_part, result_frac_part);
  tolerance = std::numeric_limits<double>::epsilon() * 1.e+6; // Five digits are taken by the integer part.
  if (result_int_part != expected_int_part || tolerance < std::fabs(result_frac_part - expected_frac_part)) {
    err() << "IntFracUtility::split(\"" << dval << "\", int_part, frac_part) returned (int_part, frac_part) = (" <<
      result_int_part << ", " << result_frac_part << "), not (" << expected_int_part << ", " << expected_frac_part <<
      ") as expected." << std::endl;
  }

  // Test for detections of overflow/underflow in splitting.
  double large_number = std::numeric_limits<long>::max() + .5;
  double too_large = std::numeric_limits<long>::max() + 1.5;
  double small_number = std::numeric_limits<long>::min() - .5;
  double too_small = std::numeric_limits<long>::min() - 1.5;
  int diff_digits = std::numeric_limits<long>::digits - std::numeric_limits<double>::digits;
  if (diff_digits > 0) {
    // Create test numbers by bit-wise operation.
    // Note: This approach assumes that a number is represented by a sign, a significand, a base, and an exponent as
    //       number = sign x significand x base ^ exponent, and a bse of 2 is used for arithmatic.  As a result, the
    //       followign tests may fail because the assumption is not correct, not because of a bug in IntFracUtility class.
    long minimum_increment = (1 << diff_digits);
    long bit_mask = ~(minimum_increment - 1);
    large_number = std::numeric_limits<long>::max() & bit_mask;
    too_large = large_number + minimum_increment * 1.1;
    small_number = std::numeric_limits<long>::min() & bit_mask;
    too_small = small_number - minimum_increment * 1.1;
  }
  try {
    utility.split(large_number, result_int_part, result_frac_part);
  } catch (const std::exception &) {
    err() << "IntFracUtility::split(\"" << large_number << "\", int_part, frac_part) threw an exception." << std::endl;
  }
  try {
    utility.split(too_large, result_int_part, result_frac_part);
    err() << "IntFracUtility::split(\"" << too_large << "\", int_part, frac_part) did not throw an exception." << std::endl;
  } catch (const std::exception &) {
    // That's good.
  }
  try {
    utility.split(small_number, result_int_part, result_frac_part);
  } catch (const std::exception &) {
    err() << "IntFracUtility::split(\"" << small_number << "\", int_part, frac_part) threw an exception." << std::endl;
  }
  try {
    utility.split(too_small, result_int_part, result_frac_part);
    err() << "IntFracUtility::split(\"" << too_small << "\", int_part, frac_part) did not throw an exception." << std::endl;
  } catch (const std::exception &) {
    // That's good.
  }
}

void TimeSystemTestApp::testSourcePosition() {
  setMethod("testSourcePosition");

  // Prepare test parameters and variables.
  std::auto_ptr<SourcePosition> src_ptr(0);
  double base_ra = 30.;
  double base_dec = 60.;
  double base_x = 0.5 * std::sqrt(3.)/2.;
  double base_y = 0.5 * 1./2.;
  double base_z = std::sqrt(3.)/2.;
  double test_distance = 3.26 * 365.25 * 86400.; // 1 light-year in light-seconds.
  double tolerance = std::numeric_limits<double>::epsilon() * 1000.;
  std::string coord_name("XYZ");
  double par_table[][5] = {
    {       base_ra,  base_dec,  base_x,  base_y,  base_z},
    {180. - base_ra,  base_dec, -base_x,  base_y,  base_z},
    {180. + base_ra,  base_dec, -base_x, -base_y,  base_z},
    {360. - base_ra,  base_dec,  base_x, -base_y,  base_z},
    {       base_ra, -base_dec,  base_x,  base_y, -base_z},
    {180. - base_ra, -base_dec, -base_x,  base_y, -base_z},
    {180. + base_ra, -base_dec, -base_x, -base_y, -base_z},
    {360. - base_ra, -base_dec,  base_x, -base_y, -base_z}
  };

  // Test the constructor that takes RA and Dec.
  for (size_t ii = 0; ii < sizeof(par_table)/sizeof(double)/5; ++ii) {
    double * par_ptr = par_table[ii];
    double test_ra = par_ptr[0];
    double test_dec = par_ptr[1];
    std::vector<double> expected_dircos(par_ptr+2, par_ptr+5);

    // Test four different constructors.
    for (size_t constructor_type = 0; constructor_type < 4; ++constructor_type) {
      if (0 == constructor_type) {
        src_ptr.reset(new SourcePosition(test_ra, test_dec));
      } else if (1 == constructor_type) {
        src_ptr.reset(new SourcePosition(test_ra, test_dec, test_distance));
      } else if (2 == constructor_type) {
        src_ptr.reset(new SourcePosition(expected_dircos));
      } else if (3 == constructor_type) {
        src_ptr.reset(new SourcePosition(expected_dircos, test_distance));
      } else {
        break;
      }

      // Create an indicator of the test type.
      std::ostringstream os;
      os << " (parameter index: " << ii << ", constructor type: " << constructor_type << ")";
      std::string test_id(os.str());

      // Test conversions/copies of direction cosines.
      const std::vector<double> & result_dircos = src_ptr->getDirection();
      if (result_dircos.size() != 3) {
        err() << "SourcePosition.getDirection returns a vector of size " << result_dircos.size() << ", not 3." <<
          test_id << std::endl;
      } else {
        for (size_t jj = 0; jj < 3; ++jj) {
          if (std::fabs(result_dircos[jj] - expected_dircos[jj]) > tolerance) {
            err() << "SourcePosition.getDirection returns " << result_dircos[jj] << " for its " << coord_name[jj] <<
              " coordinate, not " << expected_dircos[jj] << "." << test_id << std::endl;
          }
        }
      }

      // Test access to the distance to the source.
      if (constructor_type % 2) {
        // Test distance checker.
        if (!src_ptr->hasDistance()) {
          err() << "SourcePosition.hasDistance returns " << src_ptr->hasDistance() << ", not " << true << "." <<
            test_id << std::endl;
        } else {
          // Test distance getter.
          double result_distance = src_ptr->getDistance();
          if (std::fabs(result_distance - test_distance) > tolerance) {
            err() << "SourcePosition.getDistance returns " << result_distance << ", not " << test_distance << "." <<
              test_id << std::endl;
          }
        }

      } else {
        // Test distance checker.
        if (src_ptr->hasDistance()) {
          err() << "SourcePosition.hasDistance returns " << src_ptr->hasDistance() << ", not " << false << "." <<
            test_id << std::endl;
        }
      }
    }
  }

  // Test detection of a zero vector.
  std::vector<double> zero_vector(3, 0.);
  double dummy_distance = 123.4567;
  try {
    src_ptr.reset(new SourcePosition(zero_vector));
    err() << "Constructor of SourcePosition (w/o distance) did not throw an exception for a zero vector." << std::endl;
  } catch (const std::exception &) {
  }
  try {
    src_ptr.reset(new SourcePosition(zero_vector, dummy_distance));
    err() << "Constructor of SourcePosition (w/ distance) did not throw an exception for a zero vector." << std::endl;
  } catch (const std::exception &) {
  }

  // Test detection of a vector with no element.
  std::vector<double> zero_dim_vector(0, 0.);
  try {
    src_ptr.reset(new SourcePosition(zero_dim_vector));
    err() << "Constructor of SourcePosition (w/o distance) did not throw an exception for a zero dimentional vector." << std::endl;
  } catch (const std::exception &) {
  }
  try {
    src_ptr.reset(new SourcePosition(zero_dim_vector, dummy_distance));
    err() << "Constructor of SourcePosition (w/ distance) did not throw an exception for a zero dimentional vector." << std::endl;
  } catch (const std::exception &) {
  }

  // Test proper handling of wrong-sized "three"-vector input.
  double vector_table[][3] = {
    {std::sqrt(1./3.), std::sqrt(1./3.), std::sqrt(1./3.)},
    {std::sqrt(1./3.), std::sqrt(1./3.), std::sqrt(1./3.)},
    {std::sqrt(1./3.), std::sqrt(1./3.), std::sqrt(1./3.)},
    {std::sqrt(1./3.), std::sqrt(1./3.), std::sqrt(1./3.)},
    {std::sqrt(1./3.), std::sqrt(1./3.), std::sqrt(1./3.)},
    {std::sqrt(1./2.), std::sqrt(1./2.), 0.},
    {1., 0., 0.}
  };
  size_t num_itor = sizeof(vector_table)/sizeof(double)/3;
  std::vector<double> test_vector(num_itor, 1.);
  for (size_t ii = 0; ii < num_itor; ++ii) {
    double * vector_ptr = vector_table[ii];
    std::vector<double> expected_dircos(vector_ptr, vector_ptr+3);

    // Test two constructors that take a three vector.
    for (size_t constructor_type = 0; constructor_type < 2; ++constructor_type) {
      if (0 == constructor_type) {
        src_ptr.reset(new SourcePosition(test_vector));
      } else if (1 == constructor_type) {
        src_ptr.reset(new SourcePosition(test_vector, dummy_distance));
      } else {
        break;
      }

      // Create an indicator of the test type.
      std::ostringstream os;
      os << " (parameter index: " << ii << ", constructor type: " << constructor_type << ")";
      std::string test_id(os.str());


      // Test conversions/copies of direction cosines.
      const std::vector<double> & result_dircos = src_ptr->getDirection();
      if (result_dircos.size() != 3) {
        err() << "SourcePosition.getDirection returns a vector of size " << result_dircos.size() << ", not 3." <<
          test_id << std::endl;
      } else {
        for (size_t jj = 0; jj < 3; ++jj) {
          if (std::fabs(result_dircos[jj] - expected_dircos[jj]) > tolerance) {
            err() << "SourcePosition.getDirection returns " << result_dircos[jj] << " for its " << coord_name[jj] <<
              " coordinate, not " << expected_dircos[jj] << "." << test_id << std::endl;
          }
        }
      }
    }

    // Remove one element from the test input for the next iteration.
    test_vector.pop_back();
  }
}

void TimeSystemTestApp::testBaryTimeComputer() {
  setMethod("testBaryTimeComputer");

  // Prepare a time to be geo/barycentered and an expected result after geo/barycentered.
  AbsoluteTime glast_tt_origin("TT", 51910, 64.184);
  double glast_time_original = 2.123393677090199E+08; // TSTART in my_pulsar_events_v3.fits.
  AbsoluteTime original = glast_tt_origin + ElapsedTime("TT", Duration(glast_time_original, "Sec"));
  AbsoluteTime glast_tdb_origin("TDB", 51910, 64.184);
  double glast_time_bary = 2.123393824137859E+08; // TSTART in my_pulsar_events_bary_v3.fits.
  AbsoluteTime expected_bary = glast_tdb_origin + ElapsedTime("TDB", Duration(glast_time_bary, "Sec"));
  double glast_time_geo = 212339367.70603555441; // Once computed by BaryTimeComputer::computeGeoTime method.
  AbsoluteTime expected_geo = glast_tt_origin + ElapsedTime("TT", Duration(glast_time_geo, "Sec"));

  // Set position/velocity vectors of the Earth, the Sun, and the solar system barycenter.
  // Note: Those values are once computed by C-function "dpleph" in dpleph.c with JPL DE405.
  //double rce[] = {500.8780937237265789, 10.97861817447444821, 4.7143308336665485925}; // SSBC-to-Earth vector (nnecessary here).
  double rca[] = {500.88913851975058833, 10.996303638826491422, 4.724526724679001255}; // SSBC-to-S/C vector.
  double vce[] = {-3.5658338025897011906e-06, 9.078003543825905223e-05, 3.9352343261386849526e-05}; // Earth velocity w.r.t SSBC.
  //double rcs[] = {0.38800865319152139099, 2.2364243580518863297, 0.9254092484672552521}; // SSBC-to-Sun vector (nnecessary here).
  double rsa[] = {500.50112986655904024, 8.7598792807746050926, 3.7991174762117458918}; // Sun-to-S/C vector.

  // Set parameters for barycentering.
  double ra = 85.0482;
  double dec = -69.3319;
  double glast_pos_array[] = {3311146.54815027, 5301968.82897028, 3056651.22812332}; // SC position at TSTART (computed separately).
  std::vector<double> glast_pos(glast_pos_array, glast_pos_array + 3);
  SourcePosition src_pos(ra, dec);
  const std::vector<double> & src_dir = src_pos.getDirection();

  // Check pre-computed results of geo/barycentric corrections.
  ElapsedTime tolerance("TDB", Duration(1.e-7, "Sec"));
  const double speed_of_light = 2.99792458e+8;
  const double solar_mass = 4.925490948308836987e-06; // Once computed by C-function "dpleph" in dpleph.c with JPL DE405.
  double bary_delay = src_dir[0]*rca[0] + src_dir[1]*rca[1] + src_dir[2]*rca[2]
    + (glast_pos[0]*vce[0] + glast_pos[1]*vce[1] + glast_pos[2]*vce[2]) / speed_of_light
    + 2. * solar_mass * std::log(1. + (src_dir[0]*rsa[0] + src_dir[1]*rsa[1] + src_dir[2]*rsa[2])
      / std::sqrt(rsa[0]*rsa[0] + rsa[1]*rsa[1] + rsa[2]*rsa[2]));
  AbsoluteTime result = original + ElapsedTime("TDB", Duration(bary_delay, "Sec"));
  if (!result.equivalentTo(expected_bary, tolerance)) {
    err() << "Explicit computation of barycentric time results in AbsoluteTime(" << result <<
      "), not equivalent to the pre-computed barycentric time AbsoluteTime(" << expected_bary <<
      ") with tolerance of " << tolerance << "." << std::endl;
  }

  double geo_delay = (src_dir[0] * glast_pos[0] + src_dir[1] * glast_pos[1] + src_dir[2] * glast_pos[2]) / speed_of_light;
  result = original + ElapsedTime("TT", Duration(geo_delay, "Sec"));
  if (!result.equivalentTo(expected_geo, tolerance)) {
    err() << "Explicit computation of geocentric time results in AbsoluteTime(" << result <<
      "), not equivalent to the pre-computed geocentric time AbsoluteTime(" << expected_geo <<
      ") with tolerance of " << tolerance << "." << std::endl;
  }

  // Test error detection in getting a BaryTimeComputer object for non-existing ephemeris.
  try {
    BaryTimeComputer::getComputer("No Such Ephemeris");
    err() << "BaryTimeComputer::getComputer(\"No Such Ephemeris\") did not throw an exception when it should." << std::endl;
  } catch (const std::exception &) {
  }

  // Get a barycentric time computer for JPL DE405 ephemeris.
  const BaryTimeComputer & computer405 = BaryTimeComputer::getComputer("JPL DE405");

  // Check ephemeris name.
  std::string ephem_name = computer405.getPlanetaryEphemerisName();
  if ("JPL DE405" != ephem_name) {
    err() << "BaryTimeComputer::getPlanetaryEphemerisName() returned \"" << ephem_name << "\", not \"JPL DE405\"." << std::endl;
  }

  // Test barycentric correction (for a source at an infinate distance).
  result = original;
  computer405.computeBaryTime(src_pos, glast_pos, result);
  if (!result.equivalentTo(expected_bary, tolerance)) {
    err() << "BaryTimeComputer::computeBaryTime(SourcePosition(" << ra << ", " << dec << "), " << original << ")" <<
      " returned AbsoluteTime(" << result << "), not equivalent to AbsoluteTime(" << expected_bary <<
      ") with tolerance of " << tolerance << "." << std::endl;
  }

  // Test geocentric correction (for a source at an infinate distance).
  result = original;
  computer405.computeGeoTime(src_pos, glast_pos, result);
  tolerance = ElapsedTime("TT", Duration(1.e-7, "Sec"));
  if (!result.equivalentTo(expected_geo, tolerance)) {
    err() << "BaryTimeComputer::computeGeoTime(SourcePosition(" << ra << ", " << dec << "), " << original << ")" <<
      " returned AbsoluteTime(" << result << "), not equivalent to AbsoluteTime(" << expected_geo <<
      ") with tolerance of " << tolerance << "." << std::endl;
  }

  // Compute a source position at a finite distance, whose apparent viewing direction at the spacecraft
  // happens to be identical to the one used in the previous test (ra = 85.0482, dec = -69.3319).
  double distance_from_sc = 5000.; // Approximately 10 AU, in order for parallax to stand out.
  std::vector<double> ssb_to_src(3);
  double distance_from_ssb = 0.;
  for (std::size_t ii = 0; ii < 3; ++ii) {
    ssb_to_src[ii] = rca[ii] + src_dir[ii] * distance_from_sc;
    distance_from_ssb += ssb_to_src[ii]*ssb_to_src[ii];
  }
  distance_from_ssb = std::sqrt(distance_from_ssb);
  SourcePosition nearby_src(ssb_to_src, distance_from_ssb);

  // Test barycentric correction for a source at a finite distance.
  result = original;
  computer405.computeBaryTime(nearby_src, glast_pos, result);
  if (!result.equivalentTo(expected_bary, tolerance)) {
    err() << "BaryTimeComputer::computeBaryTime(nearby_src, " << original << ")" <<
      " returned AbsoluteTime(" << result << "), not equivalent to AbsoluteTime(" << expected_bary <<
      ") with tolerance of " << tolerance << "." << std::endl;
  }

  // Test geocentric correction for a source at a finate distance.
  result = original;
  computer405.computeGeoTime(nearby_src, glast_pos, result);
  tolerance = ElapsedTime("TT", Duration(1.e-7, "Sec"));
  if (!result.equivalentTo(expected_geo, tolerance)) {
    err() << "BaryTimeComputer::computeGeoTime(nearby_src, " << original << ")" <<
      " returned AbsoluteTime(" << result << "), not equivalent to AbsoluteTime(" << expected_geo <<
      ") with tolerance of " << tolerance << "." << std::endl;
  }

  // Test error detection in getting a BaryTimeComputer object for a different, supported JPL ephemeris.
  try {
    BaryTimeComputer::getComputer("JPL DE200");
    err() << "BaryTimeComputer::getComputer(\"JPL DE200\") did not throw an exception when it should." << std::endl;
  } catch (const std::exception &) {
  }

  // Test error non-detection in getting a BaryTimeComputer object for JPL DE405 again.
  try {
    BaryTimeComputer::getComputer("JPL DE405");
  } catch (const std::exception &) {
    err() << "BaryTimeComputer::getComputer(\"JPL DE405\") threw an exception when it should not." << std::endl;
  }
}

class BogusTimeHandlerBase: public EventTimeHandler {
  public:
    virtual ~BogusTimeHandlerBase() {}

    static EventTimeHandler * createInstance(const std::string & /*file_name*/, const std::string & /*extension_name*/,
      bool /*read_only*/ = true)
      { return 0; }

    virtual void initTimeCorrection(const std::string & /*sc_file_name*/, const std::string & /*sc_extension_name*/,
      const std::string & /*solar_eph*/, bool /*match_solar_eph*/, double /*angular_tolerance*/) {}

    virtual void setSourcePosition(double /*ra*/, double /*dec*/) {}
    virtual void setSourcePosition(const SourcePosition & /*src_position*/) {}

    virtual AbsoluteTime readTime(const std::string & /*field_name*/, bool /*from_header*/ = false) const
      { return AbsoluteTime("TDB", 51910, 0.); }

    virtual void writeTime(const std::string & /*field_name*/, const AbsoluteTime & /*abs_time*/, bool /*to_header*/ = false) {}

    virtual AbsoluteTime getGeoTime(const std::string & /*field_name*/, bool /*from_header*/ = false) const
      { return AbsoluteTime("TDB", 51910, 0.); }

    virtual AbsoluteTime getBaryTime(const std::string & /*field_name*/, bool /*from_header*/ = false) const
      { return AbsoluteTime("TDB", 51910, 0.); }

    virtual AbsoluteTime parseTimeString(const std::string & /*time_string*/, const std::string & /*time_system*/ = "FILE") const
      { return AbsoluteTime("TDB", 51910, 0.); }

  protected:
    BogusTimeHandlerBase(const std::string & file_name, const std::string & extension_name, bool read_only = true):
    EventTimeHandler(file_name, extension_name, read_only) {}
};

class BogusTimeHandler1: public BogusTimeHandlerBase {
  public:
    static EventTimeHandler * createInstance(const std::string & /*file_name*/, const std::string & /*extension_name*/,
      bool /*read_only*/ = true)
      { return 0; }

  private:
    BogusTimeHandler1(const std::string & file_name, const std::string & extension_name):
    BogusTimeHandlerBase(file_name, extension_name) {}
};

class BogusTimeHandler2: public BogusTimeHandlerBase {
  public:
    static EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name,
      bool read_only = true)
      { return new BogusTimeHandler2(file_name, extension_name, read_only); }

  private:
    BogusTimeHandler2(const std::string & file_name, const std::string & extension_name, bool read_only = true):
    BogusTimeHandlerBase(file_name, extension_name, read_only) {}
};

class BogusTimeHandler3: public BogusTimeHandlerBase {
  public:
    static EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name,
      bool read_only = true)
      { return new BogusTimeHandler3(file_name, extension_name, read_only); }

  private:
    BogusTimeHandler3(const std::string & file_name, const std::string & extension_name, bool read_only = true):
    BogusTimeHandlerBase(file_name, extension_name, read_only) {}
};

void TimeSystemTestApp::testEventTimeHandlerFactory() {
  setMethod("testEventTimeHandlerFactory");

  // Prepare test parameters in this method.
  std::string event_file = prependDataPath("my_pulsar_events_v3.fits");

  // Test creation of BogusTimeHandler1 (an EventTimeHandler sub-class) through its createInstance method.
  std::auto_ptr<EventTimeHandler> handler(0);
  handler.reset(BogusTimeHandler1::createInstance(event_file, "EVENTS"));
  if (handler.get() != 0) {
    err() << "BogusTimeHandler1::createInstance method did not return a null pointer (0)." << std::endl;
  }

  // Test creation of BogusTimeHandler2 (an EventTimeHandler sub-class) through its createInstance method.
  handler.reset(BogusTimeHandler2::createInstance(event_file, "EVENTS"));
  if (0 == handler.get()) {
    err() << "BogusTimeHandler2::createInstance method returned a null pointer (0)." << std::endl;
  } else if (0 == dynamic_cast<BogusTimeHandler2 *>(handler.get())) {
    err() << "BogusTimeHandler2::createInstance method did not return a BogusTimeHandler2 object." << std::endl;
  }

  // Test creation of BogusTimeHandler3 (an EventTimeHandler sub-class) through its createInstance method.
  handler.reset(BogusTimeHandler3::createInstance(event_file, "EVENTS"));
  if (0 == handler.get()) {
    err() << "BogusTimeHandler3::createInstance method returned a null pointer (0)." << std::endl;
  } else if (0 == dynamic_cast<BogusTimeHandler3 *>(handler.get())) {
    err() << "BogusTimeHandler3::createInstance method did not return a BogusTimeHandler3 object." << std::endl;
  }

  // Test the decision-making mechanism for cases without a prior setup.
  try {
    handler.reset(IEventTimeHandlerFactory::createHandler(event_file, "EVENTS"));
    err() << "IEventTimeHandlerFactory::createHandler method did not throw an exception when no handler was registered." << std::endl;
  } catch (const std::exception &) {
  }

  // Register BogusTimeHandler1 to EventTimeHandlerFactory.
  EventTimeHandlerFactory<BogusTimeHandler1> factory1;

  // Test the decision-making mechanism for cases with only BogusTimeHandler1 registered.
  try {
    handler.reset(IEventTimeHandlerFactory::createHandler(event_file, "EVENTS"));
    err() << "IEventTimeHandlerFactory::createHandler method did not throw an exception when only BogusTimeHandler1 was registered."
      << std::endl;
  } catch (const std::exception &) {
  }

  // Register BogusTimeHandler3 to EventTimeHandlerFactory.
  EventTimeHandlerFactory<BogusTimeHandler3> factory2;

  // Test the decision-making mechanism for cases with BogusTimeHandler1 and BogusTimeHandler3 registered.
  handler.reset(IEventTimeHandlerFactory::createHandler(event_file, "EVENTS"));
  if (0 == handler.get()) {
    err() << "IEventTimeHandlerFactory::createHandler method returned a null pointer (0)." << std::endl;
  } else if (0 == dynamic_cast<BogusTimeHandler3 *>(handler.get())) {
    err() << "IEventTimeHandlerFactory::createHandler method did not return a BogusTimeHandler3 object" <<
      " when it is the only appropriate handler." << std::endl;
  }

  // Register BogusTimeHandler2 to EventTimeHandlerFactory.
  EventTimeHandlerFactory<BogusTimeHandler2> factory3;

  // Test the decision-making mechanism for cases with BogusTimeHandler1, BogusTimeHandler3, and BogusTimeHandler2 registered.
  handler.reset(IEventTimeHandlerFactory::createHandler(event_file, "EVENTS"));
  if (0 == handler.get()) {
    err() << "IEventTimeHandlerFactory::createHandler method returned a null pointer (0)." << std::endl;
  } else if (0 == dynamic_cast<BogusTimeHandler3 *>(handler.get())) {
    err() << "IEventTimeHandlerFactory::createHandler method did not return a BogusTimeHandler3 object" <<
      " when it is an appropriate handler that can respond first." << std::endl;
  }

  // De-register and re-register three handlers in a different order.
  factory1.deregisterHandler();
  factory2.deregisterHandler();
  factory3.deregisterHandler();
  factory3.registerHandler();
  factory2.registerHandler();
  factory1.registerHandler();

  // Test the decision-making mechanism for cases with BogusTimeHandler2, BogusTimeHandler3, and BogusTimeHandler1 registered.
  handler.reset(IEventTimeHandlerFactory::createHandler(event_file, "EVENTS"));
  if (0 == handler.get()) {
    err() << "IEventTimeHandlerFactory::createHandler method returned a null pointer (0)." << std::endl;
  } else if (0 == dynamic_cast<BogusTimeHandler2 *>(handler.get())) {
    err() << "IEventTimeHandlerFactory::createHandler method did not return a BogusTimeHandler2 object" <<
      " when it is an appropriate handler that can respond first." << std::endl;
  }
}

void TimeSystemTestApp::testglastscorbit() {
  setMethod("testglastscorbit");

  // Set spacecraft file name.
  std::string sc_file = prependDataPath("testscfile_std.fits");
  char * sc_file_char = const_cast<char *>(sc_file.c_str());

  // Test initialization function.
  GlastScFile * scptr = glastscorbit_open(sc_file_char, "SC_DATA");
  if (0 == scptr) {
    err() << "Function glastscorbit_open returns a null pointer for spacecraft file \"" << sc_file <<
      "\" and extension name \"SC_DATA\"." << std::endl;
  }

  int status = glastscorbit_getstatus(scptr);
  if (status) {
    err() << "Function glastscorbit_open returns with non-zero status (" << status << ") for spacecraft file \"" <<
      sc_file << "\" and extension name \"SC_DATA\"." << std::endl;
  }

  // Prepare parameters for testing interpolation of spacecraft position.
  double orbit_radius = 7000000.;
  double scpos0x = orbit_radius;
  double scpos1x = orbit_radius * std::sqrt(3.) / 2.;
  double scpos1y = orbit_radius / 2. * std::sqrt(3.) / 2.;
  double scpos1z = orbit_radius / 2. / 2.;
  double scpos2x = orbit_radius / 2.;
  double scpos2y = orbit_radius * std::sqrt(3.) / 2. * std::sqrt(3.) / 2.;
  double scpos2z = orbit_radius * std::sqrt(3.) / 2. / 2.;
  double scpos3y = orbit_radius * std::sqrt(3.) / 2.;
  double scpos3z = orbit_radius / 2.;
  double par_list[][4] = {
    // F a circular orbit with the radius of 7000 km.
    {1000., +scpos0x,       0.,       0.},
    {1010., +scpos1x, +scpos1y, +scpos1z},
    {1020., +scpos2x, +scpos2y, +scpos2z},
    {1030.,       0., +scpos3y, +scpos3z},
    {1040., -scpos2x, +scpos2y, +scpos2z},
    {1050., -scpos1x, +scpos1y, +scpos1z},
    {1060., -scpos0x,       0.,       0.},
    {1070., -scpos1x, -scpos1y, -scpos1z},
    {1080., -scpos2x, -scpos2y, -scpos2z},
    {1090.,       0., -scpos3y, -scpos3z},
    {1100., +scpos2x, -scpos2y, -scpos2z},
    {1110., +scpos1x, -scpos1y, -scpos1z},
    {1120., +scpos0x,       0.,       0.},
    // For a non-circular orbit with the average radius of 7000 km.
    {2000., +scpos0x*1.03,       0.,       0.},
    {2010., +scpos1x*1.02, +scpos1y*1.02, +scpos1z*1.02},
    {2020., +scpos2x*1.01, +scpos2y*1.01, +scpos2z*1.01},
    {2030.,            0., +scpos3y,      +scpos3z},
    {2040., -scpos2x*0.99, +scpos2y*0.99, +scpos2z*0.99},
    {2050., -scpos1x*0.98, +scpos1y*0.98, +scpos1z*0.98},
    {2060., -scpos0x*0.97,            0.,            0.},
    {2070., -scpos1x*0.98, -scpos1y*0.98, -scpos1z*0.98},
    {2080., -scpos2x*0.99, -scpos2y*0.99, -scpos2z*0.99},
    {2090.,            0., -scpos3y,      -scpos3z},
    {2100., +scpos2x*1.01, -scpos2y*1.01, -scpos2z*1.01},
    {2110., +scpos1x*1.02, -scpos1y*1.02, -scpos1z*1.02},
    {2120., +scpos0x*1.03,            0.,            0.},
    // For unphysical special cases.
    {3000., +7200000., 0., 0.},
    {3010., +4800000., 0., 0.},
    {3020., +2400000., 0., 0.},
    {3030.,        0., 0., 0.},
    {3040.,        0., 0., 0.},
    {3050.,        0., 0., 0.},
    {3060.,        0., 0., 0.},
    {3070., -2400000., 0., 0.},
    {3080., -4800000., 0., 0.},
    {3090., -7200000., 0., 0.},
    {3100., -7100000., 0., 0.},
    {3110., -7000000., 0., 0.},
    {3120., -6900000., 0., 0.}
  };
  double tolerance = 1.; // Absolute tolerance of 1 meter corresponds to 3.3 nanoseconds.
  char axis_name[3] = {'X', 'Y', 'Z'};

  // Test interpolation of spacecraft position, only if successful in file opening.
  if (0 == glastscorbit_getstatus(scptr)) {
    for (size_t ipar = 0; ipar < sizeof(par_list)/sizeof(double)/4; ++ipar) {
      double glast_time = par_list[ipar][0];
      double scpos_result[3];
      status = glastscorbit_calcpos(scptr, glast_time, scpos_result);
      double * scpos_expected = par_list[ipar] + 1;
      if (status) {
        err() << "Function glastscorbit_calcpos returns with non-zero status (" << status << ") for MET = " << glast_time <<
          "." << std::endl;
        glastscorbit_clearerr(scptr); // Clear a potential FITS read error.
      } else {
        for (int ii=0; ii<3; ++ii) {
          if (std::fabs(scpos_result[ii] - scpos_expected[ii]) > tolerance) {
            err() << "Function glastscorbit_calcpos returns " << axis_name[ii] << " = " << scpos_result[ii] << " for MET = " <<
              glast_time << ", not " << scpos_expected[ii] << " as expected." << std::endl;
          }
        }
      }
    }
  }

  // Test clean-up function.
  status = glastscorbit_close(scptr);
  if (status) {
    err() << "Function glastscorbit_close returns with non-zero status (" << status << ")." << std::endl;
  }

  // Test the original function "glastscorbit" for backward compatibility.
  for (size_t ipar = 0; ipar < sizeof(par_list)/sizeof(double)/4; ++ipar) {
    double glast_time = par_list[ipar][0];
    double * scpos_result = glastscorbit(sc_file_char, glast_time, &status);
    double * scpos_expected = par_list[ipar] + 1;
    if (status) {
      err() << "Function glastscorbit returns with non-zero status (" << status << ") for MET = " << glast_time <<
        "." << std::endl;
    } else {
      for (int ii=0; ii<3; ++ii) {
        if (std::fabs(scpos_result[ii] - scpos_expected[ii]) > tolerance) {
          err() << "Function glastscorbit returns " << axis_name[ii] << " = " << scpos_result[ii] << " for MET = " << glast_time <<
            ", not " << scpos_expected[ii] << " as expected." << std::endl;
        }
      }
    }
  }

  // Test error detections near the boundaries of spacecraft file.
  double dummy_array[3];
  double earliest_met = 1000.0;
  double latest_met   = 3120.0;
  double small_time_diff = 1.e-6; // 1 micro-second.
  double large_time_diff = 1.; // 1 second.
  typedef std::list<std::pair<double, int> > time_status_type;
  time_status_type time_status_list;
  time_status_list.push_back(std::make_pair(earliest_met - large_time_diff, TIME_OUT_BOUNDS));
  time_status_list.push_back(std::make_pair(earliest_met - small_time_diff, 0));
  time_status_list.push_back(std::make_pair(earliest_met + small_time_diff, 0));
  time_status_list.push_back(std::make_pair(latest_met   - small_time_diff, 0));
  time_status_list.push_back(std::make_pair(latest_met   + small_time_diff, 0));
  time_status_list.push_back(std::make_pair(latest_met   + large_time_diff, TIME_OUT_BOUNDS));

  scptr = glastscorbit_open(sc_file_char, "SC_DATA");
  for (time_status_type::const_iterator itor = time_status_list.begin(); itor != time_status_list.end(); ++itor) {
    double glast_time = itor->first;
    int status_expected = itor->second;
    int status_result = glastscorbit_calcpos(scptr, glast_time, dummy_array);
    if (status_result != status_expected) {
      err() << "Function glastscorbit_calcpos returns with status = " << status_result << " for MET = " << glast_time <<
        ", not " << status_expected << " as expected." << std::endl;
    }
  }
  glastscorbit_close(scptr);

  for (time_status_type::const_iterator itor = time_status_list.begin(); itor != time_status_list.end(); ++itor) {
    double glast_time = itor->first;
    int status_expected = itor->second;
    int status_result = 0;
    glastscorbit(sc_file_char, glast_time, &status_result);
    if (status_result != status_expected) {
      err() << "Function glastscorbit returns with status = " << status_result << " for MET = " << glast_time <<
        ", not " << status_expected << " as expected." << std::endl;
    }
  }

  // Test detection of a null pointer for file name.
  scptr = glastscorbit_open(0, "SC_DATA");
  status = glastscorbit_getstatus(scptr);
  if (!status) {
    err() << "Function glastscorbit_open returns with status = " << status <<
      " for a null pointer given for a spacecraft file name." << std::endl;
  }
  if (scptr->data) {
    err() << "Function glastscorbit_open returns a non-null pointer to the spacecraft data" <<
      " for a null pointer given for a spacecraft file name." << std::endl;
  }
  if (glastscorbit_close(scptr)) {
    err() << "Function glastscorbit_close fails to close a file that was attempted to be opened" <<
      " for a null pointer given for a spacecraft file name." << std::endl;
  }

  // Test detection of a null pointer for extension name.
  scptr = glastscorbit_open(sc_file_char, 0);
  status = glastscorbit_getstatus(scptr);
  if (!status) {
    err() << "Function glastscorbit_open returns with status = " << status <<
      " for a null pointer given for an extension name." << std::endl;
  }
  if (scptr->data) {
    err() << "Function glastscorbit_open returns a non-null pointer to the spacecraft data" <<
      " for a null pointer given for an extension name." << std::endl;
  }
  if (glastscorbit_close(scptr)) {
    err() << "Function glastscorbit_close fails to close a file that was attempted to be opened" <<
      " for a null pointer given for an extension name." << std::endl;
  }

  // Test detection of null pointers for both file name and extension name.
  scptr = glastscorbit_open(0, 0);
  status = glastscorbit_getstatus(scptr);
  if (!status) {
    err() << "Function glastscorbit_open returns with status = " << status <<
      " for null pointers given for a spacecraft file name and an extension name." << std::endl;
  }
  if (scptr->data) {
    err() << "Function glastscorbit_open returns a non-null pointer to the spacecraft data" <<
      " for null pointers given for a spacecraft file name and an extension name." << std::endl;
  }
  if (glastscorbit_close(scptr)) {
    err() << "Function glastscorbit_close fails to close a file that was attempted to be opened" <<
      " for null pointers given for a spacecraft file name and an extension name." << std::endl;
  }

  // Test detection of non-existing file.
  scptr = glastscorbit_open("no_such_file.fits", "SC_DATA");
  status = glastscorbit_getstatus(scptr);
  if (!status) {
    err() << "Function glastscorbit_open returns with status = " << status << " for non-existing spacecraft file." << std::endl;
  }
  if (scptr->data) {
    err() << "Function glastscorbit_open returns a non-null pointer to the spacecraft data" <<
      " for non-existing spacecraft file." << std::endl;
  }
  if (glastscorbit_close(scptr)) {
    err() << "Function glastscorbit_close fails to close a file that was attempted to be opened" <<
      " for non-existing spacecraft file." << std::endl;
  }

  glastscorbit("no_such_file.fits", 0., &status);
  if (!status) {
    err() << "Function glastscorbit returns with status = " << status << " for non-existing spacecraft file." << std::endl;
  }

  // Test detection of non-existing extension.
  scptr = glastscorbit_open(sc_file_char, "NO_SUCH_EXTENSION");
  status = glastscorbit_getstatus(scptr);
  if (!status) {
    err() << "Function glastscorbit_open returns with status = " << status << " for non-existing extension." << std::endl;
  }
  if (scptr->data) {
    err() << "Function glastscorbit_open returns a non-null pointer to the spacecraft data" <<
      " for non-existing extension." << std::endl;
  }
  if (glastscorbit_close(scptr)) {
    err() << "Function glastscorbit_close fails to close a file that was attempted to be opened" <<
      " for non-existing extension." << std::endl;
  }

  sc_file = prependDataPath("testscfile_lam.fits");
  sc_file_char = const_cast<char *>(sc_file.c_str());
  glastscorbit(sc_file_char, 2001., &status);
  if (!status) {
    err() << "Function glastscorbit returns with status = " << status << " for non-standard extension." << std::endl;
  }

  // Test detection of not-enough spacecraft data.
  sc_file = prependDataPath("testscfile_one.fits");
  sc_file_char = const_cast<char *>(sc_file.c_str());
  scptr = glastscorbit_open(sc_file_char, "SC_DATA");
  status = glastscorbit_getstatus(scptr);
  if (!status) {
    err() << "Function glastscorbit_open returns with status = " << status << " for spacecraft file with only one row." << std::endl;
  }
  if (scptr->data) {
    err() << "Function glastscorbit_open returns a non-null pointer to the spacecraft data" <<
      " for spacecraft file with only one row." << std::endl;
  }
  if (glastscorbit_close(scptr)) {
    err() << "Function glastscorbit_close fails to close a file that was attempted to be opened" <<
      " for spacecraft file with only one row." << std::endl;
  }

  glastscorbit(sc_file_char, 2001., &status);
  if (!status) {
    err() << "Function glastscorbit returns with status = " << status << " for spacecraft file with only one row." << std::endl;
  }

  // Test non-standard spacecraft file, with extention name "LOOK_AT_ME".
  sc_file = prependDataPath("testscfile_lam.fits");
  sc_file_char = const_cast<char *>(sc_file.c_str());
  scptr = glastscorbit_open(sc_file_char, "LOOK_AT_ME");
  status = glastscorbit_getstatus(scptr);
  if (status) {
    err() << "Function glastscorbit_open returns with non-zero status (" << status << ") for spacecraft file \"" <<
      sc_file << "\" and extension name \"LOOK_AT_ME\"." << std::endl;

  } else {
    status = glastscorbit_calcpos(scptr, 2001., dummy_array);
    if (status) {
      err() << "Function glastscorbit_calcpos returns with non-zero status (" << status << ") for spacecraft file \"" <<
      sc_file << "\" and extension name \"LOOK_AT_ME\"." << std::endl;
    }
  }
  glastscorbit_close(scptr);

  // Test non-standard spacecraft file, with LOOK_AT_ME extension in the second HDU and SC_DATA extension in the third.
  sc_file = prependDataPath("testscfile_3rd.fits");
  sc_file_char = const_cast<char *>(sc_file.c_str());
  scptr = glastscorbit_open(sc_file_char, "LOOK_AT_ME");
  status = glastscorbit_getstatus(scptr);
  if (status) {
    err() << "Function glastscorbit_open returns with non-zero status (" << status << ") for spacecraft file \"" <<
      sc_file << "\" and extension name \"LOOK_AT_ME\"." << std::endl;

  } else {
    status = glastscorbit_calcpos(scptr, 2001., dummy_array);
    if (status) {
      err() << "Function glastscorbit_calcpos returns with non-zero status (" << status << ") for spacecraft file \"" <<
      sc_file << "\" and extension name \"LOOK_AT_ME\"." << std::endl;
    }
  }
  glastscorbit_close(scptr);

  scptr = glastscorbit_open(sc_file_char, "SC_DATA");
  status = glastscorbit_getstatus(scptr);
  if (status) {
    err() << "Function glastscorbit_open returns with non-zero status (" << status << ") for spacecraft file \"" <<
      sc_file << "\" and extension name \"SC_DATA\"." << std::endl;

  } else {
    glastscorbit_calcpos(scptr, 1001., dummy_array);
    if (status) {
      err() << "Function glastscorbit_calcpos returns with non-zero status (" << status << ") for spacecraft file \"" <<
      sc_file << "\" and extension name \"SC_DATA\"." << std::endl;
    }
  }
  glastscorbit_close(scptr);

  glastscorbit(sc_file_char, 1001., &status);
  if (status) {
    err() << "Function glastscorbit returns with non-zero status (" << status << ") for non-standard spacecraft file \"" <<
      sc_file << "\" with \"SC_DATA\" in the third HDU." << std::endl;
  }

  // Test re-opening of an already opened spacecraft file.
  std::string sc_file1 = prependDataPath("testscfile_std.fits");
  char * sc_file1_char = const_cast<char *>(sc_file1.c_str());
  GlastScFile * scptr1 = glastscorbit_open(sc_file1_char, "SC_DATA");
  GlastScFile * scptr2 = glastscorbit_open(sc_file1_char, "SC_DATA");
  if (scptr1->data != scptr2->data) {
    err() << "Function glastscorbit_open returns different pointers for the same file/extensin names." << std::endl;
  }

  // Test closing of a "shared" spacecraft file.
  glastscorbit_close(scptr1);
  if (0 == scptr2->data) {
    err() << "Function call glastscorbit_close(scptr1) nullify a wrong pointer (scptr2)." << std::endl;
  }
  if (glastscorbit_calcpos(scptr2, 1001., dummy_array)) {
    err() << "After glastscorbit_close(scptr1), glastscorbit_calcpos with scptr2 fails." << std::endl;
  }
  glastscorbit_close(scptr2);

  // Test no re-opening of an already opened spacecraft file, in opening different extensions of the same file.
  std::string sc_file2 = prependDataPath("testscfile_3rd.fits");
  char * sc_file2_char = const_cast<char *>(sc_file2.c_str());
  scptr1 = glastscorbit_open(sc_file2_char, "SC_DATA");
  scptr2 = glastscorbit_open(sc_file2_char, "LOOK_AT_ME");
  if (scptr1->data == scptr2->data) {
    err() << "Function glastscorbit_open returns the same pointer for different extensions of the same file." << std::endl;
  }
  glastscorbit_close(scptr1);
  glastscorbit_close(scptr2);

  // Test no re-opening of an already opened spacecraft file, in opening the same extensions of different files.
  scptr1 = glastscorbit_open(sc_file1_char, "SC_DATA");
  scptr2 = glastscorbit_open(sc_file2_char, "SC_DATA");
  if (scptr1->data == scptr2->data) {
    err() << "Function glastscorbit_open returns the same pointer for same extensions of different files." << std::endl;
  }
  glastscorbit_close(scptr1);
  glastscorbit_close(scptr2);

  // Test no re-opening of an already opened spacecraft file, in opening different extensions of different files.
  scptr1 = glastscorbit_open(sc_file1_char, "SC_DATA");
  scptr2 = glastscorbit_open(sc_file2_char, "LOOK_AT_ME");
  if (scptr1->data == scptr2->data) {
    err() << "Function glastscorbit_open returns the same pointer for different extensions of different files." << std::endl;
  }
  glastscorbit_close(scptr1);
  glastscorbit_close(scptr2);

  // Test closing a null pointer.
  status = glastscorbit_close(0);
  if (status) {
    err() << "Function call glastscorbit_close(0) returns a non-zero status." << std::endl;
  }

  // Test error reporting by glastscorbit_calcpos for a null input pointer.
  status = glastscorbit_calcpos(0, 1001., dummy_array);
  if (!status) {
    err() << "Function glastscorbit_calcpos returns zero (0) for a null input pointer." << std::endl;
  }

  // Test error reporting by glastscorbit_calcpos for a null pointer to the spacecraft data table.
  GlastScFile test_scfile;
  test_scfile.data = 0;
  test_scfile.status = 0;
  status = glastscorbit_calcpos(&test_scfile, 1001., dummy_array);
  if (!status) {
    err() << "Function glastscorbit_calcpos returns zero (0), given a null pointer to the spacecraft data table." << std::endl;
  }

  // Test error reporting by glastscorbit_calcpos for a null pointer to the spacecraft data.
  GlastScData * test_scdata = 0;
  test_scfile.data = &test_scdata;
  test_scfile.status = 0;
  status = glastscorbit_calcpos(&test_scfile, 1001., dummy_array);
  if (!status) {
    err() << "Function glastscorbit_calcpos returns zero (0), given a null pointer to the spacecraft data." << std::endl;
  }

  // Test error reporting by glastscorbit_calcpos for a non-zero status.
  sc_file = prependDataPath("testscfile_std.fits");
  sc_file_char = const_cast<char *>(sc_file.c_str());
  scptr = glastscorbit_open(sc_file_char, "SC_DATA");
  scptr->status = -1000; // Bogus error status.
  status = glastscorbit_calcpos(scptr, 1001., dummy_array);
  if (!status) {
    err() << "Function glastscorbit_calcpos returns zero (0), given a bogus non-zero status." << std::endl;
  }
  glastscorbit_clearerr(scptr);
  status = glastscorbit_calcpos(scptr, 1001., dummy_array);
  if (status) {
    err() << "Function glastscorbit_calcpos returns a non-zero status, after clearing a bogus error status." << std::endl;
  }
  glastscorbit_close(scptr);
}

void TimeSystemTestApp::testGlastTimeHandler() {
  setMethod("testGlastTimeHandler");

  // Set tolerance for AbsoluteTime comparison.
  ElapsedTime time_tolerance("TT", Duration(1.e-7, "Sec"));

  // Prepare test parameters in this method.
  std::string event_file = prependDataPath("my_pulsar_events_v3.fits");
  std::string event_file_geo = prependDataPath("my_pulsar_events_geo_v3.fits");
  std::string event_file_bary = prependDataPath("my_pulsar_events_bary_v3.fits");
  std::string event_file_copy = prependDataPath("my_pulsar_events_copy_v3.fits");
  std::string event_file_fermi = prependDataPath("my_pulsar_events_v4.fits");
  std::string sc_file = prependDataPath("my_pulsar_spacecraft_data_v3r1.fits");
  std::string sc_file_fermi = prependDataPath("my_pulsar_spacecraft_data_v4.fits");
  double ra = 85.0482;
  double dec = -69.3319;
  double angular_tolerance = 1.e-8; // In degrees.
  // Note: A difference of 1.e-8 degree produces approx. 90 ns difference in barycentric times at maximum.
  double ra_close = ra + 5.e-9;
  double dec_close = dec + 5.e-9;
  double ra_wrong = ra + 1.e-8;
  double dec_wrong = dec + 1.e-8;
  double ra_opposite = ra + 180.;
  double dec_opposite = -dec;
  std::string pl_ephem = "JPL DE405";
  bool from_header = true;
  bool from_column = false;
  bool to_header = true;
  bool to_column = false;
  bool match_solar_eph = true;

  // Copy the event file for write testing.
  tip::TipFile tip_file = tip::IFileSvc::instance().openFile(event_file);
  tip_file.copyFile(event_file_copy, true);

  // Create an auto-pointer object.
  std::auto_ptr<EventTimeHandler> handler(0);

  // Test creation of GlastScTimeHandler through its createInstance method.
  handler.reset(GlastScTimeHandler::createInstance(event_file, "EVENTS"));
  if (0 == handler.get()) {
    err() << "GlastScTimeHandler::createInstance method returned a null pointer (0)." << std::endl;
  } else if (0 == dynamic_cast<GlastScTimeHandler *>(handler.get())) {
    err() << "GlastScTimeHandler::createInstance method did not return a GlastScTimeHandler object." << std::endl;
  }

  // Test creation of GlastGeoTimeHandler through its createInstance method.
  handler.reset(GlastGeoTimeHandler::createInstance(event_file_geo, "EVENTS"));
  if (0 == handler.get()) {
    err() << "GlastGeoTimeHandler::createInstance method returned a null pointer (0)." << std::endl;
  } else if (0 == dynamic_cast<GlastGeoTimeHandler *>(handler.get())) {
    err() << "GlastGeoTimeHandler::createInstance method did not return a GlastGeoTimeHandler object." << std::endl;
  }

  // Test creation of GlastBaryTimeHandler through its createInstance method.
  handler.reset(GlastBaryTimeHandler::createInstance(event_file_bary, "EVENTS"));
  if (0 == handler.get()) {
    err() << "GlastBaryTimeHandler::createInstance method returned a null pointer (0)." << std::endl;
  } else if (0 == dynamic_cast<GlastBaryTimeHandler *>(handler.get())) {
    err() << "GlastBaryTimeHandler::createInstance method did not return a GlastBaryTimeHandler object." << std::endl;
  }

  // Test non-creation of GlastScTimeHandler through its createInstance method for a geocentered file.
  handler.reset(GlastScTimeHandler::createInstance(event_file_geo, "EVENTS"));
  if (0 != handler.get()) {
    err() << "GlastScTimeHandler::createInstance method did not return a null pointer (0)." << std::endl;
  }

  // Test non-creation of GlastScTimeHandler through its createInstance method for a barycentered file.
  handler.reset(GlastScTimeHandler::createInstance(event_file_bary, "EVENTS"));
  if (0 != handler.get()) {
    err() << "GlastScTimeHandler::createInstance method did not return a null pointer (0)." << std::endl;
  }

  // Test non-creation of GlastGeoTimeHandler through its createInstance method for a uncorrected file.
  handler.reset(GlastGeoTimeHandler::createInstance(event_file, "EVENTS"));
  if (0 != handler.get()) {
    err() << "GlastGeoTimeHandler::createInstance method did not return a null pointer (0)." << std::endl;
  }

  // Test non-creation of GlastGeoTimeHandler through its createInstance method for a barycentered file.
  handler.reset(GlastGeoTimeHandler::createInstance(event_file_bary, "EVENTS"));
  if (0 != handler.get()) {
    err() << "GlastGeoTimeHandler::createInstance method did not return a null pointer (0)." << std::endl;
  }

  // Test non-creation of GlastBaryTimeHandler through its createInstance method for a uncorrected file.
  handler.reset(GlastBaryTimeHandler::createInstance(event_file, "EVENTS"));
  if (0 != handler.get()) {
    err() << "GlastBaryTimeHandler::createInstance method did not return a null pointer (0)." << std::endl;
  }

  // Test non-creation of GlastBaryTimeHandler through its createInstance method for a geocentered file.
  handler.reset(GlastBaryTimeHandler::createInstance(event_file_geo, "EVENTS"));
  if (0 != handler.get()) {
    err() << "GlastBaryTimeHandler::createInstance method did not return a null pointer (0)." << std::endl;
  }

  // Test creation of GlastScTimeHandler through GlastTimeHandler::createInstance method.
  handler.reset(GlastTimeHandler::createInstance(event_file, "EVENTS"));
  if (0 == handler.get()) {
    err() << "GlastTimeHandler::createInstance method returned a null pointer (0)." << std::endl;
  } else if (0 == dynamic_cast<GlastScTimeHandler *>(handler.get())) {
    err() << "GlastTimeHandler::createInstance method did not return a GlastScTimeHandler object." << std::endl;
  }

  // Test creation of GlastGeoTimeHandler through GlastTimeHandler:: createInstance method.
  handler.reset(GlastTimeHandler::createInstance(event_file_geo, "EVENTS"));
  if (0 == handler.get()) {
    err() << "GlastTimeHandler::createInstance method returned a null pointer (0)." << std::endl;
  } else if (0 == dynamic_cast<GlastGeoTimeHandler *>(handler.get())) {
    err() << "GlastTimeHandler::createInstance method did not return a GlastGeoTimeHandler object." << std::endl;
  }

  // Test creation of GlastBaryTimeHandler through GlastTimeHandler:: createInstance method.
  handler.reset(GlastTimeHandler::createInstance(event_file_bary, "EVENTS"));
  if (0 == handler.get()) {
    err() << "GlastTimeHandler::createInstance method returned a null pointer (0)." << std::endl;
  } else if (0 == dynamic_cast<GlastBaryTimeHandler *>(handler.get())) {
    err() << "GlastTimeHandler::createInstance method did not return a GlastBaryTimeHandler object." << std::endl;
  }

  // Create a GlastScTimeHandler object for EVENTS extension of an event file.
  handler.reset(GlastScTimeHandler::createInstance(event_file, "EVENTS"));

  // Test setting to the first record.
  handler->setFirstRecord();
  double glast_time;
  handler->getCurrentRecord()["TIME"].get(glast_time);
  double expected_glast_time = 2.123393701794728E+08; // TIME in the first row of my_pulsar_events_v3.fits.
  double epsilon = 1.e-7; // 100 nanoseconds.
  if (std::fabs(glast_time - expected_glast_time) > epsilon) {
    err() << "GlastScTimeHandler::getCurrentRecord() did not return the first record after GlastScTimeHandler::setFirstRecord()." <<
      std::endl;
  }

  // Test setting to the third record.
  handler->setFirstRecord();
  handler->setNextRecord();
  handler->setNextRecord();
  handler->getCurrentRecord()["TIME"].get(glast_time);
  expected_glast_time = 2.123393750454886E+08; // TIME in the third row of my_pulsar_events_v3.fits.
  if (std::fabs(glast_time - expected_glast_time) > epsilon) {
    err() << "GlastScTimeHandler::getCurrentRecord() did not return the third record after GlastScTimeHandler::setFirstRecord()" <<
      " followed by two GlastScTimeHandler::setNextRecord() calls." << std::endl;
  }

  // Test setting to the last record.
  handler->setLastRecord();
  handler->getCurrentRecord()["TIME"].get(glast_time);
  expected_glast_time = 2.124148548657289E+08; // TIME in the last row of my_pulsar_events_v3.fits.
  if (std::fabs(glast_time - expected_glast_time) > epsilon) {
    err() << "GlastScTimeHandler::getCurrentRecord() did not return the last record after GlastScTimeHandler::setLastRecord()." <<
      std::endl;
  }

  // Test testing the end of table.
  handler->setLastRecord();
  if (handler->isEndOfTable()) {
    err() << "GlastScTimeHandler::isEndOfTable() returned true after GlastScTimeHandler::setLastRecord()." << std::endl;
  }
  handler->setNextRecord();
  if (!handler->isEndOfTable()) {
    err() << "GlastScTimeHandler::isEndOfTable() returned false after GlastScTimeHandler::setLastRecord()" <<
      " followed by GlastScTimeHandler::setNextRecord()." << std::endl;
  }

  // Test parsing a time string.
  std::string time_string = "12345.6789012345";
  AbsoluteTime result = handler->parseTimeString(time_string);
  AbsoluteTime glast_tt_origin("TT", 51910, 64.184);
  AbsoluteTime expected = glast_tt_origin + ElapsedTime("TT", Duration(12345.6789012345, "Sec"));
  if (!result.equivalentTo(expected, time_tolerance)) {
    err() << "GlastScTimeHandler::parseTimeString(\"" << time_string << "\") returned AbsoluteTime(" << result <<
      "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
  }

  // Test parsing a time string, with a different time system.
  time_string = "12345.6789012345";
  result = handler->parseTimeString(time_string, "TDB");
  AbsoluteTime glast_tdb_origin("TDB", 51910, 64.184);
  expected = glast_tdb_origin + ElapsedTime("TDB", Duration(12345.6789012345, "Sec"));
  if (!result.equivalentTo(expected, time_tolerance)) {
    err() << "GlastScTimeHandler::parseTimeString(\"" << time_string << "\", \"TDB\") returned AbsoluteTime(" << result <<
      "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
  }

  // Test reading header keyword value.
  result = handler->readTime("TSTART", from_header);
  glast_time = 2.123393677090199E+08; // TSTART in my_pulsar_events_v3.fits.
  expected = glast_tt_origin + ElapsedTime("TT", Duration(glast_time, "Sec"));
  if (!result.equivalentTo(expected, time_tolerance)) {
    err() << "GlastScTimeHandler::readTime(\"TSTART\", " << from_header << ") returned AbsoluteTime(" << result <<
      "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
  }

  // Test reading header keyword value, in Calendar date & time format.
  result = handler->readTime("DATE-OBS", from_header);
  time_string = "2007-09-24T15:09:31"; // DATE-OBS in my_pulsar_events_v3.fits.
  expected = AbsoluteTime("UTC", CalendarFmt, time_string);
  if (!result.equivalentTo(expected, time_tolerance)) {
    err() << "GlastScTimeHandler::readTime(\"DATE-OBS\", " << from_header << ") returned AbsoluteTime(" << result <<
      "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
  }

  // Test reading header keyword value, requesting geocentering, before initializing handler for arrival time corrections.
  try {
    result = handler->getGeoTime("TSTART", from_header);
    err() << "GlastScTimeHandler::getGeoTime(\"TSTART\", " << from_header << ") did not throw an exception when it should." <<
      std::endl;
  } catch (const std::exception &) {
  }

  // Test reading header keyword value, requesting barycentering, before initializing handler for arrival time corrections.
  try {
    result = handler->getBaryTime("TSTART", from_header);
    err() << "GlastScTimeHandler::getBaryTime(\"TSTART\", " << from_header << ") did not throw an exception when it should." <<
      std::endl;
  } catch (const std::exception &) {
  }

  // Initialize handler for arrival time corrections.
  handler->initTimeCorrection(sc_file, "SC_DATA", pl_ephem, match_solar_eph, angular_tolerance);
  handler->setSourcePosition(SourcePosition(ra, dec));

  // Test reading header keyword value, requesting geocentering.
  result = handler->getGeoTime("TSTART", from_header);
  glast_time = 212339367.70603555441; // Once computed by BaryTimeComputer::computeGeoTime method.
  expected = glast_tt_origin + ElapsedTime("TT", Duration(glast_time, "Sec"));
  if (!result.equivalentTo(expected, time_tolerance)) {
    err() << "GlastScTimeHandler::getGeoTime(\"TSTART\", " << from_header << ") returned AbsoluteTime(" << result <<
      "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
  }

  // Test reading header keyword value, requesting barycentering.
  result = handler->getBaryTime("TSTART", from_header);
  glast_time = 2.123393824137859E+08; // TSTART in my_pulsar_events_bary_v3.fits.
  expected = glast_tdb_origin + ElapsedTime("TDB", Duration(glast_time, "Sec"));
  if (!result.equivalentTo(expected, time_tolerance)) {
    err() << "GlastScTimeHandler::getBaryTime(\"TSTART\", " << from_header << ") returned AbsoluteTime(" << result <<
      "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
  }

  // Test reading TIME column value.
  handler->setFirstRecord(); // Points to the first event.
  handler->setNextRecord();  // Points to the second event.
  handler->setNextRecord();  // Points to the third event.
  result = handler->readTime("TIME", from_column);
  glast_time = 2.123393750454886E+08; // TIME of the third row in my_pulsar_events_v3.fits.
  expected = glast_tt_origin + ElapsedTime("TT", Duration(glast_time, "Sec"));
  if (!result.equivalentTo(expected, time_tolerance)) {
    err() << "GlastScTimeHandler::readTime(\"TIME\", " << from_column << ") returned AbsoluteTime(" << result <<
      "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
  }

  // Test reading TIME column value, requesting geocentering.
  handler->setFirstRecord(); // Re-setting to the first event.
  handler->setNextRecord();  // Points to the second event.
  handler->setNextRecord();  // Points to the third event.
  result = handler->getGeoTime("TIME", from_column);
  glast_time = 212339375.04258754849; // Once computed by BaryTimeComputer::computeGeoTime method.
  expected = glast_tt_origin + ElapsedTime("TT", Duration(glast_time, "Sec"));
  if (!result.equivalentTo(expected, time_tolerance)) {
    err() << "GlastScTimeHandler::getGeoTime(\"TIME\", " << from_column << ") returned AbsoluteTime(" << result <<
      "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
  }

  // Test reading TIME column value, requesting barycentering.
  handler->setFirstRecord(); // Re-setting to the first event.
  handler->setNextRecord();  // Points to the second event.
  handler->setNextRecord();  // Points to the third event.
  result = handler->getBaryTime("TIME", from_column);
  glast_time = 2.123393897503012E+08; // TIME of the third row in my_pulsar_events_bary_v3.fits.
  expected = glast_tdb_origin + ElapsedTime("TDB", Duration(glast_time, "Sec"));
  if (!result.equivalentTo(expected, time_tolerance)) {
    err() << "GlastScTimeHandler::getBaryTime(\"TIME\", " << from_column << ") returned AbsoluteTime(" << result <<
      "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
  }

  // Create a GlastScTimeHandler object for EVENTS extension of a copied event file for write testing.
  handler.reset(GlastScTimeHandler::createInstance(event_file_copy, "EVENTS", false));

  // Test writing header keyword value.
  double expected_double = 12345678.1234567;
  AbsoluteTime abs_time = glast_tt_origin + ElapsedTime("TT", Duration(expected_double, "Sec"));
  handler->writeTime("TSTART", abs_time, to_header);
  const tip::Header & header = handler->getHeader();
  double result_double = 0.;
  header["TSTART"].get(result_double);
  double tolerance_double = 1.e-9; // 1 nanosecond.
  if (std::fabs(result_double - expected_double) > tolerance_double) {
    err() << "GlastScTimeHandler::writeTime(\"TSTART\", " << to_header << ") wrote " << result_double <<
      ", not equivalent to " << expected_double << " with tolerance of " << tolerance_double << "." << std::endl;
  }

  // Test writing header keyword value, in Calendar date & time format.
  std::string expected_time_string = "2008-10-25T12:34:56.1234";
  abs_time = AbsoluteTime("UTC", CalendarFmt, expected_time_string);
  handler->writeTime("DATE-OBS", abs_time, to_header);
  std::string result_time_string;
  header["DATE-OBS"].get(result_time_string);
  if (result_time_string != expected_time_string) {
    err() << "GlastScTimeHandler::writeTime(\"DATE-OBS\", " << to_header << ") wrote " << result_time_string <<
      ", not " << expected_time_string << " as expected." << std::endl;
  }

  // Test writing TIME column value.
  expected_double = 12345678.1234567;
  abs_time = glast_tt_origin + ElapsedTime("TT", Duration(expected_double, "Sec"));
  handler->setFirstRecord(); // Points to the first event.
  handler->setNextRecord();  // Points to the second event.
  handler->setNextRecord();  // Points to the third event.
  handler->writeTime("TIME", abs_time, to_column);
  const tip::TableRecord & record = handler->getCurrentRecord();
  result_double = 0.;
  record["TIME"].get(result_double);
  if (std::fabs(result_double - expected_double) > tolerance_double) {
    err() << "GlastScTimeHandler::writeTime(\"TIME\", " << to_column << ") wrote " << result_double <<
      ", not equivalent to " << expected_double << " with tolerance of " << tolerance_double << "." << std::endl;
  }

  // Create an GlastBaryTimeHandler object for EVENTS extension of a barycentered event file.
  handler.reset(GlastBaryTimeHandler::createInstance(event_file_bary, "EVENTS"));
  handler->initTimeCorrection(sc_file, "SC_DATA", pl_ephem, match_solar_eph, angular_tolerance);
  handler->setSourcePosition(SourcePosition(ra, dec));

  // Test parsing a time string.
  time_string = "12345.6789012345";
  result = handler->parseTimeString(time_string);
  expected = glast_tdb_origin + ElapsedTime("TDB", Duration(12345.6789012345, "Sec"));
  if (!result.equivalentTo(expected, time_tolerance)) {
    err() << "GlastBaryTimeHandler::parseTimeString(\"" << time_string << "\") returned AbsoluteTime(" << result <<
      "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
  }

  // Test parsing a time string, with a different time system.
  time_string = "12345.6789012345";
  result = handler->parseTimeString(time_string, "TT");
  expected = glast_tt_origin + ElapsedTime("TT", Duration(12345.6789012345, "Sec"));
  if (!result.equivalentTo(expected, time_tolerance)) {
    err() << "GlastBaryTimeHandler::parseTimeString(\"" << time_string << "\", \"TT\") returned AbsoluteTime(" << result <<
      "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
  }

  // Test reading header keyword value, requesting geocentering.
  try {
    result = handler->getGeoTime("TSTART", from_header);
    err() << "GlastBaryTimeHandler::getGeoTime(\"TSTART\", " << from_header << ") did not throw an exception when it should." <<
      std::endl;
  } catch (const std::exception &) {
  }

  // Test reading header keyword value, requesting barycentering.
  result = handler->getBaryTime("TSTART", from_header);
  glast_time = 2.123393824137859E+08; // TSTART in my_pulsar_events_bary_v3.fits.
  expected = glast_tdb_origin + ElapsedTime("TDB", Duration(glast_time, "Sec"));
  if (!result.equivalentTo(expected, time_tolerance)) {
    err() << "GlastBaryTimeHandler::getBaryTime(\"TSTART\", " << from_header << ") returned AbsoluteTime(" << result <<
      "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
  }

  // Test reading column value, requesting geocentering.
  try {
    result = handler->getGeoTime("TIME", from_column);
    err() << "GlastBaryTimeHandler::getGeoTime(\"TIME\", " << from_column << ") did not throw an exception when it should." <<
      std::endl;
  } catch (const std::exception &) {
  }

  // Test reading column value, requesting barycentering.
  handler->setFirstRecord(); // Points to the first event.
  handler->setNextRecord();  // Points to the second event.
  handler->setNextRecord();  // Points to the third event.
  result = handler->getBaryTime("TIME", from_column);
  glast_time = 2.123393897503012E+08; // TIME of the third row in my_pulsar_events_bary_v3.fits.
  expected = glast_tdb_origin + ElapsedTime("TDB", Duration(glast_time, "Sec"));
  if (!result.equivalentTo(expected, time_tolerance)) {
    err() << "GlastBaryTimeHandler::getBaryTime(\"TIME\", " << from_column << ") returned AbsoluteTime(" << result <<
      "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
  }

  // Test setting a wrong sky position (ra, dec).
  try {
    handler->setSourcePosition(SourcePosition(ra_wrong, dec_wrong));
    err() << "GlastBaryTimeHandler::setSourcePosition(SourcePosition(" << ra_wrong << ", " << dec_wrong << 
      ")) did not throw an exception when it should." << std::endl;
  } catch (const std::exception &) {
  }

  // Test setting a different, but close sky position (ra, dec).
  try {
    handler->setSourcePosition(SourcePosition(ra_close, dec_close));
  } catch (const std::exception &) {
    err() << "GlastBaryTimeHandler::setSourcePosition(SourcePosition(" << ra_close << ", " << dec_close << 
      ")) threw an exception when it should not." << std::endl;
  }

  // Test exact match in sky position (ra, dec), with angular tolerance of zero (0) degree.
  angular_tolerance = 0.;
  handler->initTimeCorrection(sc_file, "SC_DATA", pl_ephem, match_solar_eph, angular_tolerance);
  try {
    handler->setSourcePosition(SourcePosition(ra, dec));
  } catch (const std::exception &) {
    err() << "GlastBaryTimeHandler::setSourcePosition(SourcePosition(" << ra << ", " << dec << 
      ")) threw an exception with angular tolerance of zero (0) degree." << std::endl;
  }

  // Test large angular tolerance over 180 degrees.
  angular_tolerance = 180.1;
  handler->initTimeCorrection(sc_file, "SC_DATA", pl_ephem, match_solar_eph, angular_tolerance);
  try {
    handler->setSourcePosition(SourcePosition(ra_wrong, dec_wrong));
  } catch (const std::exception &) {
    err() << "GlastBaryTimeHandler::setSourcePosition(SourcePosition(" << ra_wrong << ", " << dec_wrong <<
      ")) threw an exception with angular tolerance of 180.1 degrees." << std::endl;
  }

  // Test large angular difference, with small angular tolerance.
  angular_tolerance = 1.e-8;
  handler->initTimeCorrection(sc_file, "SC_DATA", pl_ephem, match_solar_eph, angular_tolerance);
  try {
    handler->setSourcePosition(SourcePosition(ra_opposite, dec_opposite));
    err() << "GlastBaryTimeHandler::setSourcePosition(SourcePosition(" << ra_opposite << ", " << dec_opposite <<
      ")) did not throw an exception with angular tolerance of 1e-8 degrees." << std::endl;
  } catch (const std::exception &) {
  }

  // Test large angular difference, with large angular tolerance over 180 degrees.
  angular_tolerance = 180.1;
  handler->initTimeCorrection(sc_file, "SC_DATA", pl_ephem, match_solar_eph, angular_tolerance);
  try {
    handler->setSourcePosition(SourcePosition(ra_opposite, dec_opposite));
  } catch (const std::exception &) {
    err() << "GlastBaryTimeHandler::setSourcePosition(SourcePosition(" << ra_opposite << ", " << dec_opposite <<
      ")) threw an exception with angular tolerance of 180.1 degrees." << std::endl;
  }

  // Test checking solar system ephemeris name, with a non-barycentered event extension.
  handler.reset(GlastScTimeHandler::createInstance(event_file, "EVENTS"));
  angular_tolerance = 1.e-8;
  try {
    handler->initTimeCorrection(sc_file, "SC_DATA", "Bogus Name", match_solar_eph, angular_tolerance);
    err() << "GlastScTimeHandler::initTimeCorrection(\"" << sc_file << "\", \"SC_DATA\", \"Bogus Name\", " << match_solar_eph <<
      ", " << angular_tolerance << ") did not throw an exception for non-barycentered event extension." << std::endl;
  } catch (const std::exception &) {
  }

  // Test checking solar system ephemeris name, with a barycentered event extension (match).
  handler.reset(GlastBaryTimeHandler::createInstance(event_file_bary, "EVENTS"));
  try {
    handler->initTimeCorrection(sc_file, "SC_DATA", "JPL DE405", match_solar_eph, angular_tolerance);
  } catch (const std::exception &) {
    err() << "GlastBaryTimeHandler::initTimeCorrection(\"" << sc_file << "\", \"SC_DATA\", \"JPL DE405\", " << match_solar_eph <<
      ", " << angular_tolerance << ") threw an exception for a barycentered event extension." << std::endl;
  }

  // Test checking solar system ephemeris name, with a barycentered event extension (no match).
  try {
    handler->initTimeCorrection(sc_file, "SC_DATA", "JPL DE200", match_solar_eph, angular_tolerance);
    err() << "GlastBaryTimeHandler::initTimeCorrection(\"" << sc_file << "\", \"SC_DATA\", \"JPL DE200\", " << match_solar_eph <<
      ", " << angular_tolerance << ") did not throw an exception for a barycentered event extension." << std::endl;
  } catch (const std::exception &) {
  }

  // Test checking solar system ephemeris name, with a barycentered GTI extension (rough match).
  handler.reset(GlastBaryTimeHandler::createInstance(event_file_bary, "GTI"));
  try {
    handler->initTimeCorrection(sc_file, "SC_DATA", "JPL DE405", match_solar_eph, angular_tolerance);
  } catch (const std::exception &) {
    err() << "GlastBaryTimeHandler::initTimeCorrection(\"" << sc_file << "\", \"SC_DATA\", \"JPL DE405\", " << match_solar_eph <<
      ", " << angular_tolerance << ") threw an exception for a barycentered GTI extension." << std::endl;
  }

  // Test checking solar system ephemeris name, with a barycentered GTI extension (no match).
  try {
    handler->initTimeCorrection(sc_file, "SC_DATA", "JPL DE200", match_solar_eph, angular_tolerance);
    err() << "GlastBaryTimeHandler::initTimeCorrection(\"" << sc_file << "\", \"SC_DATA\", \"JPL DE200\", " << match_solar_eph <<
      ", " << angular_tolerance << ") did not throw an exception for a barycentered GTI extension." << std::endl;
  } catch (const std::exception &) {
  }

  // Create an GlastGeoTimeHandler object for EVENTS extension of a geocentered event file.
  handler.reset(GlastGeoTimeHandler::createInstance(event_file_geo, "EVENTS"));
  handler->initTimeCorrection(sc_file, "SC_DATA", pl_ephem, match_solar_eph, angular_tolerance); // This has no effect.
  handler->setSourcePosition(SourcePosition(ra, dec)); // This has no effect.

  // Test parsing a time string.
  time_string = "12345.6789012345";
  result = handler->parseTimeString(time_string);
  expected = glast_tt_origin + ElapsedTime("TT", Duration(12345.6789012345, "Sec"));
  if (!result.equivalentTo(expected, time_tolerance)) {
    err() << "GlastGeoTimeHandler::parseTimeString(\"" << time_string << "\") returned AbsoluteTime(" << result <<
      "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
  }

  // Test parsing a time string, with a different time system.
  time_string = "12345.6789012345";
  result = handler->parseTimeString(time_string, "TDB");
  expected = glast_tdb_origin + ElapsedTime("TDB", Duration(12345.6789012345, "Sec"));
  if (!result.equivalentTo(expected, time_tolerance)) {
    err() << "GlastGeoTimeHandler::parseTimeString(\"" << time_string << "\", \"TDB\") returned AbsoluteTime(" << result <<
      "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
  }

  // Test reading header keyword value, requesting geocentering.
  result = handler->getGeoTime("TSTART", from_header);
  glast_time = 2.12339367706036E+08; // TSTART in my_pulsar_events_geo_v3.fits.
  expected = glast_tt_origin + ElapsedTime("TT", Duration(glast_time, "Sec"));
  if (!result.equivalentTo(expected, time_tolerance)) {
    err() << "GlastGeoTimeHandler::getGeoTime(\"TSTART\", " << from_header << ") returned AbsoluteTime(" << result <<
      "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
  }

  // Test reading column value, requesting geocentering.
  handler->setFirstRecord(); // Points to the first event.
  handler->setNextRecord();  // Points to the second event.
  handler->setNextRecord();  // Points to the third event.
  result = handler->getGeoTime("TIME", from_column);
  glast_time = 2.123393750425875E+08; // TIME of the third row in my_pulsar_events_geo_v3.fits.
  expected = glast_tt_origin + ElapsedTime("TT", Duration(glast_time, "Sec"));
  if (!result.equivalentTo(expected, time_tolerance)) {
    err() << "GlastGeoTimeHandler::getGeoTime(\"TIME\", " << from_column << ") returned AbsoluteTime(" << result <<
      "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
  }

  // Test reading column value, requesting barycentering.
  try {
    result = handler->getBaryTime("TIME", from_column);
    err() << "GlastGeoTimeHandler::getBaryTime(\"TIME\", " << from_column << ") did not throw an exception when it should." <<
      std::endl;
  } catch (const std::exception &) {
  }

  // Test reading header keyword value, requesting barycentering.
  try {
    result = handler->getBaryTime("TSTART", from_header);
    err() << "GlastGeoTimeHandler::getBaryTime(\"TSTART\", " << from_header << ") did not throw an exception when it should." <<
      std::endl;
  } catch (const std::exception &) {
  }

  // Test setting a wrong sky position (ra, dec).
  try {
    handler->setSourcePosition(SourcePosition(ra_wrong, dec_wrong));
  } catch (const std::exception &) {
    err() << "GlastGeoTimeHandler::setSourcePosition(SourcePosition(" << ra_wrong << ", " << dec_wrong <<
      ")) threw an exception when it should not." << std::endl;
  }

  // Test setting a different, but close sky position (ra, dec).
  try {
    handler->setSourcePosition(SourcePosition(ra_close, dec_close));
  } catch (const std::exception &) {
    err() << "GlastGeoTimeHandler::setSourcePosition(SourcePosition(" << ra_close << ", " << dec_close <<
      ")) threw an exception when it should not." << std::endl;
  }

  // Test loading of an event file with TELESCOP = FERMI.
  handler.reset(GlastScTimeHandler::createInstance(event_file_fermi, "EVENTS"));
  if (0 == handler.get()) {
    err() << "GlastScTimeHandler::createInstance method returned a null pointer (0)." << std::endl;
  } else if (0 == dynamic_cast<GlastScTimeHandler *>(handler.get())) {
    err() << "GlastScTimeHandler::createInstance method did not return a GlastScTimeHandler object." << std::endl;
  }

  // Test Initialization of handler with spacecraft file with TELESCOP = FERMI.
  try {
    handler->initTimeCorrection(sc_file_fermi, "SC_DATA", pl_ephem, match_solar_eph, angular_tolerance);
  } catch (const std::exception &) {
    err() << "GlastScTimeHandler::initTimeCorrection method threw an exception for spacecraft file with TELESCOP=FERMI." << std::endl;
  }

  // Test arrival time corrections near the boundaries of spacecraft file.
  double earliest_met = 212322400.0;
  double latest_met   = 212422420.0;
  double small_time_diff = 1.e-6; // 1 micro-second.
  double large_time_diff = 1.; // 1 second.

  // Create a GlastScTimeHandler object for EVENTS extension of a copied event file for testing detections of time boundaries.
  handler.reset(GlastScTimeHandler::createInstance(event_file_copy, "EVENTS", false));
  handler->initTimeCorrection(sc_file, "SC_DATA", pl_ephem, match_solar_eph, angular_tolerance);
  handler->setSourcePosition(SourcePosition(ra, dec)); // This has no effect.

  // Test detection of time out of bounds, for a time too early.
  handler->getHeader().setKeyword("TSTART", earliest_met - large_time_diff);
  try {
    handler->getBaryTime("TSTART", from_header);
    err() << "GlastScTimeHandler::getBaryTime(\"TSTART\", " << from_header << ") did not throw an exception for the time " <<
      large_time_diff << " second(s) earlier than the earliest time covered by the spacecraft file." << std::endl;
  } catch (const std::exception &) {
  }

  // Test non-detection of time out of bounds, for a time slightly too early.
  handler->getHeader().setKeyword("TSTART", earliest_met - small_time_diff);
  try {
    handler->getBaryTime("TSTART", from_header);
  } catch (const std::exception &) {
    err() << "GlastScTimeHandler::getBaryTime(\"TSTART\", " << from_header << ") threw an exception for the time " <<
      small_time_diff << " second(s) earlier than the earliest time covered by the spacecraft file." << std::endl;
  }

  // Test non-detection of time out of bounds, for a time in-bounds.
  handler->getHeader().setKeyword("TSTART", earliest_met + small_time_diff);
  try {
    handler->getBaryTime("TSTART", from_header);
  } catch (const std::exception &) {
    err() << "GlastScTimeHandler::getBaryTime(\"TSTART\", " << from_header << ") threw an exception for the time " <<
      small_time_diff << " second(s) later than the earliest time covered by the spacecraft file." << std::endl;
  }

  // Test non-detection of time out of bounds, for a time in-bounds.
  handler->getHeader().setKeyword("TSTART", latest_met - small_time_diff);
  try {
    handler->getBaryTime("TSTART", from_header);
  } catch (const std::exception &) {
    err() << "GlastScTimeHandler::getBaryTime(\"TSTART\", " << from_header << ") threw an exception for the time " <<
      small_time_diff << " second(s) earlier than the latest time covered by the spacecraft file." << std::endl;
  }

  // Test non-detection of time out of bounds, for a time slightly late.
  handler->getHeader().setKeyword("TSTART", latest_met + small_time_diff);
  try {
    handler->getBaryTime("TSTART", from_header);
  } catch (const std::exception &) {
    err() << "GlastScTimeHandler::getBaryTime(\"TSTART\", " << from_header << ") threw an exception for the time " <<
      small_time_diff << " second(s) later than the latest time covered by the spacecraft file." << std::endl;
  }

  // Test detection of time out of bounds, for a time too late.
  handler->getHeader().setKeyword("TSTART", latest_met + large_time_diff);
  try {
    handler->getBaryTime("TSTART", from_header);
    err() << "GlastScTimeHandler::getBaryTime(\"TSTART\", " << from_header << ") did not throw an exception for the time " <<
      large_time_diff << " second(s) later than the latest time covered by the spacecraft file." << std::endl;
  } catch (const std::exception &) {
  }
}

void TimeSystemTestApp::testTimeCorrectorApp() {
  setMethod("testTimeCorrectorApp");

  // Create an application tester object.
  TimeCorrectorAppTester app_tester(*this);

  // Prepare variables to create application objects.
  std::list<std::string> test_name_cont;
  test_name_cont.push_back("par1");
  test_name_cont.push_back("par2");
  test_name_cont.push_back("par3");
  test_name_cont.push_back("par4");
  test_name_cont.push_back("par5");
  test_name_cont.push_back("par6");

  // Prepare settings to be used in the tests.
  std::string evfile_0540 = prependDataPath("my_pulsar_events_v3r1.fits");
  std::string scfile_0540 = prependDataPath("my_pulsar_spacecraft_data_v3r1.fits");
  double ra_0540 = 85.0482;
  double dec_0540 = -69.3319;
  std::string evfile_bary = prependDataPath("my_pulsar_events_bary_v3r1.fits");
  std::string evfile_geo = prependDataPath("my_pulsar_events_geo_v3r1.fits");

  // Loop over parameter sets.
  for (std::list<std::string>::const_iterator test_itor = test_name_cont.begin(); test_itor != test_name_cont.end(); ++test_itor) {
    const std::string & test_name = *test_itor;
    std::string log_file(getMethod() + "_" + test_name + ".log");
    std::string log_file_ref(getMethod() + "_" + test_name + ".ref");
    std::string out_file(getMethod() + "_" + test_name + ".fits");
    std::string out_file_ref(prependOutrefPath(out_file));
    bool ignore_exception(false);

    // Set default parameters.
    st_app::AppParGroup pars(app_tester.getName());
    pars["evfile"] = "";
    pars["scfile"] = "";
    pars["outfile"] = "";
    pars["ra"] = 0.;
    pars["dec"] = 0.;
    pars["tcorrect"] = "BARY";
    pars["solareph"] = "JPL DE405";
    pars["angtol"] = 1.e-8;
    pars["timefield"] = "TIME";
    pars["sctable"] = "SC_DATA";
    pars["leapsecfile"] = "DEFAULT";
    pars["chatter"] = 2;
    pars["clobber"] = "yes";
    pars["debug"] = "no";
    pars["gui"] = "no";
    pars["mode"] = "ql";

    // Set test-specific parameters.
    if ("par1" == test_name) {
      // Test barycentric corrections.
      pars["evfile"] = evfile_0540;
      pars["scfile"] = scfile_0540;
      pars["outfile"] = out_file;
      pars["ra"] = ra_0540;
      pars["dec"] = dec_0540;
      pars["tcorrect"] = "BARY";

      log_file.erase();
      log_file_ref.erase();

    } else if ("par2" == test_name) {
      // Test geocentric corrections.
      pars["evfile"] = evfile_0540;
      pars["scfile"] = scfile_0540;
      pars["outfile"] = out_file;
      pars["ra"] = ra_0540;
      pars["dec"] = dec_0540;
      pars["tcorrect"] = "GEO";

      log_file.erase();
      log_file_ref.erase();

    } else if ("par3" == test_name) {
      // Test refusal of barycentric corrections on a barycentered file.
      pars["evfile"] = evfile_bary;
      pars["scfile"] = scfile_0540;
      pars["outfile"] = out_file;
      pars["ra"] = ra_0540;
      pars["dec"] = dec_0540;
      pars["tcorrect"] = "BARY";

      remove(log_file_ref.c_str());
      std::ofstream ofs(log_file_ref.c_str());
      std::runtime_error error("Unsupported timing extension: HDU 1 (EXTNAME=EVENTS) of input file \"" + evfile_bary + "\"");
      app_tester.writeException(ofs, error);
      ofs.close();

      out_file.erase();
      out_file_ref.erase();
      ignore_exception = true;

    } else if ("par4" == test_name) {
      // Test refusal of geocentric corrections on a barycentered file.
      pars["evfile"] = evfile_bary;
      pars["scfile"] = scfile_0540;
      pars["outfile"] = out_file;
      pars["ra"] = ra_0540;
      pars["dec"] = dec_0540;
      pars["tcorrect"] = "GEO";

      remove(log_file_ref.c_str());
      std::ofstream ofs(log_file_ref.c_str());
      std::runtime_error error("Unsupported timing extension: HDU 1 (EXTNAME=EVENTS) of input file \"" + evfile_bary + "\"");
      app_tester.writeException(ofs, error);
      ofs.close();

      out_file.erase();
      out_file_ref.erase();
      ignore_exception = true;

    } else if ("par5" == test_name) {
      // Test refusal of barycentric corrections on a geocentered file.
      pars["evfile"] = evfile_geo;
      pars["scfile"] = scfile_0540;
      pars["outfile"] = out_file;
      pars["ra"] = ra_0540;
      pars["dec"] = dec_0540;
      pars["tcorrect"] = "BARY";

      remove(log_file_ref.c_str());
      std::ofstream ofs(log_file_ref.c_str());
      std::runtime_error error("Unsupported timing extension: HDU 0 (primary HDU) of input file \"" + evfile_geo + "\"");
      app_tester.writeException(ofs, error);
      ofs.close();

      out_file.erase();
      out_file_ref.erase();
      ignore_exception = true;

    } else if ("par6" == test_name) {
      // Test refusal of geocentric corrections on a geocentered file.
      pars["evfile"] = evfile_geo;
      pars["scfile"] = scfile_0540;
      pars["outfile"] = out_file;
      pars["ra"] = ra_0540;
      pars["dec"] = dec_0540;
      pars["tcorrect"] = "GEO";

      remove(log_file_ref.c_str());
      std::ofstream ofs(log_file_ref.c_str());
      std::runtime_error error("Unsupported timing extension: HDU 0 (primary HDU) of input file \"" + evfile_geo + "\"");
      app_tester.writeException(ofs, error);
      ofs.close();

      out_file.erase();
      out_file_ref.erase();
      ignore_exception = true;

    } else {
      // Skip this iteration.
      continue;
    }

    // Remove output FITS file.
    remove(out_file.c_str());

    // Test the application.
    app_tester.test(pars, log_file, log_file_ref, out_file, out_file_ref, ignore_exception);
  }
}

StAppFactory<TimeSystemTestApp> g_factory("test_timeSystem");
