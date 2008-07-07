/** \file test_timeSystem.h
    \brief Unit test for timeSystem package.
    \author Masa Hirayama, James Peachey
*/

#include "facilities/commonUtilities.h"

#include "st_app/StApp.h"
#include "st_app/StAppFactory.h"

#include "st_stream/st_stream.h"
#include "st_stream/StreamFormatter.h"

#include "timeSystem/AbsoluteTime.h"
#include "timeSystem/BaryTimeComputer.h"
#include "timeSystem/CalendarFormat.h"
#include "timeSystem/Duration.h"
#include "timeSystem/ElapsedTime.h"
#include "timeSystem/EventTimeHandler.h"
#include "timeSystem/GlastTimeHandler.h"
#include "timeSystem/MjdFormat.h"
#include "timeSystem/TimeInterval.h"
#include "timeSystem/TimeFormat.h"
#include "timeSystem/TimeSystem.h"

#include <cmath>
#include <exception>
#include <iomanip>
#include <limits>
#include <list>
#include <stdexcept>

namespace {

  bool s_failed = false;
  st_stream::StreamFormatter s_os("test_timeSystem", "", 2);

  void TestDuration();

  void TestTimeSystem();

  void TestAbsoluteTime();

  void TestElapsedTime();

  void TestTimeInterval();

  void TestTimeFormat();

  void TestBaryTimeComputer();

  void TestEventTimeHandlerFactory();

  void TestGlastTimeHandler();
}

using namespace st_app;
using namespace st_stream;
using namespace timeSystem;

class TestTimeSystemApp : public StApp {
  public:
    virtual void run();
};

void TestTimeSystemApp::run() {
  // Initialize output streams.
  InitStdStreams("test_timeSystem", 2, true);

  // Set precision high enough to show numbers in error messages accurately.
  s_os.err().precision(std::numeric_limits<double>::digits10);

  // Test Duration class.
  TestDuration();

  // Test TimeSystem class and subclasses.
  TestTimeSystem();

  // Test AbsoluteTime class.
  TestAbsoluteTime();

  // Test ElapsedTime class.
  TestElapsedTime();

  // Test TimeInterval class.
  TestTimeInterval();

  // Test TimeFormat class.
  TestTimeFormat();

  // Test BaryTimeComputer class.
  TestBaryTimeComputer();

  // Test EventTimeHandlerFactory class.
  TestEventTimeHandlerFactory();

  // Test GlastTimeHandler class.
  TestGlastTimeHandler();

  // Interpret failure flag to report error.
  if (s_failed) throw std::runtime_error("Unit test failure");
}

namespace {

  st_stream::OStream & err() {
    s_failed = true;
    return s_os.err() << prefix;
  }

  void TestDurationGetter(long day, double sec, const std::string & time_unit_name, long int_part, double frac_part, double tolerance) {
    // Test the getter that takes a long variable, a double variable, and a time unit name.
    long result_int = 0;
    double result_frac = 0.;
    Duration(day, sec).get(time_unit_name, result_int, result_frac);
    if (!(int_part == result_int && std::fabs(frac_part - result_frac) < tolerance)) {
      err() << "Duration(" << day << ", " << sec << ").get(int_part, frac_part, " << time_unit_name <<
        ") returned (int_part, frac_part) = (" << result_int << ", " << result_frac << "), not (" << int_part << ", " <<
        frac_part << ") as expected." << std::endl;
    }

    // Test the getter that takes a double variable and a time unit name.
    double result_double = 0.;
    Duration(day, sec).get(time_unit_name, result_double);
    double expected_double = int_part + frac_part;
    if (std::fabs(expected_double - result_double) > tolerance) {
      err() << "Duration(" << day << ", " << sec << ").get(result_double, " << time_unit_name << ") returned result_double = " <<
        result_double << ", not " << expected_double << " as expected." << std::endl;
    }

    // Test the getter that takes a time unit name only.
    result_double = Duration(day, sec).get(time_unit_name);
    expected_double = int_part + frac_part;
    if (std::fabs(expected_double - result_double) > tolerance) {
      err() << "Duration(" << day << ", " << sec << ").get(" << time_unit_name << ") returned " <<
        result_double << ", not " << expected_double << " as expected." << std::endl;
    }
  }

  void TestDurationConstructor(const std::string & time_unit_name, long int_part, double frac_part, const Duration & expected_result,
    const Duration & tolerance_high, const Duration & tolerance_low) {
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

  void TestOneComparison(const std::string & comparator, const Duration & dur1, const Duration & dur2,
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

  void TestOneComputation(const std::string & computation, const Duration & dur1, const Duration & dur2,
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

  void TestDuration() {
    s_os.setMethod("TestDuration");
    double epsilon = std::numeric_limits<double>::epsilon();

    // For tests of Duration getters for duration of +6 days.
    TestDurationGetter(6, 0., "Day",  6,         0., epsilon);
    TestDurationGetter(6, 0., "Hour", 6 * 24,    0., epsilon);
    TestDurationGetter(6, 0., "Min",  6 * 1440,  0., epsilon);
    TestDurationGetter(6, 0., "Sec",  6 * 86400, 0., epsilon);

    // For tests of Duration::getters for duration of +6 seconds.
    TestDurationGetter(0, 6., "Day",  0, 6. / 86400., epsilon);
    TestDurationGetter(0, 6., "Hour", 0, 6. / 3600.,  epsilon);
    TestDurationGetter(0, 6., "Min",  0, 6. / 60.,    epsilon);
    TestDurationGetter(0, 6., "Sec",  6, 0.,          epsilon);

    // For tests of Duration getters for duration of -6 days.
    TestDurationGetter(-6, 0., "Day",  -6,         0., epsilon);
    TestDurationGetter(-6, 0., "Hour", -6 * 24,    0., epsilon);
    TestDurationGetter(-6, 0., "Min",  -6 * 1440,  0., epsilon);
    TestDurationGetter(-6, 0., "Sec",  -6 * 86400, 0., epsilon);

    // For tests of Duration::getters for duration of -6 seconds.
    TestDurationGetter(0, -6., "Day",   0, -6. / 86400., epsilon);
    TestDurationGetter(0, -6., "Hour",  0, -6. / 3600.,  epsilon);
    TestDurationGetter(0, -6., "Min",   0, -6. / 60.,    epsilon);
    TestDurationGetter(0, -6., "Sec",  -6,  0.,          epsilon);

    // Tests of constructors.
    long int_part = 3456789;
    double frac_part = .56789567895678956789;
    Duration tol_high(0, 1.e-9); // 1 nano-second.
    Duration tol_low(0, 1.e-3); // 1 milli-second.
    TestDurationConstructor("Day", int_part, frac_part, Duration(int_part, frac_part*86400.), tol_high, tol_low);
    TestDurationConstructor("Hour", int_part, frac_part, Duration(int_part/24, (int_part%24 + frac_part)*3600.), tol_high, tol_low);
    TestDurationConstructor("Min", int_part, frac_part, Duration(int_part/1440, (int_part%1440 + frac_part)*60.), tol_high, tol_low);
    TestDurationConstructor("Sec", int_part, frac_part, Duration(int_part/86400, int_part%86400 + frac_part), tol_high, tol_low);

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
      Duration dur(+1, -0.1, "Day");
      err() << "Duration constructor did not throw an exception for Duration(+1, -0.1, \"Day\")" << std::endl;
    } catch (const std::exception & x) {
    }
    try {
      Duration dur(+1, +1.0, "Day");
      err() << "Duration constructor did not throw an exception for Duration(+1, +1.0, \"Day\")" << std::endl;
    } catch (const std::exception & x) {
    }
    try {
      Duration dur(-1, +0.1, "Day");
      err() << "Duration constructor did not throw an exception for Duration(-1, +0.1, \"Day\")" << std::endl;
    } catch (const std::exception & x) {
    }
    try {
      Duration dur(-1, -1.0, "Day");
      err() << "Duration constructor did not throw an exception for Duration(-1, -1.0, \"Day\")" << std::endl;
    } catch (const std::exception & x) {
    }
    try {
      Duration dur(0, +1.0, "Day");
      err() << "Duration constructor did not throw an exception for Duration(0, +1.0, \"Day\")" << std::endl;
    } catch (const std::exception & x) {
    }
    try {
      Duration dur(0, -1.0, "Day");
      err() << "Duration constructor did not throw an exception for Duration(0, -1.0, \"Day\")" << std::endl;
    } catch (const std::exception & x) {
    }

    // Test for detections of overflow/underflow: the basic constructor that takes days and seconds.
    double large_day = std::numeric_limits<long>::max() + .4;
    double small_day = std::numeric_limits<long>::min() + 1 - .4;
    // Note: Duration keeps its day part as one less than the integer part of it for computational advantages.
    double overflow_day = std::numeric_limits<long>::max() + 1.1;
    double underflow_day = std::numeric_limits<long>::min() + 1 - 1.1;
    // Note: Duration keeps its day part as one less than the integer part of it for computational advantages.

    // --- Case which should *not* overflow, but is close to overflowing.
    double sec = large_day * SecPerDay();
    try {
      Duration dur(0, sec);
    } catch (const std::exception & x) {
      err() << "Duration constructor threw an exception for " << sec << " seconds unexpectedly: " << x.what() << std::endl;
    }
    // --- Case which should *not* underflow, but is close to underflowing.
    sec = small_day * SecPerDay();
    try {
      Duration dur(0, sec);
    } catch (const std::exception & x) {
      err() << "Duration constructor threw an exception for " << sec << " seconds unexpectedly: " << x.what() << std::endl;
    }
    // --- Case which should overflow.
    sec = overflow_day * SecPerDay();
    try {
      Duration dur(0, sec);
      err() << "Duration constructor did not throw an exception for " << sec << " seconds." << std::endl;
    } catch (const std::exception & x) {
    }
    // --- Case which should underflow.
    sec = underflow_day * SecPerDay();
    try {
      Duration dur(0, sec);
      err() << "Duration constructor did not throw an exception for " << sec << " seconds." << std::endl;
    } catch (const std::exception & x) {
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
        Duration dur(target_time, time_unit_name);
      } catch (const std::exception & x) {
        err() << "Duration constructor threw an exception for Duration(" << target_time << ", \"" << time_unit_name <<
          "\") unexpectedly: " << x.what() << std::endl;
      }

      // --- Case which should *not* underflow, but is close to underflowing.
      target_time = small_day * conversion_factor;
      try {
        Duration dur(target_time, time_unit_name);
      } catch (const std::exception & x) {
        err() << "Duration constructor threw an exception for Duration(" << target_time << ", \"" << time_unit_name <<
          "\") unexpectedly: " << x.what() << std::endl;
      }

      // --- Case which should overflow.
      target_time = overflow_day * conversion_factor;
      try {
        Duration dur(target_time, time_unit_name);
        err() << "Duration constructor did not throw an exception for Duration(" << target_time << ", \"" << time_unit_name <<
          "\")." << std::endl;
      } catch (const std::exception & x) {
      }

      // --- Case which should underflow.
      target_time = underflow_day * conversion_factor;
      try {
        Duration dur(target_time, time_unit_name);
        err() << "Duration constructor did not throw an exception for Duration(" << target_time << ", \"" << time_unit_name <<
          "\")." << std::endl;
      } catch (const std::exception & x) {
      }
    }

    // Test for detections of overflow/underflow: the getters that computes a pair of integer and fractional parts.
    double large_number = std::numeric_limits<long>::max() + .4;
    double small_number = std::numeric_limits<long>::min() - .4;
    double overflow_number = std::numeric_limits<long>::max() + 1.1;
    double underflow_number = std::numeric_limits<long>::min() - 1.1;

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
      } catch (const std::exception & x) {
      }

      // --- Case which should underflow.
      dur = Duration(0, underflow_number * conversion_factor);
      try {
        dur.get(time_unit_string, int_part, frac_part);
        err() << "Duration getter that computes an integer-fraction pair did not throw an exception for a time duration of " <<
          underflow_number << " " << time_unit_string << "." << std::endl;
      } catch (const std::exception & x) {
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
    try {
      large_dur + Duration(1, 0.);
      err() << "Adding Duration(1, 0.) to a time duration of " << large_day << " days did not throw an exception." << std::endl;
    } catch (const std::exception & x) {
    }
    try {
      small_dur - Duration(1, 0.);
      err() << "Subtracting Duration(1, 0.) from a time duration of " << small_day << " days did not throw an exception." << std::endl;
    } catch (const std::exception & x) {
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
      TestOneComparison("!=", itor->first, dur0, (itor->second != 0));
      TestOneComparison("==", itor->first, dur0, (itor->second == 0));
      TestOneComparison("<",  itor->first, dur0, (itor->second <  0));
      TestOneComparison("<=", itor->first, dur0, (itor->second <= 0));
      TestOneComparison(">",  itor->first, dur0, (itor->second >  0));
      TestOneComparison(">=", itor->first, dur0, (itor->second >= 0));
    }

    // Test computation operators: +, +=, -, -=, and unary .-
    Duration dur1(321, 654.321);
    Duration dur2(123, 123.456);
    Duration tolerance(0, 1.e-9); // 1 nanosecond.
    TestOneComputation("+",  dur1, dur2, Duration( 444,   777.777), tolerance);
    TestOneComputation("+=", dur1, dur2, Duration( 444,   777.777), tolerance);
    TestOneComputation("-",  dur1, dur2, Duration( 198,   530.865), tolerance);
    TestOneComputation("-=", dur1, dur2, Duration( 198,   530.865), tolerance);
    TestOneComputation("u-", dur1, dur2, Duration(-322, 85745.679), tolerance);

    // Test proper handling of small difference in second part when two Duration's are added.
    epsilon = std::numeric_limits<double>::epsilon() * 10.;
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
    TestOneComputation("+",  max, zero, max, tolerance);
    TestOneComputation("+",  zero, max, max, tolerance);
    TestOneComputation("+",  min, zero, min, tolerance);
    TestOneComputation("+",  zero, min, min, tolerance);
    TestOneComputation("-",  max, zero, max, tolerance);
    TestOneComputation("-",  min, zero, min, tolerance);

    // Test addition of two numbers adding up to max.
    TestOneComputation("+",  max_minus_one, one, max, tolerance);
    TestOneComputation("+",  one, max_minus_one, max, tolerance);
    TestOneComputation("+",  min, one, min_plus_one, tolerance);
    TestOneComputation("+",  one, min, min_plus_one, tolerance);
    TestOneComputation("-",  max, one, max_minus_one, tolerance);
    TestOneComputation("-",  min_plus_one, one, min, tolerance);

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

  void TestOneConversion(const std::string & src_system_name, const moment_type & src_moment,
    const std::string & dest_system_name, const moment_type & expected_moment, double tolerance = 1.e-9) {
    s_os.setMethod("TestOneConversion");
    const TimeSystem & src_sys(TimeSystem::getSystem(src_system_name));
    const TimeSystem & dest_sys(TimeSystem::getSystem(dest_system_name));
    moment_type dest_moment = dest_sys.convertFrom(src_sys, src_moment);
    Duration tol_dur(0, tolerance);
    if (dest_moment.first != expected_moment.first || !expected_moment.second.equivalentTo(dest_moment.second, tol_dur)) {
      err() << "Converting from " << src_sys << " to " << dest_sys << ", moment_type(" << src_moment.first << ", " <<
        src_moment.second << ") was converted to moment_type(" << dest_moment.first << ", " << dest_moment.second <<
        "), not equivalent to moment_type(" << expected_moment.first << ", " << expected_moment.second << ") with tolerance of " <<
        tol_dur << "." << std::endl;
    }
  }

  void TestOneSubtraction(const moment_type & moment1, const moment_type & moment2, double difference, double difference_utc) {
    std::map<std::string, Duration> expected_diff;
    expected_diff["TAI"] = Duration(0, difference);
    expected_diff["TDB"] = Duration(0, difference);
    expected_diff["TT"]  = Duration(0, difference);
    expected_diff["UTC"] = Duration(0, difference_utc);

    Duration tolerance(0, 1.e-9); // 1 nanosecond.

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

  void TestOneDateTimeComputation(const moment_type & moment, const datetime_type & expected_datetime,
      const datetime_type & expected_datetime_utc) {
    std::list<std::string> time_system_name_list;
    time_system_name_list.push_back("TAI");
    time_system_name_list.push_back("TDB");
    time_system_name_list.push_back("TT");
    time_system_name_list.push_back("UTC");
    double tolerance = 1.e-9;

    for (std::list<std::string>::const_iterator itor_sys = time_system_name_list.begin(); itor_sys != time_system_name_list.end(); ++itor_sys) {
      std::string time_system_name = *itor_sys;
      const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
      datetime_type expected(expected_datetime);
      if ("UTC" == time_system_name) expected = expected_datetime_utc;
      datetime_type result = time_system.computeDateTime(moment);

      if (result.first != expected.first || std::fabs(result.second - expected.second) > tolerance) {
        err() << "computeDateTime of " << time_system_name << " returned datetime_type(" << result.first <<
          ", " << result.second << ") for moment_type(" << moment.first << ", " << moment.second <<
          "), not equivalent to the expected result of moment_type(" << expected.first << ", " << expected.second <<
          "), with tolerance of " << tolerance << " seconds." << std::endl;
      }
    }
  }

  void TestTimeSystem() {
    s_os.setMethod("TestTimeSystem");
    using namespace facilities;

    // Set the default leap second file to a local copy of an actual leap second table.
    std::string test_leap = commonUtilities::joinPath(commonUtilities::getDataPath("timeSystem"), "testls.fits");
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

    moment_type tai_ref_moment(moment_type(ref_day, Duration(0, tai_ref_sec)));
    moment_type tt_ref_moment(moment_type(ref_day, Duration(0, tt_ref_sec)));
    moment_type tdb_ref_moment(moment_type(ref_day, Duration(0, tdb_ref_sec)));
    moment_type utc_ref_moment(moment_type(ref_day, Duration(0, utc_ref_sec)));

    double tdb_tolerance = 1.e-7; // 100 ns is the accuracy of algorithms involving TDB.

    // Test conversions, reflexive cases.
    TestOneConversion("TAI", tai_ref_moment, "TAI", tai_ref_moment);
    TestOneConversion("TDB", tdb_ref_moment, "TDB", tdb_ref_moment);
    TestOneConversion("TT",  tt_ref_moment,  "TT",  tt_ref_moment);
    TestOneConversion("UTC", utc_ref_moment, "UTC", utc_ref_moment);

    // Test conversions from TAI to...
    TestOneConversion("TAI", tai_ref_moment, "TDB", tdb_ref_moment, tdb_tolerance);
    TestOneConversion("TAI", tai_ref_moment, "TT",  tt_ref_moment);
    TestOneConversion("TAI", tai_ref_moment, "UTC", utc_ref_moment);

    // Test conversions from TDB to...
    TestOneConversion("TDB", tdb_ref_moment, "TAI", tai_ref_moment, tdb_tolerance);
    TestOneConversion("TDB", tdb_ref_moment, "TT",  tt_ref_moment, tdb_tolerance);
    TestOneConversion("TDB", tdb_ref_moment, "UTC", utc_ref_moment, tdb_tolerance);

    // Test conversions from TT to...
    TestOneConversion("TT",  tt_ref_moment, "TAI", tai_ref_moment);
    TestOneConversion("TT",  tt_ref_moment, "TDB", tdb_ref_moment, tdb_tolerance);
    TestOneConversion("TT",  tt_ref_moment, "UTC", utc_ref_moment);

    // Test conversions from UTC to...
    TestOneConversion("UTC", utc_ref_moment, "TAI", tai_ref_moment);
    TestOneConversion("UTC", utc_ref_moment, "TDB", tdb_ref_moment, tdb_tolerance);
    TestOneConversion("UTC", utc_ref_moment, "TT",  tt_ref_moment);

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
    TestOneConversion("UTC", moment_type(leap1, Duration(0, 0.)),
                      "TAI", moment_type(leap1, Duration(0, diff1)));
    // --- Slightly before a leap second is inserted.
    TestOneConversion("UTC", moment_type(leap1 - 1, Duration(0, SecPerDay() - .001)),
                      "TAI", moment_type(leap1 - 1, Duration(0, SecPerDay() - .001 + diff0)));
    // --- Same as above, but with a large elapsed time.
    //     Although the total time (origin + elapsed) is large enough to cross two leap second boundaries, still
    //     the earliest leap second should be used because the choice of leap second is based only on the origin time.
    TestOneConversion("UTC", moment_type(leap1 - 1, Duration(delta_leap + 1, -.001 + 2.002)),
                      "TAI", moment_type(leap1 - 1, Duration(delta_leap + 1, -.001 + 2.002 + diff0)));

    // To ensure TAI->UTC is handled correctly, do some tougher conversions, i.e. times which are close to
    // times when leap seconds are inserted.
    // --- At the end of a leap second.
    TestOneConversion("TAI", moment_type(leap1, Duration(0, diff1)),
                      "UTC", moment_type(leap1, Duration(0, 0.)));
    // --- During a leap second.
    TestOneConversion("TAI", moment_type(leap1, Duration(0, -0.3 + diff1)),
                      "UTC", moment_type(leap1, Duration(0, -0.3)));
    // --- At the beginning of a leap second.
    TestOneConversion("TAI", moment_type(leap1, Duration(0, -1.0 + diff1)),
                      "UTC", moment_type(leap1, Duration(0, -1.0)));
    // --- After the end of a leap second.
    TestOneConversion("TAI", moment_type(leap1, Duration(0, +0.3 + diff1)),
                      "UTC", moment_type(leap1, Duration(0, +0.3)));
    // --- Before the beginning of a leap second.
    TestOneConversion("TAI", moment_type(leap1, Duration(0, -1.3 + diff1)),
                      "UTC", moment_type(leap1, Duration(0, -1.3)));

    // Test that conversion uses table keyed by TAI times, not by UTC.
    TestOneConversion("TAI", moment_type(leap1 - 1, Duration(0, SecPerDay() - 2.)),
                      "UTC", moment_type(leap1 - 1, Duration(0, SecPerDay() - 2. - diff0)));

    // Test case before first time covered by the current UTC definition. This is "undefined" in the current scheme.
    long diff_oldest = 10;
    try {
      TestOneConversion("UTC", moment_type(0, Duration(0, 0.)), "TAI", moment_type(0, Duration(0, diff_oldest)));
      err() << "Conversion of time 0. MJD UTC to TAI did not throw an exception." << std::endl;
    } catch (const std::exception &) {
      // That's OK!
    }
    try {
      TestOneConversion("TAI", moment_type(0, Duration(0, diff_oldest)), "UTC", moment_type(0, Duration(0, 0.)));
      err() << "Conversion of time 0. MJD TAI to UTC did not throw an exception." << std::endl;
    } catch (const std::exception &) {
      // That's OK!
    }

    // Test undefined UTC time, with the time origin covered by the current definition.
    long oldest_mjd = 41317;
    try {
      TestOneConversion("UTC", moment_type(oldest_mjd + 1, Duration(-2, 0.)),
                        "TAI", moment_type(oldest_mjd + 1, Duration(-2, diff_oldest)));
      err() << "Conversion of moment_type(" << oldest_mjd + 1 << ", " << Duration(-2, 0.) <<
        ") from UTC to TAI did not throw an exception." << std::endl;
    } catch (const std::exception &) {
      // That's OK!
    }
    try {
      TestOneConversion("TAI", moment_type(oldest_mjd + 1, Duration(-2, diff_oldest)),
                        "UTC", moment_type(oldest_mjd + 1, Duration(-2, 0.)));
      err() << "Conversion of moment_type(" << oldest_mjd + 1 << ", " << Duration(-2, diff_oldest) <<
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
    std::string bogus_leap = commonUtilities::joinPath(commonUtilities::getDataPath("timeSystem"), "bogusls.fits");

    // Test loading specific file for leap second table, by first setting the default file name.
    TimeSystem::setDefaultLeapSecFileName(bogus_leap);

    if (bogus_leap != TimeSystem::getDefaultLeapSecFileName()) 
      err() << "After setting default leap second file name, default leap second file name was " <<
        TimeSystem::getDefaultLeapSecFileName() << ", not " << bogus_leap << " as expected." << std::endl;

    TimeSystem::loadLeapSeconds();

    // Test case after last time covered by the current UTC definition.
    long leap_last = 53737;
    double diff_last = 31.;
    TestOneConversion("UTC", moment_type(leap_last, Duration(0, 100.)), "TAI", moment_type(leap_last, Duration(0, 100. + diff_last)));

    // Reset default leap second file name.
    TimeSystem::setDefaultLeapSecFileName("");

    // Finally, test loading the real leap seconds file.
    TimeSystem::loadLeapSeconds(bogus_leap);

    // Test case after last time covered by the current UTC definition.
    leap_last = 53737;
    diff_last = 31.;
    TestOneConversion("UTC", moment_type(leap_last, Duration(0, 100.)), "TAI", moment_type(leap_last, Duration(0, 100. + diff_last)));

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
    TestOneConversion("TAI", moment_type(leap1, Duration(0, diff1)),
                      "UTC", moment_type(leap1, Duration(0, 0.)));
    // --- Slightly before a leap second is removed.
    TestOneConversion("TAI", moment_type(leap1, Duration(0, -.001 + diff1)),
                      "UTC", moment_type(leap1, Duration(0, -.001)));
    // --- Same as above, but with a large elapsed time.
    //     Although the total time (origin + elapsed) is large enough to cross two leap second boundaries, still
    //     the earliest leap second should be used because the choice of leap second is based only on the origin time.
    TestOneConversion("TAI", moment_type(leap1, Duration(delta_leap, -.001 + .002)),
                      "UTC", moment_type(leap1, Duration(delta_leap, -.001 + .002 - diff1)));

    // To ensure UTC->TAI is handled correctly, do some tougher conversions, i.e. times which are close to
    // times when leap seconds are removed.
    // --- At the end of a leap second.
    TestOneConversion("UTC", moment_type(leap1, Duration(0, 0.)),
                      "TAI", moment_type(leap1, Duration(0, diff1)));
    // --- During a leap second
    // Note: As of May 29th, 2008, the design doesn't allow/need this test due to the improved robustness.
    // --- At the beginning of a leap second.
    TestOneConversion("UTC", moment_type(leap1 - 1, Duration(0, SecPerDay() - 1.0)),
                      "TAI", moment_type(leap1 - 1, Duration(0, SecPerDay() - 1.0 + diff0)));
    // --- After the end of a leap second.
    TestOneConversion("UTC", moment_type(leap1, Duration(0, 0.3)),
                      "TAI", moment_type(leap1, Duration(0, diff1 + 0.3)));
    // --- Before the beginning of a leap second.
    TestOneConversion("UTC", moment_type(leap1 - 1, Duration(0, SecPerDay() - 1.3)),
                      "TAI", moment_type(leap1 - 1, Duration(0, SecPerDay() - 1.3 + diff0)));

    // Test computeTimeDifference method.
    // Middle of nowhere
    TestOneSubtraction(moment_type(51910, Duration(0, 120.)), moment_type(51910, Duration(0, 100.)),  20., 20.);
    // Leap second insertion
    TestOneSubtraction(moment_type(leap2, Duration(0, +10.)), moment_type(leap2 - 1, Duration(0, SecPerDay() - 10.)), 20., 21.);
    // leap second removal
    TestOneSubtraction(moment_type(leap1, Duration(0, +10.)), moment_type(leap1 - 1, Duration(0, SecPerDay() - 10.)), 20., 19.);
    // Non-existing time in UTC
    // Note: As of May 29th, 2008, the design doesn't allow/need this test due to the improved robustness.

    // Test computeDateTime method.
    double deltat = 20.;
    // Middle of nowhere
    TestOneDateTimeComputation(moment_type(51910, Duration(0, 100.)), datetime_type(51910, 100.), datetime_type(51910, 100.));
    // Middle of nowhere, with a negative elapsed time
    TestOneDateTimeComputation(moment_type(51910, Duration(0, -100.)),
       datetime_type(51909, SecPerDay() - 100.), datetime_type(51909, SecPerDay() - 100.));
    // Across leap second insertion in UTC
    TestOneDateTimeComputation(moment_type(leap2 - 1, Duration(1, deltat - 10.)),
      datetime_type(leap2, deltat - 10.), datetime_type(leap2, deltat - 10. - 1.));
    // Across leap second removal in UTC
    TestOneDateTimeComputation(moment_type(leap1 - 1, Duration(1, deltat - 10.)),
      datetime_type(leap1, deltat - 10.), datetime_type(leap1, deltat - 10. + 1.));
    // Non-existing time in UTC
    // Note: As of May 29th, 2008, the design doesn't allow/need this test due to the improved robustness.

    // Tests at times close to times when leap seconds are inserted (in UTC).
    // Note: As of May 29th, 2008, the design isn't sensitive to those tests due to the improved robustness.
    // --- Before the beginning of a leap second.
    TestOneDateTimeComputation(moment_type(leap2 - 1, Duration(1, deltat - 0.3)),
      datetime_type(leap2, deltat - 0.3), datetime_type(leap2, deltat - 1.3));
    // --- At the beginning of a leap second.
    TestOneDateTimeComputation(moment_type(leap2 - 1, Duration(1, deltat)),
      datetime_type(leap2, deltat), datetime_type(leap2, deltat - 1.0));
    // --- During a leap second.
    TestOneDateTimeComputation(moment_type(leap2 - 1, Duration(1, deltat + 0.3)),
      datetime_type(leap2, deltat + 0.3), datetime_type(leap2, deltat - 0.7));
    // --- At the end of a leap second.
    TestOneDateTimeComputation(moment_type(leap2 - 1, Duration(1, deltat + 1.0)),
      datetime_type(leap2, deltat + 1.0), datetime_type(leap2, deltat));
    // --- After the end of a leap second.
    TestOneDateTimeComputation(moment_type(leap2 - 1, Duration(1, deltat + 1.3)),
      datetime_type(leap2, deltat + 1.3), datetime_type(leap2, deltat + 0.3));

    // Tests at times close to times when leap seconds are removed (in UTC).
    // Note: As of May 29th, 2008, the design isn't sensitive to those tests due to the improved robustness.
    // --- Before the beginning of a leap second.
    TestOneDateTimeComputation(moment_type(leap1 - 1, Duration(1, deltat - 1.3)),
      datetime_type(leap1, deltat - 1.3), datetime_type(leap1, deltat - 0.3));
    // --- At the beginning of a leap second.
    TestOneDateTimeComputation(moment_type(leap1 - 1, Duration(1, deltat - 1.0)),
      datetime_type(leap1, deltat - 1.0), datetime_type(leap1, deltat));
    // --- During a leap second.
    // Note: As of May 29th, 2008, the design doesn't allow/need this test due to the improved robustness.
    // --- At the end of a leap second.
    // Note: As of May 29th, 2008, the design doesn't allow/need this test due to the improved robustness.
    // --- After the end of a leap second.
    TestOneDateTimeComputation(moment_type(leap1 - 1, Duration(1, deltat - 0.7)),
      datetime_type(leap1, deltat - 0.7), datetime_type(leap1, deltat + 0.3));

    // Note: The following test became unnecessary as of May 29th, 2008 due to the improved robustness of the design.
#if 0
    // Test proper handling of origin of UTC Moment. It should be adjusted to an existing MJD time in UTC and
    // special caution is needed for cases of leap second removal. In the test below, note that MJD Duration(leap1, -.7)
    // does NOT exist in UTC system because it expresses the "removed" second, which requires some adjustment for UTC origin.
    Duration mjd_tai(leap1, -.7);
    Duration mjd_utc(leap1, -diff0 -.7);
    TestOneConversion("UTC", mjd_utc, Duration(0, 0.), "TAI", mjd_tai, Duration(0, 0.)); // easy test.
    TestOneConversion("TAI", mjd_tai, Duration(0, 0.), "UTC", mjd_utc, Duration(0, 0.)); // tougher test, need handle with care.

    // Another tricky test.  MJD Duration(leap1, 0.) DOES exist in UTC system, too, but it is immediately after the leap
    // second removal. So, below needs a different kind of careful handling of UTC origin than above.
    mjd_tai = Duration(leap1, 0.);
    mjd_utc = Duration(leap1, -diff0);
    TestOneConversion("UTC", mjd_utc, Duration(0, 0.), "TAI", mjd_tai, Duration(0, 0.)); // easy test.
    TestOneConversion("TAI", mjd_tai, Duration(0, 0.), "UTC", mjd_utc, Duration(0, 0.)); // tougher test, need handle with care.
#endif

    // Test of conversion condition of TAI-to-UTC conversion.
    // Below must produce identical result for test input of 100. seconds after 51910.0 MJD (TAI).
    long tai_day = 0;
    double tai_sec = 100. + 51910. * SecPerDay();
    try {
      TestOneConversion("TAI", moment_type(tai_day, Duration(0, tai_sec)),
                        "UTC", moment_type(oldest_mjd, Duration(51910 - oldest_mjd, 100. - diff_oldest)));
    } catch (const std::exception &) {
      err() << "Conversion of TAI to UTC for moment_type(" << tai_day << ", " << tai_sec << ") threw an exception." << std::endl;
    }

    // Test of conversion condition of TAI-to-UTC conversion.
    // Below must throw an exception because resulting UTC cannot be expressed properly.
    tai_day = 51910;
    tai_sec = 100. - 51910. * SecPerDay();
    try {
      TestOneConversion("TAI", moment_type(tai_day, Duration(0, tai_sec)),
                        "UTC", moment_type(0, Duration(0, 100. - diff2)));
      err() << "Conversion of TAI to UTC for moment_type(" << tai_day << ", " << tai_sec << ") did not throw an exception." << std::endl;
    } catch (const std::exception &) {
      // That's OK!
    }

    // Test of conversion condition of UTC-to-TAI conversion.
    // Below must throw an exception because originating UTC cannot be interpreted properly.
    long utc_day = 0;
    double utc_sec = 100. + 51910. * SecPerDay() - diff2;
    try {
      TestOneConversion("UTC", moment_type(utc_day, Duration(0, utc_sec)),
                        "TAI", moment_type(51910, Duration(0, 100.)));
      err() << "Conversion of UTC to TAI for moment_type(" << utc_day << ", " << utc_sec << ") did not throw an exception." << std::endl;
    } catch (const std::exception &) {
      // That's OK!
    }

    // Test of conversion condition of UTC-to-TAI conversion.
    // Below must throw an exception because originating UTC time cannot be interpreted properly.
    utc_day = 51910;
    utc_sec = 100. - 51910. * SecPerDay() - diff2;
    try {
      TestOneConversion("UTC", moment_type(utc_day, Duration(0, utc_sec)),
                        "TAI", moment_type(utc_day, Duration(-utc_day, 100.)));
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
      " MJD and " << utc_mjd2.getValue(Day) << " MJD as " << result << ", not " << expected_result << " as expected." <<
       std::endl;
    }
#endif

    // Test of origin for UTC time that must be after MJD 41317.0 (January 1st, 1972) by definition.
    moment_type tai_moment(oldest_mjd - 1, Duration(1, 1. + diff_oldest));
    moment_type utc_moment = TimeSystem::getSystem("UTC").convertFrom(TimeSystem::getSystem("TAI"), tai_moment);
    if (oldest_mjd > utc_moment.first) {
      err() << "Conversion from TAI to UTC for input Moment(" << tai_moment.first << ", " << tai_moment.second <<
      ") returned Moment(" << utc_moment.first << ", " << utc_moment.second <<
	"), which is earlier than the beginning of the current UTC definition " << oldest_mjd << " MJD." << std::endl;
    }
  }

  static void CompareAbsoluteTime(const AbsoluteTime & abs_time, const AbsoluteTime & later_time) {
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

  void TestAbsoluteTime() {
    s_os.setMethod("TestAbsoluteTime");

    // Use the bogus leap second table for this unit test.
    using namespace facilities;
    std::string bogus_leap = commonUtilities::joinPath(commonUtilities::getDataPath("timeSystem"), "bogusls.fits");
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
    double double_tol = 100.e-9 / SecPerDay(); // 100 nano-seconds in days.
    if (expected_mjd.m_int != result_mjd.m_int || std::fabs(expected_mjd.m_frac - result_mjd.m_frac) > double_tol) {
      err() << "After abs_time = AbsoluteTime(\"TT\", " << mjd_day << ", " << mjd_sec <<
        "), abs_time.get(\"TT\", result_mjd) gave result_mjd = (" << result_mjd.m_int << ", " << result_mjd.m_frac <<
        "), not (" << expected_mjd.m_int << ", " << expected_mjd.m_frac << ") as expected." << std::endl;
    }

    // Test the getter for low-precision MJD.
    abs_time = AbsoluteTime("TT", mjd_day, mjd_sec);
    Mjd1 result_mjd1(0.);
    abs_time.get("TT", result_mjd1);
    double_tol = 10.e-6 / SecPerDay(); // 10 micro-seconds in days.
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
    double_tol = 100.e-9 / SecPerDay(); // 100 nano-seconds in days.
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
    double_tol = 100.e-9 / SecPerDay(); // 100 nano-seconds in days.
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
    double_tol = 100.e-9 / SecPerDay(); // 100 nano-seconds in days.
    if (expected_mjd.m_int != result_mjd.m_int || std::fabs(expected_mjd.m_frac - result_mjd.m_frac) > double_tol) {
      err() << "After abs_time = AbsoluteTime(\"TT\", Mjd(" << mjd_day << ", " << mjd_sec / SecPerDay() <<
        "))), abs_time.get(\"TT\", result_mjd) gave result_mjd = (" << result_mjd.m_int << ", " << result_mjd.m_frac <<
        "), not (" << expected_mjd.m_int << ", " << expected_mjd.m_frac << ") as expected." << std::endl;
    }

    // Test the constructor taking a low-precision MJD.
    abs_time = AbsoluteTime("TT", Mjd1(mjd_day + mjd_sec / SecPerDay()));
    result_mjd1 = Mjd1(0.);
    abs_time.get("TT", result_mjd);
    double_tol = 10.e-6 / SecPerDay(); // 10 micro-seconds in days.
    if (expected_mjd.m_int != result_mjd.m_int || std::fabs(expected_mjd.m_frac - result_mjd.m_frac) > double_tol) {
      err() << "After abs_time = AbsoluteTime(\"TT\", Mjd1(" << mjd_day + mjd_sec / SecPerDay() <<
        "))), abs_time.get(\"TT\", result_mjd) gave result_mjd = (" << result_mjd.m_int << ", " << result_mjd.m_frac <<
        "), not (" << expected_mjd.m_int << ", " << expected_mjd.m_frac << ") as expected." << std::endl;
    }

    // Test the setter taking a high-precision MJD.
    abs_time.set("TT", Mjd(mjd_day, mjd_sec / SecPerDay()));
    result_mjd = Mjd(0, 0.);
    abs_time.get("TT", result_mjd);
    double_tol = 100.e-9 / SecPerDay(); // 100 nano-seconds in days.
    if (expected_mjd.m_int != result_mjd.m_int || std::fabs(expected_mjd.m_frac - result_mjd.m_frac) > double_tol) {
      err() << "After abs_time.set(\"TT\", Mjd(" << mjd_day << ", " << mjd_sec / SecPerDay() <<
        "))), abs_time.get(\"TT\", result_mjd) gave result_mjd = (" << result_mjd.m_int << ", " << result_mjd.m_frac <<
        "), not (" << expected_mjd.m_int << ", " << expected_mjd.m_frac << ") as expected." << std::endl;
    }

    // Test the setter taking a low-precision MJD.
    abs_time.set("TT", Mjd1(mjd_day + mjd_sec / SecPerDay()));
    result_mjd1 = Mjd1(0.);
    abs_time.get("TT", result_mjd);
    double_tol = 10.e-6 / SecPerDay(); // 10 micro-seconds in days.
    if (expected_mjd.m_int != result_mjd.m_int || std::fabs(expected_mjd.m_frac - result_mjd.m_frac) > double_tol) {
      err() << "After abs_time.set(\"TT\", Mjd1(" << mjd_day + mjd_sec / SecPerDay() <<
        "))), abs_time.get(\"TT\", result_mjd) gave result_mjd = (" << result_mjd.m_int << ", " << result_mjd.m_frac <<
        "), not (" << expected_mjd.m_int << ", " << expected_mjd.m_frac << ") as expected." << std::endl;
    }

    // Test the setter taking an MJD string.
    std::string mjd_string("54321.142881944444444");
    abs_time.set("TT", MjdFmt, mjd_string);
    result_mjd = Mjd(0, 0.);
    abs_time.get("TT", result_mjd);
    double_tol = 100.e-9 / SecPerDay(); // 100 nano-seconds in days.
    if (expected_mjd.m_int != result_mjd.m_int || std::fabs(expected_mjd.m_frac - result_mjd.m_frac) > double_tol) {
      err() << "After abs_time.set(\"TT\", \"MJD\", \"" << mjd_string <<
        "\"), abs_time.get(\"TT\", result_mjd) gave result_mjd = (" << result_mjd.m_int << ", " << result_mjd.m_frac <<
        "), not (" << expected_mjd.m_int << ", " << expected_mjd.m_frac << ") as expected." << std::endl;
    }

    // Create an absolute time corresponding to MET 1000. s.
    mjd_day = 51910;
    mjd_sec = 1000.;
    abs_time = AbsoluteTime("TDB", mjd_day, mjd_sec);

    // Test printing the absolute time.
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

    // Test printing the absolute time, with a negative elapsed time.
    abs_time = AbsoluteTime("TDB", mjd_day, -mjd_sec);
    {
      std::ostringstream os;
      os << abs_time;
      result_string = os.str();
    }
    expected_string = "1000 seconds before 51910.0 MJD (TDB)";
    if (expected_string != result_string) {
      err() << "AbsoluteTime object wrote \"" << result_string << "\", not \"" << expected_string <<
      "\" as expected." << std::endl;
    }

    // Test printing the absolute time, with a zero elapsed time.
    abs_time = AbsoluteTime("TDB", mjd_day, 0.);
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

    // Test adding an elapsed time to this time.
    // Create an absolute time corresponding to 100. s MET TDB to verify adding an elapsed time.
    abs_time = AbsoluteTime("TDB", mjd_day, mjd_sec);
    double delta_t = 100.;
    ElapsedTime elapsed_time("TDB", Duration(0, delta_t));
    AbsoluteTime result = abs_time + elapsed_time;
    AbsoluteTime expected_result("TDB", mjd_day, mjd_sec + delta_t);
    ElapsedTime epsilon("TDB", Duration(0, 1.e-9)); // 1 nanosecond.
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
    CompareAbsoluteTime(abs_time, later_time);

    // Test comparison operators (>, >=, <, and <=) in UTC system.
    long mjd_leap = 51179;
    AbsoluteTime abs_time_utc("UTC", mjd_leap - 1, 86400.8);
    AbsoluteTime later_time_utc("UTC", mjd_leap, 0.2);
    CompareAbsoluteTime(abs_time_utc, later_time_utc);

    // Test equivalentTo.
    // Test situations where they are not equivalent.
    ElapsedTime tight_tol("TDB", Duration(0, 99.9999));
    if (abs_time.equivalentTo(later_time, tight_tol))
      err() << "After AbsoluteTime abs_time(TDB, 51910, 1000.), abs_time.equivalentTo returned true for \"" << later_time <<
        "\" with tolerance of " << tight_tol << ", not false as expected." << std::endl;
    if (later_time.equivalentTo(abs_time, tight_tol))
      err() << "After AbsoluteTime later_time(TDB, 51910, 1100.), later_time.equivalentTo returned true for \"" << abs_time <<
        "\" with tolerance of " << tight_tol << ", not false as expected." << std::endl;

    // Test situations where they are equivalent.
    ElapsedTime loose_tol("TDB", Duration(0, 100.));
    if (!(abs_time.equivalentTo(later_time, loose_tol)))
      err() << "After AbsoluteTime abs_time(TDB, 51910, 1000.), abs_time.equivalentTo returned false for \"" << later_time <<
        "\" with tolerance of " << loose_tol << ", not true as expected." << std::endl;
    if (!(later_time.equivalentTo(abs_time, loose_tol)))
      err() << "After AbsoluteTime later_time(TDB, 51910, 1100.), later_time.equivalentTo returned false for \"" << abs_time <<
        "\" with tolerance of " << loose_tol << ", not true as expected." << std::endl;

    // Test subtraction of absolute time from absolute time.
    Duration expected_diff(0, 100.);
    Duration tolerance(0, 1.e-9); // 1 ns.
    Duration difference = (later_time - abs_time).computeElapsedTime("TDB").getDuration();
    if (!expected_diff.equivalentTo(difference, tolerance))
      err() << "Absolute time [" << abs_time << "] subtracted from absolute time [" << later_time << "] gave " << difference <<
        ", not " << expected_diff << " as expected." << std::endl;

    // Test subtraction of absolute time from absolute time in UTC system
    expected_diff = Duration(0, .4);
    difference = (later_time_utc - abs_time_utc).computeElapsedTime("UTC").getDuration();
    if (!expected_diff.equivalentTo(difference, tolerance))
      err() << "Absolute time [" << abs_time_utc << "] subtracted from absolute time [" << later_time_utc << "] gave " << difference <<
        ", not " << expected_diff << " as expected." << std::endl;
  }

  void TestElapsedTime() {
    s_os.setMethod("TestElapsedTime");

    // Test of the getter that returns a Duration object.
    Duration original_dur(1, SecPerDay() * 0.125);
    Duration expected_dur = original_dur;
    Duration tolerance(0, 1.e-9); // 1 ns.
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

  void TestTimeInterval() {
    s_os.setMethod("TestTimeInterval");

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
    Duration expected_dur = Duration(0, 1000.123456789);
    Duration result_dur = interval_a.computeDuration("TDB");
    Duration tolerance_dur(0, 1.e-9); // 1 ns.
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

  void TestOneCalendarDate(long mjd, long calendar_year, long month, long month_day, long iso_year, long week_number,
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
  void TestOneBadDateTime(const TimeFormat<TimeRepType> & time_format, const datetime_type & datetime,
    const std::string & time_rep_name, const std::string & data_description, bool exception_expected = true) {
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
  void TestOneBadTimeRep(const TimeFormat<TimeRepType> & time_format, const TimeRepType & time_rep, const std::string & time_rep_name,
    const std::string & data_description, bool exception_expected = true) {
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
  void TestOneBadTimeString(const TimeFormat<TimeRepType> & time_format, const std::string & time_string,
    const std::string & time_rep_name, bool exception_expected = true) {
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

  void TestTimeFormat() {
    s_os.setMethod("TestTimeFormat");

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
    double tolerance = 100.e-9; // 100 nano-seconds.
    if (expected_datetime.first != datetime.first || tolerance < std::fabs(expected_datetime.second - datetime.second)) {
      err() << "TimeFormat<Mjd>::convert method converted (" << expected_mjd.m_int << " + " << expected_mjd.m_frac <<
        ") MJD into datetime_type pair (" << datetime.first << ", " << datetime.second << "), not (" <<
        expected_datetime.first << ", " << expected_datetime.second << ") as expected." << std::endl;
    }

    // Test conversion from an Mjd1 object (that holds a single MJD number of double type) to a datetime_type object.
    datetime = mjd_format.convert(expected_mjd);
    tolerance = 10.e-6; // 10 micro-seconds.
    if (expected_datetime.first != datetime.first || tolerance < std::fabs(expected_datetime.second - datetime.second)) {
      err() << "TimeFormat<Mjd>::convert method converted " << expected_mjd1.m_day << " MJD into datetime_type pair (" <<
        datetime.first << ", " << datetime.second << "), not (" << expected_datetime.first << ", " << expected_datetime.second <<
        ") as expected." << std::endl;
    }

    // Test conversion from an Jd object (that holds integer part and fractional part of JD) to a datetime_type object.
    datetime = jd_format.convert(expected_jd);
    tolerance = 100.e-9; // 100 nano-seconds.
    if (expected_datetime.first != datetime.first || tolerance < std::fabs(expected_datetime.second - datetime.second)) {
      err() << "TimeFormat<Jd>::convert method converted (" << expected_jd.m_int << " + " << expected_jd.m_frac <<
        ") JD into datetime_type pair (" << datetime.first << ", " << datetime.second << "), not (" <<
        expected_datetime.first << ", " << expected_datetime.second << ") as expected." << std::endl;
    }

    // Test conversion from an Jd1 object (that holds a single JD number of double type) to a datetime_type object.
    datetime = jd_format.convert(expected_jd);
    tolerance = 10.e-6; // 10 micro-seconds.
    if (expected_datetime.first != datetime.first || tolerance < std::fabs(expected_datetime.second - datetime.second)) {
      err() << "TimeFormat<Jd>::convert method converted " << expected_jd1.m_day << " JD into datetime_type pair (" <<
        datetime.first << ", " << datetime.second << "), not (" << expected_datetime.first << ", " << expected_datetime.second <<
        ") as expected." << std::endl;
    }

    // Test conversion from a datetime_type object to an Mjd object (that holds integer part and fractional part of MJD).
    Mjd result_mjd = mjd_format.convert(expected_datetime);
    tolerance = 100.e-9 / SecPerDay(); // 100 nano-seconds in units of day.
    if (expected_mjd.m_int != result_mjd.m_int || tolerance < std::fabs(expected_mjd.m_frac - result_mjd.m_frac)) {
      err() << "TimeFormat<Mjd>::convert method converted datetime_type pair (" << expected_datetime.first << ", " <<
        expected_datetime.second << ") into (" << result_mjd.m_int << " + " << result_mjd.m_frac << ") MJD, not (" <<
        expected_mjd.m_int << " + " << expected_mjd.m_frac << ") MJD as expected." << std::endl;
    }

    // Test conversion from a datetime_type object to an Mjd1 object (that holds a single MJD number of double type).
    Mjd1 result_mjd1 = mjd1_format.convert(expected_datetime);
    tolerance = 100.e-9 / SecPerDay(); // 100 nano-seconds in units of day.
    if (tolerance < std::fabs(expected_mjd1.m_day - result_mjd1.m_day)) {
      err() << "TimeFormat<Mjd1>::convert method converted datetime_type pair (" << expected_datetime.first << ", " <<
        expected_datetime.second << ") into " << result_mjd1.m_day << " MJD, not " << expected_mjd1.m_day << " MJD as expected." <<
        std::endl;
    }

    // Test conversion from a datetime_type object to an Jd object (that holds integer part and fractional part of JD).
    Jd result_jd = jd_format.convert(expected_datetime);
    tolerance = 100.e-9 / SecPerDay(); // 100 nano-seconds in units of day.
    if (expected_jd.m_int != result_jd.m_int || tolerance < std::fabs(expected_jd.m_frac - result_jd.m_frac)) {
      err() << "TimeFormat<Jd>::convert method converted datetime_type pair (" << expected_datetime.first << ", " <<
        expected_datetime.second << ") into (" << result_jd.m_int << " + " << result_jd.m_frac << ") JD, not (" <<
        expected_jd.m_int << " + " << expected_jd.m_frac << ") JD as expected." << std::endl;
    }

    // Test conversion from a datetime_type object to an Jd1 object (that holds a single JD number of double type).
    Jd1 result_jd1 = jd1_format.convert(expected_datetime);
    tolerance = 100.e-9 / SecPerDay(); // 100 nano-seconds in units of day.
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
    tolerance = 100.e-9 / SecPerDay(); // 100 nano-seconds in units of day.
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
    tolerance = 10.e-6 / SecPerDay(); // 10 micro-seconds in units of day.
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
    tolerance = 100.e-9 / SecPerDay(); // 100 nano-seconds in units of day.
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
    tolerance = 10.e-6 / SecPerDay(); // 10 micro-seconds in units of day.
    if (tolerance < std::fabs(expected_jd1.m_day - result_jd1.m_day)) {
      err() << "TimeFormat<Jd1>::parse method parsed \"" << test_jd_string << "\" into " << result_jd1.m_day << " JD, not " <<
        expected_jd1.m_day << " JD as expected." << std::endl;
    }

    // Test detections of bad times of the day.
    TestOneBadDateTime(mjd_format, datetime_type(51910, -0.001), "Mjd", "a time of the day: -0.001");
    TestOneBadDateTime(mjd_format, datetime_type(51910, SecPerDay() + 0.001), "Mjd", "a time of the day: 86400.001");
    TestOneBadDateTime(mjd1_format, datetime_type(51910, -0.001), "Mjd1", "a time of the day: -0.001");
    TestOneBadDateTime(mjd1_format, datetime_type(51910, SecPerDay() + 0.001), "Mjd1", "a time of the day: 86400.001");
    TestOneBadDateTime(jd_format, datetime_type(51910, -0.001), "Jd", "a time of the day: -0.001");
    TestOneBadDateTime(jd_format, datetime_type(51910, SecPerDay() + 0.001), "Jd", "a time of the day: 86400.001");
    TestOneBadDateTime(jd1_format, datetime_type(51910, -0.001), "Jd1", "a time of the day: -0.001");
    TestOneBadDateTime(jd1_format, datetime_type(51910, SecPerDay() + 0.001), "Jd1", "a time of the day: 86400.001");

    // Test detections of bad MJD numbers.
    TestOneBadTimeRep(mjd_format, Mjd(+1, -.001), "Mjd", "Mjd(+1, -0.001)");
    TestOneBadTimeRep(mjd_format, Mjd(+1, +1.), "Mjd", "Mjd(+1, +1.)");
    TestOneBadTimeRep(mjd_format, Mjd(0, +1.), "Mjd", "Mjd(0, +1.)");
    TestOneBadTimeRep(mjd_format, Mjd(0, -1.), "Mjd", "Mjd(0, -1.)");
    TestOneBadTimeRep(mjd_format, Mjd(-1, +.001), "Mjd", "Mjd(-1, +0.001)");
    TestOneBadTimeRep(mjd_format, Mjd(-1, -1.), "Mjd", "Mjd(-1, -1.)");

    // Test detections of bad JD numbers.
    TestOneBadTimeRep(jd_format, Jd(+1, -.001), "Jd", "Jd(+1, -0.001)");
    TestOneBadTimeRep(jd_format, Jd(+1, +1.), "Jd", "Jd(+1, +1.)");
    TestOneBadTimeRep(jd_format, Jd(0, +1.), "Jd", "Jd(0, +1.)");
    TestOneBadTimeRep(jd_format, Jd(0, -1.), "Jd", "Jd(0, -1.)");
    TestOneBadTimeRep(jd_format, Jd(-1, +.001), "Jd", "Jd(-1, +0.001)");
    TestOneBadTimeRep(jd_format, Jd(-1, -1.), "Jd", "Jd(-1, -1.)");

    // Test detections of bad MJD/JD strings.
    TestOneBadTimeString(mjd_format, "Not A Number", "Mjd");
    TestOneBadTimeString(mjd1_format, "Not A Number", "Mjd1");
    TestOneBadTimeString(jd_format, "Not A Number", "Jd");
    TestOneBadTimeString(jd1_format, "Not A Number", "Jd1");

    // Prepare test parameters for Calendar, IsoWeek, and Ordinal classes.
    const TimeFormat<Calendar> & calendar_format(TimeFormatFactory<Calendar>::getFormat());
    const TimeFormat<IsoWeek> & iso_week_format(TimeFormatFactory<IsoWeek>::getFormat());
    const TimeFormat<Ordinal> & ordinal_format(TimeFormatFactory<Ordinal>::getFormat());
    expected_datetime = datetime_type(54634, 45296.789);
    Calendar expected_calendar(2008, 6, 17, 12, 34, 56.789);
    IsoWeek expected_iso_week(2008, 25, 2, 12, 34, 56.789);
    Ordinal expected_ordinal(2008, 169, 12, 34, 56.789);
    tolerance = 100.e-9; // 100 nano-seconds.

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
    tolerance = 100.e-9; // 100 nano-seconds.
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
    tolerance = 100.e-9; // 100 nano-seconds.
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
    tolerance = 100.e-9; // 100 nano-seconds.
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
    TestOneBadDateTime(calendar_format, datetime_type(51910, -0.001), "Calendar", "a time of the day: -0.001");
    TestOneBadDateTime(iso_week_format, datetime_type(51910, -0.001), "IsoWeek", "a time of the day: -0.001");
    TestOneBadDateTime(ordinal_format, datetime_type(51910, -0.001), "Ordinal", "a time of the day: -0.001");

    // Test detections of non-existing month.
    TestOneBadTimeRep(calendar_format, Calendar(2008,  0, 1, 0, 0, 0.), "Calendar", "a bad calendar month: 0.");
    TestOneBadTimeRep(calendar_format, Calendar(2008, 13, 1, 0, 0, 0.), "Calendar", "a bad calendar month: 13.");

    TestOneBadTimeString(calendar_format, "2008-00-01T00:00:00.0", "Calendar");
    TestOneBadTimeString(calendar_format, "2008-13-01T00:00:00.0", "Calendar");

    // Test detections of non-existing day of month.
    TestOneBadTimeRep(calendar_format, Calendar(2008, 1,  0, 0, 0, 0.), "Calendar", "a non-existing calendar date: 2008-01-00.");
    TestOneBadTimeRep(calendar_format, Calendar(2008, 1, 32, 0, 0, 0.), "Calendar", "a non-existing calendar date: 2008-01-32.");
    TestOneBadTimeRep(calendar_format, Calendar(2008, 1, 31, 0, 0, 0.), "Calendar", "an existing calendar date: 2008-01-31.", false);
    TestOneBadTimeRep(calendar_format, Calendar(2008, 4, 31, 0, 0, 0.), "Calendar", "a non-existing calendar date: 2008-04-31.");

    TestOneBadTimeString(calendar_format, "2008-01-00T00:00:00.0", "Calendar");
    TestOneBadTimeString(calendar_format, "2008-01-32T00:00:00.0", "Calendar");
    TestOneBadTimeString(calendar_format, "2008-01-31T00:00:00.0", "Calendar", false);
    TestOneBadTimeString(calendar_format, "2008-04-31T00:00:00.0", "Calendar");

    // Test detections of non-existing day near the end of February.
    TestOneBadTimeRep(calendar_format, Calendar(2008, 2, 30, 0, 0, 0.), "Calendar", "a non-existing calendar date: 2008-02-30.");
    TestOneBadTimeRep(calendar_format, Calendar(2008, 2, 29, 0, 0, 0.), "Calendar", "an existing calendar date: 2008-02-29.", false);
    TestOneBadTimeRep(calendar_format, Calendar(2009, 2, 29, 0, 0, 0.), "Calendar", "a non-existing calendar date: 2009-02-29.");
    TestOneBadTimeRep(calendar_format, Calendar(2100, 2, 29, 0, 0, 0.), "Calendar", "a non-existing calendar date: 2100-02-29.");
    TestOneBadTimeRep(calendar_format, Calendar(2000, 2, 29, 0, 0, 0.), "Calendar", "an existing calendar date: 2000-02-29.", false);

    TestOneBadTimeString(calendar_format, "2008-02-30T00:00:00.0", "Calendar");
    TestOneBadTimeString(calendar_format, "2008-02-29T00:00:00.0", "Calendar", false);
    TestOneBadTimeString(calendar_format, "2009-02-29T00:00:00.0", "Calendar");
    TestOneBadTimeString(calendar_format, "2100-02-29T00:00:00.0", "Calendar");
    TestOneBadTimeString(calendar_format, "2000-02-29T00:00:00.0", "Calendar", false);

    // Test detections of non-existing time.
    TestOneBadTimeRep(calendar_format, Calendar(2008, 6, 17, -1,  0,  0.), "Calendar", "a non-existing hour: -1.");
    TestOneBadTimeRep(calendar_format, Calendar(2008, 6, 17, 24,  0,  0.), "Calendar", "a non-existing hour: 24.");
    TestOneBadTimeRep(calendar_format, Calendar(2008, 6, 17,  0, -1,  0.), "Calendar", "a non-existing minute: -1.");
    TestOneBadTimeRep(calendar_format, Calendar(2008, 6, 17,  0, 60,  0.), "Calendar", "a non-existing minute: 60.");
    TestOneBadTimeRep(calendar_format, Calendar(2008, 6, 17,  0,  0, -1.), "Calendar", "a non-existing second: -1.");

    TestOneBadTimeString(calendar_format, "2008-06-17T-1:00:00.0", "Calendar");
    TestOneBadTimeString(calendar_format, "2008-06-17T24:00:00.0", "Calendar");
    TestOneBadTimeString(calendar_format, "2008-06-17T00:-1:00.0", "Calendar");
    TestOneBadTimeString(calendar_format, "2008-06-17T00:60:00.0", "Calendar");
    TestOneBadTimeString(calendar_format, "2008-06-17T00:00:-1.0", "Calendar");
    // Note: TimeFormat<Calendar> cannot detect the second part exceeding its maximum because of a possible leap second insertion,
    //       and TimeSystem class should test it instead.

    // Test detections of non-existing week number.
    TestOneBadTimeRep(iso_week_format, IsoWeek(2008,  0, 1, 0,  0,  0.), "IsoWeek", "a non-existing week number: 0.");
    TestOneBadTimeRep(iso_week_format, IsoWeek(2008, 53, 1, 0,  0,  0.), "IsoWeek", "a non-existing week number: 53.");
    TestOneBadTimeRep(iso_week_format, IsoWeek(2009, 53, 1, 0,  0,  0.), "IsoWeek", "an existing week number: 53.", false);
    TestOneBadTimeRep(iso_week_format, IsoWeek(2009, 54, 1, 0,  0,  0.), "IsoWeek", "a non-existing week number: 54.");

    TestOneBadTimeString(iso_week_format, "2008-W00-1T00:00:00.0", "IsoWeek");
    TestOneBadTimeString(iso_week_format, "2008-W53-1T00:00:00.0", "IsoWeek");
    TestOneBadTimeString(iso_week_format, "2009-W53-1T00:00:00.0", "IsoWeek", false);
    TestOneBadTimeString(iso_week_format, "2009-W54-1T00:00:00.0", "IsoWeek");

    // Test detections of non-existing day of week.
    TestOneBadTimeRep(iso_week_format, IsoWeek(2008, 25, 0, 0,  0,  0.), "IsoWeek", "a non-existing day of the week: 0.");
    TestOneBadTimeRep(iso_week_format, IsoWeek(2008, 25, 8, 0,  0,  0.), "IsoWeek", "a non-existing day of the week: 8.");

    TestOneBadTimeString(iso_week_format, "2008-W25-0T00:00:00.0", "IsoWeek");
    TestOneBadTimeString(iso_week_format, "2008-W25-8T00:00:00.0", "IsoWeek");

    // Test detections of non-existing time.
    TestOneBadTimeRep(iso_week_format, IsoWeek(2008, 25, 2, -1,  0,  0.), "IsoWeek", "a non-existing hour: -1.");
    TestOneBadTimeRep(iso_week_format, IsoWeek(2008, 25, 2, 24,  0,  0.), "IsoWeek", "a non-existing hour: 24.");
    TestOneBadTimeRep(iso_week_format, IsoWeek(2008, 25, 2,  0, -1,  0.), "IsoWeek", "a non-existing minute: -1.");
    TestOneBadTimeRep(iso_week_format, IsoWeek(2008, 25, 2,  0, 60,  0.), "IsoWeek", "a non-existing minute: 60.");
    TestOneBadTimeRep(iso_week_format, IsoWeek(2008, 25, 2,  0,  0, -1.), "IsoWeek", "a non-existing second: -1.");

    TestOneBadTimeString(iso_week_format, "2008-W25-2T-1:00:00.0", "IsoWeek");
    TestOneBadTimeString(iso_week_format, "2008-W25-2T24:00:00.0", "IsoWeek");
    TestOneBadTimeString(iso_week_format, "2008-W25-2T00:-1:00.0", "IsoWeek");
    TestOneBadTimeString(iso_week_format, "2008-W25-2T00:60:00.0", "IsoWeek");
    TestOneBadTimeString(iso_week_format, "2008-W25-2T00:00:-1.0", "IsoWeek");
    // Note: TimeFormat<IsoWeek> cannot detect the second part exceeding its maximum because of a possible leap second insertion,
    //       and TimeSystem class should test it instead.

    // Test detections of non-existing ordinal day.
    TestOneBadTimeRep(ordinal_format, Ordinal(2008, 367, 0, 0, 0.), "Ordinal", "a non-existing ordinal date: 2008-367.");
    TestOneBadTimeRep(ordinal_format, Ordinal(2008, 366, 0, 0, 0.), "Ordinal", "an existing ordinal date: 2008-366.", false);
    TestOneBadTimeRep(ordinal_format, Ordinal(2009, 366, 0, 0, 0.), "Ordinal", "a non-existing ordinal date: 2009-366.");
    TestOneBadTimeRep(ordinal_format, Ordinal(2100, 366, 0, 0, 0.), "Ordinal", "a non-existing ordinal date: 2100-366.");
    TestOneBadTimeRep(ordinal_format, Ordinal(2000, 366, 0, 0, 0.), "Ordinal", "an existing ordinal date: 2000-366.", false);

    TestOneBadTimeString(ordinal_format, "2008-367T00:00:00.0", "Ordinal");
    TestOneBadTimeString(ordinal_format, "2008-366T00:00:00.0", "Ordinal", false);
    TestOneBadTimeString(ordinal_format, "2009-366T00:00:00.0", "Ordinal");
    TestOneBadTimeString(ordinal_format, "2100-366T00:00:00.0", "Ordinal");
    TestOneBadTimeString(ordinal_format, "2000-366T00:00:00.0", "Ordinal", false);

    // Test detections of non-existing time.
    TestOneBadTimeRep(ordinal_format, Ordinal(2008, 169, -1,  0,  0.), "Ordinal", "a non-existing hour: -1.");
    TestOneBadTimeRep(ordinal_format, Ordinal(2008, 169, 24,  0,  0.), "Ordinal", "a non-existing hour: 24.");
    TestOneBadTimeRep(ordinal_format, Ordinal(2008, 169,  0, -1,  0.), "Ordinal", "a non-existing minute: -1.");
    TestOneBadTimeRep(ordinal_format, Ordinal(2008, 169,  0, 60,  0.), "Ordinal", "a non-existing minute: 60.");
    TestOneBadTimeRep(ordinal_format, Ordinal(2008, 169,  0,  0, -1.), "Ordinal", "a non-existing second: -1.");

    TestOneBadTimeString(ordinal_format, "2008-169T-1:00:00.0", "Ordinal");
    TestOneBadTimeString(ordinal_format, "2008-169T24:00:00.0", "Ordinal");
    TestOneBadTimeString(ordinal_format, "2008-169T00:-1:00.0", "Ordinal");
    TestOneBadTimeString(ordinal_format, "2008-169T00:60:00.0", "Ordinal");
    TestOneBadTimeString(ordinal_format, "2008-169T00:00:-1.0", "Ordinal");
    // Note: TimeFormat<Ordinal> cannot detect the second part exceeding its maximum because of a possible leap second insertion,
    //       and TimeSystem class should test it instead.

    // Test detections of bad Calendar, IsoWeek, and Ordinal strings.
    TestOneBadTimeString(calendar_format, "Not A Number", "Calendar");
    TestOneBadTimeString(iso_week_format, "Not A Number", "IsoWeek");
    TestOneBadTimeString(ordinal_format, "Not A Number", "Ordinal");

    // Test detections of wrong kinds of calendar-like strings to parse.
    TestOneBadTimeString(calendar_format, expected_iso_week_string, "Calendar");
    TestOneBadTimeString(calendar_format, expected_ordinal_string, "Calendar");
    TestOneBadTimeString(iso_week_format, expected_calendar_string, "IsoWeek");
    TestOneBadTimeString(iso_week_format, expected_ordinal_string, "IsoWeek");
    TestOneBadTimeString(ordinal_format, expected_calendar_string, "Ordinal");
    TestOneBadTimeString(ordinal_format, expected_iso_week_string, "Ordinal");

    // Test date conversions in year 1995, where the calendar year is not divisible by 4, 100, nor 400.
    TestOneCalendarDate(49776, 1995,  2, 28, 1995,  9, 2,  59);
    TestOneCalendarDate(49777, 1995,  3,  1, 1995,  9, 3,  60);
    TestOneCalendarDate(50082, 1995, 12, 31, 1995, 52, 7, 365);
    TestOneCalendarDate(50083, 1996,  1,  1, 1996,  1, 1,   1);

    // Test date conversions in year 1996, where the calendar year is divisible by 4, but not by 100.
    TestOneCalendarDate(50141, 1996,  2, 28, 1996,  9, 3,  59);
    TestOneCalendarDate(50142, 1996,  2, 29, 1996,  9, 4,  60);
    TestOneCalendarDate(50143, 1996,  3,  1, 1996,  9, 5,  61);
    TestOneCalendarDate(50448, 1996, 12, 31, 1997,  1, 2, 366);
    TestOneCalendarDate(50449, 1997,  1,  1, 1997,  1, 3,   1);

    // Test date conversions in year 2100, where the calendar year is divisible by 100, but not by 400.
    TestOneCalendarDate(88127, 2100,  2, 28, 2100,  8, 7,  59);
    TestOneCalendarDate(88128, 2100,  3,  1, 2100,  9, 1,  60);
    TestOneCalendarDate(88433, 2100, 12, 31, 2100, 52, 5, 365);
    TestOneCalendarDate(88434, 2101,  1,  1, 2100, 52, 6,   1);

    // Test date conversions in year 2000, where the calendar year is divisible by 400.
    TestOneCalendarDate(51602, 2000,  2, 28, 2000,  9, 1,  59);
    TestOneCalendarDate(51603, 2000,  2, 29, 2000,  9, 2,  60);
    TestOneCalendarDate(51604, 2000,  3,  1, 2000,  9, 3,  61);
    TestOneCalendarDate(51909, 2000, 12, 31, 2000, 52, 7, 366);
    TestOneCalendarDate(51910, 2001,  1,  1, 2001,  1, 1,   1);

    // Test date conversions near the beginning of 2005, when an ISO year starts after a calendar year.
    TestOneCalendarDate(53370, 2004, 12, 31, 2004, 53, 5, 366);
    TestOneCalendarDate(53371, 2005,  1,  1, 2004, 53, 6,   1);
    TestOneCalendarDate(53372, 2005,  1,  2, 2004, 53, 7,   2);
    TestOneCalendarDate(53373, 2005,  1,  3, 2005,  1, 1,   3);

    // Test date conversions near the beginning of 2007, when an ISO year starts with a calendar year.
    TestOneCalendarDate(54100, 2006, 12, 31, 2006, 52, 7, 365);
    TestOneCalendarDate(54101, 2007,  1,  1, 2007,  1, 1,   1);
    TestOneCalendarDate(54102, 2007,  1,  2, 2007,  1, 2,   2);

    // Test date conversions near the beginning of 2008, when an ISO year starts before a calendar year.
    TestOneCalendarDate(54464, 2007, 12, 30, 2007, 52, 7, 364);
    TestOneCalendarDate(54465, 2007, 12, 31, 2008,  1, 1, 365);
    TestOneCalendarDate(54466, 2008,  1,  1, 2008,  1, 2,   1);

    // Test date conversions near the beginning of 2009, when the ISO year is three days into the previous Gregorian year.
    TestOneCalendarDate(54828, 2008, 12, 28, 2008, 52, 7, 363);
    TestOneCalendarDate(54829, 2008, 12, 29, 2009,  1, 1, 364);
    TestOneCalendarDate(54830, 2008, 12, 30, 2009,  1, 2, 365);
    TestOneCalendarDate(54831, 2008, 12, 31, 2009,  1, 3, 366);
    TestOneCalendarDate(54832, 2009,  1,  1, 2009,  1, 4,   1);

    // Test date conversions near the beginning of 2010, when the ISO year is three days into the next Gregorian year.
    TestOneCalendarDate(55196, 2009, 12, 31, 2009, 53, 4, 365);
    TestOneCalendarDate(55197, 2010,  1,  1, 2009, 53, 5,   1);
    TestOneCalendarDate(55198, 2010,  1,  2, 2009, 53, 6,   2);
    TestOneCalendarDate(55199, 2010,  1,  3, 2009, 53, 7,   3);
    TestOneCalendarDate(55200, 2010,  1,  4, 2010,  1, 1,   4);
  }

  void TestBaryTimeComputer() {
    s_os.setMethod("TestBaryTimeComputer");

    // Get a barycentric time computer.
    BaryTimeComputer & computer = BaryTimeComputer::getComputer();

    // Prepare a time to be barycentered and an expected result after barycentered.
    AbsoluteTime glast_tt_origin("TT", 51910, 64.184);
    double glast_time = 2.123393677090199E+08; // TSTART in my_pulsar_events_v3.fits.
    AbsoluteTime original = glast_tt_origin + ElapsedTime("TT", Duration(0, glast_time));
    AbsoluteTime result = original;
    AbsoluteTime glast_tdb_origin("TDB", 51910, 64.184);
    glast_time = 2.123393824137859E+08; // TSTART in my_pulsar_events_bary_v3.fits.
    AbsoluteTime expected = glast_tdb_origin + ElapsedTime("TDB", Duration(0, glast_time));

    // Set parameters for barycentering.
    double ra = 85.0482;
    double dec = -69.3319;
    double sc_pos_array[] = {3311146.54815027, 5301968.82897028, 3056651.22812332}; // SC position at TSTART (computed separately).
    std::vector<double> sc_pos(sc_pos_array, sc_pos_array + 3);

    // Check initial setting of ephemeris name.
    std::string ephem_name = computer.getPlanetaryEphemerisName();
    if (!ephem_name.empty()) {
      err() << "BaryTimeComputer::getPlanetaryEphemerisName() returned a non-empty string before initialized." << std::endl;
    }

    // Test error handling for calls before initialization.
    try {
      computer.computeBaryTime(ra, dec, sc_pos, result);
      err() << "BaryTimeComputer::computeBaryTime(" << ra << ", " << dec << ", array(" << sc_pos[0] << ", " << sc_pos[1] << ", " <<
        sc_pos[2] << "), AbsoluteTime(" << result << ")) did not throw an exception when it should." << std::endl;
    } catch (const std::exception &) {
    }

    // Initialize the barycentric time computer.
    computer.initialize("JPL DE405");

    // Check ephemeris name.
    ephem_name = computer.getPlanetaryEphemerisName();
    if ("JPL DE405" != ephem_name) {
      err() << "BaryTimeComputer::getPlanetaryEphemerisName() returned \"" << ephem_name << "\", not \"JPL DE405\"." << std::endl;
    }

    // Test barycentric correction.
    computer.computeBaryTime(ra, dec, sc_pos, result);
    ElapsedTime tolerance("TDB", Duration(0, 1.e-7));
    if (!result.equivalentTo(expected, tolerance)) {
      err() << "BaryTimeComputer::correct(" << ra << ", " << dec << ", " << original << ")" <<
        " returned AbsoluteTime(" << result << "), not equivalent to AbsoluteTime(" << expected <<
        ") with tolerance of " << tolerance << "." << std::endl;
    }
  }

  class BogusTimeHandler1: public EventTimeHandler {
    public:
      virtual ~BogusTimeHandler1() {}

      static EventTimeHandler * createInstance(const std::string & /*file_name*/, const std::string & /*extension_name*/,
        const double /*angular_tolerance*/, const bool /*read_only*/ = true)
        { return 0; }

      virtual void setSpacecraftFile(const std::string & /*sc_file_name*/, const std::string & /*sc_extension_name*/) {}

      virtual AbsoluteTime parseTimeString(const std::string & /*time_string*/, const std::string & /*time_system*/ = "FILE") const
        { return AbsoluteTime("TDB", 51911, 0.); }

    protected:
      virtual AbsoluteTime readTime(const tip::Header & /*header*/, const std::string & /*keyword_name*/,
        const bool /*request_bary_time*/, const double /*ra*/, const double /*dec*/) const
        { return AbsoluteTime("TDB", 51911, 0.); }

      virtual AbsoluteTime readTime(const tip::TableRecord & /*record*/, const std::string & /*column_name*/,
        const bool /*request_bary_time*/, const double /*ra*/, const double /*dec*/) const
        { return AbsoluteTime("TDB", 51911, 0.); }

    private:
      BogusTimeHandler1(const std::string & file_name, const std::string & extension_name):
      EventTimeHandler(file_name, extension_name, 0.) {}
  };

  class BogusTimeHandler2: public EventTimeHandler {
    public:
      virtual ~BogusTimeHandler2() {}

      static EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name,
        const double angular_tolerance, const bool read_only = true)
        { return new BogusTimeHandler2(file_name, extension_name, angular_tolerance, read_only); }

      virtual void setSpacecraftFile(const std::string & /*sc_file_name*/, const std::string & /*sc_extension_name*/) {}

      virtual AbsoluteTime parseTimeString(const std::string & /*time_string*/, const std::string & /*time_system*/ = "FILE") const
        { return AbsoluteTime("TDB", 51911, 0.); }

    protected:
      virtual AbsoluteTime readTime(const tip::Header & /*header*/, const std::string & /*keyword_name*/,
        const bool /*request_bary_time*/, const double /*ra*/, const double /*dec*/) const
        { return AbsoluteTime("TDB", 51912, 0.); }

      virtual AbsoluteTime readTime(const tip::TableRecord & /*record*/, const std::string & /*column_name*/,
        const bool /*request_bary_time*/, const double /*ra*/, const double /*dec*/) const
        { return AbsoluteTime("TDB", 51912, 0.); }

    private:
      BogusTimeHandler2(const std::string & file_name, const std::string & extension_name, const double angular_tolerance,
        const bool read_only = true):
      EventTimeHandler(file_name, extension_name, angular_tolerance, read_only) {}
  };

  void TestEventTimeHandlerFactory() {
    s_os.setMethod("TestEventtTimeHandler");
    using namespace facilities;

    // Set tolerance for AbsoluteTime comparison.
    ElapsedTime time_tolerance("TT", Duration(0, 1.e-7));

    // Prepare test parameters in this method.
    std::string event_file = commonUtilities::joinPath(commonUtilities::getDataPath("timeSystem"), "my_pulsar_events_v3.fits");
    AbsoluteTime glast_tt_origin("TT", 51910, 64.184);
    double glast_time = 2.123393677090199E+08; // TSTART in my_pulsar_events_v3.fits.
    AbsoluteTime expected_glast = glast_tt_origin + ElapsedTime("TT", Duration(0, glast_time));
    AbsoluteTime expected_bogus2("TDB", 51912, 0.);
    double angular_tolerance = 0.;

    // Test creation of BogusTimeHandler1 (an EventTimeHandler sub-class) through its createInstance method.
    std::auto_ptr<EventTimeHandler> handler(0);
    handler.reset(BogusTimeHandler1::createInstance(event_file, "EVENTS", angular_tolerance));
    if (handler.get() != 0) {
      err() << "BogusTimeHandler1::createInstance method did not return a null pointer (0)." << std::endl;
    }

    // Test creation of BogusTimeHandler2 (an EventTimeHandler sub-class) through its createInstance method.
    handler.reset(BogusTimeHandler2::createInstance(event_file, "EVENTS", angular_tolerance));
    AbsoluteTime result = handler->readHeader("TSTART");
    if (!result.equivalentTo(expected_bogus2, time_tolerance)) {
      err() << "BogusTimeHandler2::createInstance method did not return a BogusTimeHandler2 object." << std::endl;
    }

    // Test creation of GlastTimeHandler (an EventTimeHandler sub-class) through its createInstance method.
    handler.reset(GlastTimeHandler::createInstance(event_file, "EVENTS", angular_tolerance));
    result = handler->readHeader("TSTART");
    if (!result.equivalentTo(expected_glast, time_tolerance)) {
      err() << "GlastTimeHandler::createInstance method did not return a GlastTimeHandler object." << std::endl;
    }

    // Test the decision-making mechanism for cases without a prior setup.
    try {
      handler.reset(IEventTimeHandlerFactory::createHandler(event_file, "EVENTS", angular_tolerance));
      err() << "IEventTimeHandlerFactory::createHandler method did not throw an exception when no handler was registered." << std::endl;
    } catch (const std::exception &) {
    }

    // Register BogusTimeHandler1 to EventTimeHandlerFactory.
    EventTimeHandlerFactory<BogusTimeHandler1> factory1;

    // Test the decision-making mechanism for cases with only BogusTimeHandler1 registered.
    try {
      handler.reset(IEventTimeHandlerFactory::createHandler(event_file, "EVENTS", angular_tolerance));
      err() << "IEventTimeHandlerFactory::createHandler method did not throw an exception when only BogusTimeHandler1 was registered."
        << std::endl;
    } catch (const std::exception &) {
    }

    // Register GlastTimeHandler to EventTimeHandlerFactory.
    EventTimeHandlerFactory<GlastTimeHandler> factory2;

    // Test the decision-making mechanism for cases with BogusTimeHandler1 and GlastTimeHandler registered.
    handler.reset(IEventTimeHandlerFactory::createHandler(event_file, "EVENTS", angular_tolerance));
    result = handler->readHeader("TSTART");
    if (!result.equivalentTo(expected_glast, time_tolerance)) {
      err() << "IEventTimeHandlerFactory::createHandler method did not return a GlastTimeHandler object" <<
        " when it is the only appropriate handler." << std::endl;
    }

    // Register GlastTimeHandler to EventTimeHandlerFactory.
    EventTimeHandlerFactory<BogusTimeHandler2> factory3;

    // Test the decision-making mechanism for cases with BogusTimeHandler1, GlastTimeHandler, and BogusTimeHandler2 registered.
    handler.reset(IEventTimeHandlerFactory::createHandler(event_file, "EVENTS", angular_tolerance));
    result = handler->readHeader("TSTART");
    if (!result.equivalentTo(expected_glast, time_tolerance)) {
      err() << "IEventTimeHandlerFactory::createHandler method did not return a GlastTimeHandler object" <<
        " when it is an appropriate handler that can respond first." << std::endl;
    }

    // De-register and re-register three handlers in a different order.
    factory1.deregisterHandler();
    factory2.deregisterHandler();
    factory3.deregisterHandler();
    factory3.registerHandler();
    factory2.registerHandler();
    factory1.registerHandler();

    // Test the decision-making mechanism for cases with BogusTimeHandler1, GlastTimeHandler, and BogusTimeHandler2 registered.
    handler.reset(IEventTimeHandlerFactory::createHandler(event_file, "EVENTS", angular_tolerance));
    result = handler->readHeader("TSTART");
    if (!result.equivalentTo(expected_bogus2, time_tolerance)) {
      err() << "IEventTimeHandlerFactory::createHandler method did not return a BogusTimeHandler2 object" <<
        " when it is an appropriate handler that can respond first." << std::endl;
    }
  }

  void TestGlastTimeHandler() {
    s_os.setMethod("TestGlastTimeHandler");
    using namespace facilities;

    // Set tolerance for AbsoluteTime comparison.
    ElapsedTime time_tolerance("TT", Duration(0, 1.e-7));

    // Get and initialize a barycentric time computer.
    BaryTimeComputer & computer = BaryTimeComputer::getComputer();
    computer.initialize("JPL DE405");

    // Prepare test parameters in this method.
    std::string event_file = commonUtilities::joinPath(commonUtilities::getDataPath("timeSystem"), "my_pulsar_events_v3.fits");
    std::string event_file_bary = commonUtilities::joinPath(commonUtilities::getDataPath("timeSystem"), "my_pulsar_events_bary_v3.fits");
    std::string sc_file = commonUtilities::joinPath(commonUtilities::getDataPath("timeSystem"), "my_pulsar_spacecraft_data_v3r1.fits");
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

    // Create an GlastTimeHandler object for EVENTS extension of an event file.
    std::auto_ptr<EventTimeHandler> handler(0);
    handler.reset(GlastTimeHandler::createInstance(event_file, "EVENTS", 0.));

    // Test setting to the first record.
    handler->setFirstRecord();
    double glast_time;
    handler->getCurrentRecord()["TIME"].get(glast_time);
    double expected_glast_time = 2.123393701794728E+08; // TIME in the first row of my_pulsar_events_v3.fits.
    double epsilon = 1.e-7; // 100 nano-seconds.
    if (std::fabs(glast_time - expected_glast_time) > epsilon) {
      err() << "GlastTimeHandler::getCurrentRecord() did not return the first record after GlastTimeHandler::setFirstRecord()." <<
        std::endl;
    }

    // Test setting to the third record.
    handler->setFirstRecord();
    handler->setNextRecord();
    handler->setNextRecord();
    handler->getCurrentRecord()["TIME"].get(glast_time);
    expected_glast_time = 2.123393750454886E+08; // TIME in the third row of my_pulsar_events_v3.fits.
    if (std::fabs(glast_time - expected_glast_time) > epsilon) {
      err() << "GlastTimeHandler::getCurrentRecord() did not return the third record after GlastTimeHandler::setFirstRecord()" <<
        " followed by two GlastTimeHandler::setNextRecord() calls." << std::endl;
    }

    // Test setting to the last record.
    handler->setLastRecord();
    handler->getCurrentRecord()["TIME"].get(glast_time);
    expected_glast_time = 2.124148548657289E+08; // TIME in the last row of my_pulsar_events_v3.fits.
    if (std::fabs(glast_time - expected_glast_time) > epsilon) {
      err() << "GlastTimeHandler::getCurrentRecord() did not return the last record after GlastTimeHandler::setLastRecord()." <<
        std::endl;
    }

    // Test testing the end of table.
    handler->setLastRecord();
    if (handler->isEndOfTable()) {
      err() << "GlastTimeHandler::isEndOfTable() returned true after GlastTimeHandler::setLastRecord()." << std::endl;
    }
    handler->setNextRecord();
    if (!handler->isEndOfTable()) {
      err() << "GlastTimeHandler::isEndOfTable() returned false after GlastTimeHandler::setLastRecord()" <<
        " followed by GlastTimeHandler::setNextRecord()." << std::endl;
    }

    // Test parsing a time string.
    std::string time_string = "12345.6789012345";
    AbsoluteTime result = handler->parseTimeString(time_string);
    AbsoluteTime glast_tt_origin("TT", 51910, 64.184);
    AbsoluteTime expected = glast_tt_origin + ElapsedTime("TT", Duration(0, 12345.6789012345));
    if (!result.equivalentTo(expected, time_tolerance)) {
      err() << "GlastTimeHandler::parseTimeString(\"" << time_string << "\") returned AbsoluteTime(" << result <<
        "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
    }

    // Test parsing a time string, with a different time system.
    time_string = "12345.6789012345";
    result = handler->parseTimeString(time_string, "TDB");
    AbsoluteTime glast_tdb_origin("TDB", 51910, 64.184);
    expected = glast_tdb_origin + ElapsedTime("TDB", Duration(0, 12345.6789012345));
    if (!result.equivalentTo(expected, time_tolerance)) {
      err() << "GlastTimeHandler::parseTimeString(\"" << time_string << "\", \"TDB\") returned AbsoluteTime(" << result <<
        "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
    }

    // Test reading header keyword value.
    result = handler->readHeader("TSTART");
    glast_time = 2.123393677090199E+08; // TSTART in my_pulsar_events_v3.fits.
    expected = glast_tt_origin + ElapsedTime("TT", Duration(0, glast_time));
    if (!result.equivalentTo(expected, time_tolerance)) {
      err() << "GlastTimeHandler::readHeader(\"TSTART\") returned AbsoluteTime(" << result << "), not equivalent to AbsoluteTime(" <<
        expected << ") with tolerance of " << time_tolerance << "." << std::endl;
    }

    // Test reading header keyword value, requesting barycentering, before setting a spacecraft file.
    try {
      result = handler->readHeader("TSTART", ra, dec);
      err() << "GlastTimeHandler::readHeader(\"TSTART\", " << ra << ", " << dec << 
        ") did not throw an exception when it should." << std::endl;
    } catch (const std::exception &) {
    }

    // Set a spacecraft file name.
    handler->setSpacecraftFile(sc_file, "SC_DATA");

    // Test reading header keyword value, requesting barycentering.
    result = handler->readHeader("TSTART", ra, dec);
    glast_time = 2.123393824137859E+08; // TSTART in my_pulsar_events_bary_v3.fits.
    expected = glast_tdb_origin + ElapsedTime("TDB", Duration(0, glast_time));
    if (!result.equivalentTo(expected, time_tolerance)) {
      err() << "GlastTimeHandler::readHeader(\"TSTART\", " << ra << ", " << dec << ") returned AbsoluteTime(" << result <<
        "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
    }

    // Test reading TIME column value.
    handler->setFirstRecord(); // Points to the first event.
    handler->setNextRecord();  // Points to the second event.
    handler->setNextRecord();  // Points to the third event.
    result = handler->readColumn("TIME");
    glast_time = 2.123393750454886E+08; // TIME of the third row in my_pulsar_events_v3.fits.
    expected = glast_tt_origin + ElapsedTime("TT", Duration(0, glast_time));
    if (!result.equivalentTo(expected, time_tolerance)) {
      err() << "GlastTimeHandler::readColumn(\"TIME\", " << ra << ", " << dec << ") returned AbsoluteTime(" << result <<
        "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
    }

    // Test reading TIME column value, requesting barycentering.
    handler->setFirstRecord(); // Re-setting to the first event.
    handler->setNextRecord();  // Points to the second event.
    handler->setNextRecord();  // Points to the third event.
    result = handler->readColumn("TIME", ra, dec);
    glast_time = 2.123393897503012E+08; // TIME of the third row in my_pulsar_events_bary_v3.fits.
    expected = glast_tdb_origin + ElapsedTime("TDB", Duration(0, glast_time));
    if (!result.equivalentTo(expected, time_tolerance)) {
      err() << "GlastTimeHandler::readColumn(\"TIME\", " << ra << ", " << dec << ") returned AbsoluteTime(" << result <<
        "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
    }

    // Create an GlastTimeHandler object for EVENTS extension of a barycentered event file.
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "EVENTS", angular_tolerance));
    handler->setSpacecraftFile(sc_file, "SC_DATA");

    // Test parsing a time string.
    time_string = "12345.6789012345";
    result = handler->parseTimeString(time_string);
    expected = glast_tdb_origin + ElapsedTime("TDB", Duration(0, 12345.6789012345));
    if (!result.equivalentTo(expected, time_tolerance)) {
      err() << "GlastTimeHandler::parseTimeString(\"" << time_string << "\") returned AbsoluteTime(" << result <<
        "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
    }

    // Test parsing a time string, with a different time system.
    time_string = "12345.6789012345";
    result = handler->parseTimeString(time_string, "TT");
    expected = glast_tt_origin + ElapsedTime("TT", Duration(0, 12345.6789012345));
    if (!result.equivalentTo(expected, time_tolerance)) {
      err() << "GlastTimeHandler::parseTimeString(\"" << time_string << "\", \"TT\") returned AbsoluteTime(" << result <<
        "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
    }

    // Test reading header keyword value, requesting barycentering.
    result = handler->readHeader("TSTART", ra, dec);
    glast_time = 2.123393824137859E+08; // TSTART in my_pulsar_events_bary_v3.fits.
    expected = glast_tdb_origin + ElapsedTime("TDB", Duration(0, glast_time));
    if (!result.equivalentTo(expected, time_tolerance)) {
      err() << "GlastTimeHandler::readHeader(\"TSTART\", " << ra << ", " << dec << ") returned AbsoluteTime(" << result <<
        "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
    }

    // Test reading header keyword value, requesting barycentering with a wrong sky position (ra, dec).
    try {
      result = handler->readHeader("TSTART", ra_wrong, dec_wrong);
      err() << "GlastTimeHandler::readHeader(\"TSTART\", " << ra_wrong << ", " << dec_wrong << 
        ") did not throw an exception when it should." << std::endl;
    } catch (const std::exception &) {
    }

    // Test reading header keyword value, requesting barycentering with a different, but close sky position (ra, dec).
    try {
      result = handler->readHeader("TSTART", ra_close, dec_close);
    } catch (const std::exception &) {
      err() << "GlastTimeHandler::readHeader(\"TSTART\", " << ra_close << ", " << dec_close << 
        ") threw an exception when it should not." << std::endl;
    }

    // Test reading column value, requesting barycentering.
    handler->setFirstRecord(); // Points to the first event.
    handler->setNextRecord();  // Points to the second event.
    handler->setNextRecord();  // Points to the third event.
    result = handler->readColumn("TIME", ra, dec);
    glast_time = 2.123393897503012E+08; // TIME of the third row in my_pulsar_events_bary_v3.fits.
    expected = glast_tdb_origin + ElapsedTime("TDB", Duration(0, glast_time));
    if (!result.equivalentTo(expected, time_tolerance)) {
      err() << "GlastTimeHandler::readColumn(\"TIME\", " << ra << ", " << dec << ") returned AbsoluteTime(" << result <<
        "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
    }

    // Test reading column value, requesting barycentering with a wrong sky position (ra, dec).
    try {
      result = handler->readColumn("TIME", ra_wrong, dec_wrong);
      err() << "GlastTimeHandler::readColumn(\"TIME\", " << ra_wrong << ", " << dec_wrong << 
        ") did not throw an exception when it should." << std::endl;
    } catch (const std::exception &) {
    }

    // Test reading column value, requesting barycentering with a different, but close sky position (ra, dec).
    try {
      result = handler->readColumn("TIME", ra_close, dec_close);
    } catch (const std::exception &) {
      err() << "GlastTimeHandler::readHeader(\"TIME\", " << ra_close << ", " << dec_close << 
        ") threw an exception when it should not." << std::endl;
    }

    // Test exact match in sky position (ra, dec), with angular tolerance of zero (0) degree.
    angular_tolerance = 0.;
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "EVENTS", angular_tolerance));
    handler->setSpacecraftFile(sc_file, "SC_DATA");
    try {
      handler->checkSkyPosition(ra, dec);
    } catch (const std::exception &) {
      err() << "GlastTimeHandler::checkSkyPosition(" << ra << ", " << dec << 
        ") threw an exception with angular tolerance of zero (0) degree." << std::endl;
    }

    // Test large angular tolerance of 180 degrees.
    angular_tolerance = 180.;
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "EVENTS", angular_tolerance));
    handler->setSpacecraftFile(sc_file, "SC_DATA");
    try {
      handler->checkSkyPosition(ra_wrong, dec_wrong);
    } catch (const std::exception &) {
      err() << "GlastTimeHandler::checkSkyPosition(" << ra_wrong << ", " << dec_wrong << 
        ") threw an exception with angular tolerance of 180 degrees." << std::endl;
    }

    // Test large angular difference, with small angular tolerance.
    angular_tolerance = 1.e-8;
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "EVENTS", angular_tolerance));
    handler->setSpacecraftFile(sc_file, "SC_DATA");
    try {
      handler->checkSkyPosition(ra_opposite, dec_opposite);
      err() << "GlastTimeHandler::checkSkyPosition(\"TSTART\", " << ra_opposite << ", " << dec_opposite << 
        ") did not throw an exception with angular tolerance of zero (0) degrees." << std::endl;
    } catch (const std::exception &) {
    }

    // Test large angular difference, with large angular tolerance of 180 degrees.
    angular_tolerance = 180.;
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "EVENTS", angular_tolerance));
    handler->setSpacecraftFile(sc_file, "SC_DATA");
    try {
      handler->checkSkyPosition(ra_opposite, dec_opposite);
    } catch (const std::exception &) {
      err() << "GlastTimeHandler::checkSkyPosition(" << ra_opposite << ", " << dec_opposite << 
        ") threw an exception with angular tolerance of 180 degrees." << std::endl;
    }

    // Test checking solar system ephemeris name, with a non-barycentered event extension.
    angular_tolerance = 1.e-8;
    handler.reset(GlastTimeHandler::createInstance(event_file, "EVENTS", angular_tolerance));
    handler->setSpacecraftFile(sc_file, "SC_DATA");
    try {
      handler->checkSolarEph("Bogus Name");
    } catch (const std::exception &) {
      err() << "GlastTimeHandler::checkSolarEph(\"Bogus Name\") threw an exception for non-barycentered event extension." << std::endl;
    }

    // Test checking solar system ephemeris name, with a barycentered event extension (exact match).
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "EVENTS", angular_tolerance));
    handler->setSpacecraftFile(sc_file, "SC_DATA");
    try {
      handler->checkSolarEph("JPL-DE405");
    } catch (const std::exception &) {
      err() << "GlastTimeHandler::checkSolarEph(\"JPL-DE405\") threw an exception for a barycentered event extension." << std::endl;
    }

    // Test checking solar system ephemeris name, with a barycentered event extension (rough match).
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "EVENTS", angular_tolerance));
    handler->setSpacecraftFile(sc_file, "SC_DATA");
    try {
      handler->checkSolarEph("JPL DE405");
    } catch (const std::exception &) {
      err() << "GlastTimeHandler::checkSolarEph(\"JPL DE405\") threw an exception for a barycentered event extension." << std::endl;
    }

    // Test checking solar system ephemeris name, with a barycentered event extension (no match).
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "EVENTS", angular_tolerance));
    handler->setSpacecraftFile(sc_file, "SC_DATA");
    try {
      handler->checkSolarEph("JPL DE200");
      err() << "GlastTimeHandler::checkSolarEph(\"JPL DE200\") did not throw an exception for a barycentered event extension." <<
        std::endl;
    } catch (const std::exception &) {
    }

    // Test checking solar system ephemeris name, with a barycentered GTI extension (rough match).
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "GTI", angular_tolerance));
    handler->setSpacecraftFile(sc_file, "SC_DATA");
    try {
      handler->checkSolarEph("JPL DE405");
    } catch (const std::exception &) {
      err() << "GlastTimeHandler::checkSolarEph(\"JPL DE405\") threw an exception for a barycentered GTI extension." << std::endl;
    }

    // Test checking solar system ephemeris name, with a barycentered GTI extension (no match).
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "GTI", angular_tolerance));
    handler->setSpacecraftFile(sc_file, "SC_DATA");
    try {
      handler->checkSolarEph("JPL DE200");
      err() << "GlastTimeHandler::checkSolarEph(\"JPL DE200\") did not throw an exception for a barycentered GTI extension." <<
        std::endl;
    } catch (const std::exception &) {
    }

  }
}

StAppFactory<TestTimeSystemApp> g_factory("test_timeSystem");
