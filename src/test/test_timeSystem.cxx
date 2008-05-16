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
#include "timeSystem/Duration.h"
#include "timeSystem/ElapsedTime.h"
#include "timeSystem/EventTimeHandler.h"
#include "timeSystem/Field.h"
#include "timeSystem/GlastMetRep.h"
#include "timeSystem/GlastTimeHandler.h"
#include "timeSystem/TimeInterval.h"
#include "timeSystem/TimeRep.h"
#include "timeSystem/TimeSystem.h"

#include <cmath>
#include <exception>
#include <limits>

#include <list>

namespace {

  bool s_failed = false;
  st_stream::StreamFormatter s_os("test_timeSystem", "", 2);

  void TestDuration();

  void TestTimeSystem();

  void TestAbsoluteTime();

  void TestElapsedTime();

  void TestIntFracPair();

  void TestTimeInterval();

  void TestTimeRep();

  void TestTimeFormat();

  void TestField();

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

  // Test IntFracPair class.
  TestIntFracPair();

  // Test TimeInterval class.
  TestTimeInterval();

  // Test TimeRep class.
  TestTimeRep();

  // Test TimeFormat class.
  TestTimeFormat();

  // Test Field class.
  TestField();

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

  struct TestParameter {
    TestParameter(long day, double sec, TimeUnit_e unit, long int_part, double frac_part, double tolerance):
      m_day(day), m_sec(sec), m_unit(unit), m_int_part(int_part), m_frac_part(frac_part), m_tolerance(tolerance) {};
    long m_day;
    double m_sec;
    TimeUnit_e m_unit;
    long m_int_part;
    double m_frac_part;
    double m_tolerance;
  };

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

    std::list<TestParameter> parameter_list;
    double epsilon = std::numeric_limits<double>::epsilon();

    // For tests of Duration::getValue() method for Duration of +6 days.
    parameter_list.push_back(TestParameter(6, 0., Day,  6,         0., epsilon));
    parameter_list.push_back(TestParameter(6, 0., Hour, 6 * 24,    0., epsilon));
    parameter_list.push_back(TestParameter(6, 0., Min,  6 * 1440,  0., epsilon));
    parameter_list.push_back(TestParameter(6, 0., Sec,  6 * 86400, 0., epsilon));

    // For tests of Duration::getValue() method for Duration of +6 seconds.
    parameter_list.push_back(TestParameter(0, 6., Day,  0, 6. / 86400., epsilon));
    parameter_list.push_back(TestParameter(0, 6., Hour, 0, 6. / 3600.,  epsilon));
    parameter_list.push_back(TestParameter(0, 6., Min,  0, 6. / 60.,    epsilon));
    parameter_list.push_back(TestParameter(0, 6., Sec,  6, 0.,          epsilon));

    // For tests of Duration::getValue() method for Duration of -6 days.
    parameter_list.push_back(TestParameter(-6, 0., Day,  -6,         0., epsilon));
    parameter_list.push_back(TestParameter(-6, 0., Hour, -6 * 24,    0., epsilon));
    parameter_list.push_back(TestParameter(-6, 0., Min,  -6 * 1440,  0., epsilon));
    parameter_list.push_back(TestParameter(-6, 0., Sec,  -6 * 86400, 0., epsilon));

    // For tests of Duration::getValue() method for Duration of -6 seconds.
    parameter_list.push_back(TestParameter(0, -6., Day,   0, -6. / 86400., epsilon));
    parameter_list.push_back(TestParameter(0, -6., Hour,  0, -6. / 3600.,  epsilon));
    parameter_list.push_back(TestParameter(0, -6., Min,   0, -6. / 60.,    epsilon));
    parameter_list.push_back(TestParameter(0, -6., Sec,  -6,  0.,          epsilon));

    // Run the tests prepared above.
    std::string unit_name[] = { "Day", "Hour", "Min", "Sec"};
    for (std::list<TestParameter>::iterator itor = parameter_list.begin(); itor != parameter_list.end(); itor++) {
      IntFracPair time_value = Duration(itor->m_day, itor->m_sec).getValue(itor->m_unit);
      if (!(itor->m_int_part == time_value.getIntegerPart() &&
            std::fabs(itor->m_frac_part - time_value.getFractionalPart()) < itor->m_tolerance)) {
        err() << "Duration(" << itor->m_day << ", " << itor->m_sec << ").getValue(" << unit_name[itor->m_unit] << ") returned " <<
          time_value << ", not " << IntFracPair(itor->m_int_part, itor->m_frac_part) << " as expected." <<
          std::endl;
      }
    }

    // Tests of constructor taking IntFracPair.
    long int_part = 3456789;
    double frac_part = .56789567895678956789;
    TimeUnit_e time_unit = Day;
    Duration expected_result(int_part, frac_part*86400.);
    Duration dur_tol(0, 1.e-9);
    if (!Duration(IntFracPair(int_part, frac_part), time_unit).equivalentTo(expected_result, dur_tol)) {
      err() << "Duration(IntFracPair(" << int_part << ", " << frac_part << "), " << unit_name[time_unit] <<
        ").equivalentTo returned false for " << expected_result << " with tolerance of " << dur_tol <<
        ", not true as expected." << Duration(IntFracPair(int_part, frac_part), time_unit) << std::endl;
    }
    time_unit = Hour;
    expected_result = Duration(int_part/24, (int_part%24 + frac_part) * 3600.);
    dur_tol = Duration(0, 1.e-9);
    if (!Duration(IntFracPair(int_part, frac_part), time_unit).equivalentTo(expected_result, dur_tol)) {
      err() << "Duration(IntFracPair(" << int_part << ", " << frac_part << "), " << unit_name[time_unit] <<
        ").equivalentTo returned false for " << expected_result << " with tolerance of " << dur_tol <<
        ", not true as expected." << Duration(IntFracPair(int_part, frac_part), time_unit) << std::endl;
    }
    time_unit = Min;
    expected_result = Duration(int_part/1440, (int_part%1440 + frac_part) * 60.);
    dur_tol = Duration(0, 1.e-9);
    if (!Duration(IntFracPair(int_part, frac_part), time_unit).equivalentTo(expected_result, dur_tol)) {
      err() << "Duration(IntFracPair(" << int_part << ", " << frac_part << "), " << unit_name[time_unit] <<
        ").equivalentTo returned false for " << expected_result << " with tolerance of " << dur_tol <<
        ", not true as expected." << Duration(IntFracPair(int_part, frac_part), time_unit) << std::endl;
    }
    time_unit = Sec;
    expected_result = Duration(int_part/86400, int_part%86400 + frac_part);
    dur_tol = Duration(0, 1.e-9);
    if (!Duration(IntFracPair(int_part, frac_part), time_unit).equivalentTo(expected_result, dur_tol)) {
      err() << "Duration(IntFracPair(" << int_part << ", " << frac_part << "), " << unit_name[time_unit] <<
        ").equivalentTo returned false for " << expected_result << " with tolerance of " << dur_tol <<
        ", not true as expected." << Duration(IntFracPair(int_part, frac_part), time_unit) << std::endl;
    }

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

    // Tests for detection of overflow/underflow:
    // A case which should *not* overflow, but is close to overflowing.
    try {
      Duration sec_max(0, std::numeric_limits<long>::max() + .4);
      sec_max.getValue(Sec);
    } catch (const std::exception & x) {
      err() << "A test which should not overflow unexpectedly caught: " << x.what() << std::endl;
    }
    // A case which should *not* underflow, but is close to underflowing.
    try {
      Duration sec_min(0, std::numeric_limits<long>::min() - .4);
      sec_min.getValue(Sec);
    } catch (const std::exception & x) {
      err() << "A test which should not underflow unexpectedly caught: " << x.what() << std::endl;
    }
    // A case which should overflow.
    double overflow_sec = std::numeric_limits<long>::max() + 1.1;
    try {
      Duration sec_max(0, overflow_sec);
      sec_max.getValue(Sec);
      err() << "Duration::sec method unexpectedly did not overflow for " << overflow_sec << " seconds." << std::endl;
    } catch (const std::exception &) {
    }
    // A case which should underflow.
    double underflow_sec = std::numeric_limits<long>::min() - 1.1;
    try {
      Duration sec_min(0, underflow_sec);
      sec_min.getValue(Sec);
      err() << "Duration::sec method unexpectedly did not underflow for " << underflow_sec << " seconds." << std::endl;
    } catch (const std::exception &) {
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
    long day_part = 0;
    double sec_part = 0.;
    std::string unit_part;
    {
      std::stringstream os;
      os << Duration(0, 86399.) + Duration(0, 1. - epsilon);
      os >> day_part >> unit_part >> sec_part >> unit_part;
      if (0. > sec_part) {
        err() << "Duration(0, 86399.) + Duration(0, 1. - " << epsilon <<") returned Duration(" << day_part << ", " << sec_part <<
          "), whose second part is negative, not positive as expected." << std::endl;
      }
    }
    {
      std::stringstream os;
      os << Duration(0, 1. - epsilon) + Duration(0, 86399.);
      os >> day_part >> unit_part >> sec_part >> unit_part;
      if (0. > sec_part) {
        err() << "Duration(0, 1. - " << epsilon <<") + Duration(0, 86399.) returned Duration(" << day_part << ", " << sec_part <<
          "), whose second part is negative, not positive as expected." << std::endl;
      }
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
    Duration zero(0, 0.);
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
  }

  void TestOneConversion(const std::string & src_name, const Duration & src_origin, const Duration & src,
    const std::string & dest_name, const Duration & dest_origin, const Duration & expected_dest, double tolerance = 1.e-9) {
    s_os.setMethod("TestOneConversion");
    const TimeSystem & src_sys(TimeSystem::getSystem(src_name));
    const TimeSystem & dest_sys(TimeSystem::getSystem(dest_name));
    Moment dest_moment = dest_sys.convertFrom(src_sys, Moment(src_origin, src));
    Duration dest = dest_moment.second + dest_sys.computeTimeDifference(dest_moment.first, dest_origin);
    if (!dest.equivalentTo(expected_dest, Duration(0, tolerance))) {
      err() << "Converting from " << src_sys << " to " << dest_sys << ", Moment(" << src_origin << ", " << src <<
        ") was converted to Moment(" << dest_moment.first << ", " << dest_moment.second << "), not equivalent to Moment(" <<
        dest_origin << ", " << expected_dest << ") with tolerance of " << tolerance << "." << std::endl;
    }
  }

  void TestTimeSystem() {
    s_os.setMethod("TestTimeSystem");

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
    Duration tai_ref_time(long(365*.25), 100. - 32.184);
    Duration tt_ref_time(long(365*.25), 100.);
    Duration tdb_ref_time(long(365*.25), 100. + 0.001634096289);
    Duration utc_ref_time(long(365*.25), 100. - 32. - 32.184);

    double tdb_tolerance = 1.e-7; // 100 ns is the accuracy of algorithms involving TDB.

    // Test conversions, reflexive cases.
    TestOneConversion("TAI", Duration(51910, 0.), tai_ref_time, "TAI", Duration(51910, 0.), tai_ref_time);
    TestOneConversion("TDB", Duration(51910, 0.), tdb_ref_time, "TDB", Duration(51910, 0.), tdb_ref_time);
    TestOneConversion("TT",  Duration(51910, 0.), tt_ref_time,  "TT",  Duration(51910, 0.), tt_ref_time);
    TestOneConversion("UTC", Duration(51910, 0.), utc_ref_time, "UTC", Duration(51910, 0.), utc_ref_time);

    // Test conversions from TAI to...
    TestOneConversion("TAI", Duration(51910, 0.), tai_ref_time, "TDB", Duration(51910, 0.), tdb_ref_time, tdb_tolerance);
    TestOneConversion("TAI", Duration(51910, 0.), tai_ref_time, "TT",  Duration(51910, 0.), tt_ref_time);
    TestOneConversion("TAI", Duration(51910, 0.), tai_ref_time, "UTC", Duration(51910, 0.), utc_ref_time);

    // Test conversions from TDB to...
    TestOneConversion("TDB", Duration(51910, 0.), tdb_ref_time, "TAI", Duration(51910, 0.), tai_ref_time, tdb_tolerance);
    TestOneConversion("TDB", Duration(51910, 0.), tdb_ref_time, "TT",  Duration(51910, 0.), tt_ref_time, tdb_tolerance);
    TestOneConversion("TDB", Duration(51910, 0.), tdb_ref_time, "UTC", Duration(51910, 0.), utc_ref_time, tdb_tolerance);

    // Test conversions from TT to...
    TestOneConversion("TT",  Duration(51910, 0.), tt_ref_time, "TAI", Duration(51910, 0.), tai_ref_time);
    TestOneConversion("TT",  Duration(51910, 0.), tt_ref_time, "TDB", Duration(51910, 0.), tdb_ref_time, tdb_tolerance);
    TestOneConversion("TT",  Duration(51910, 0.), tt_ref_time, "UTC", Duration(51910, 0.), utc_ref_time);

    // Test conversions from UTC to...
    TestOneConversion("UTC", Duration(51910, 0.), utc_ref_time, "TAI", Duration(51910, 0.), tai_ref_time);
    TestOneConversion("UTC", Duration(51910, 0.), utc_ref_time, "TDB", Duration(51910, 0.), tdb_ref_time, tdb_tolerance);
    TestOneConversion("UTC", Duration(51910, 0.), utc_ref_time, "TT",  Duration(51910, 0.), tt_ref_time);

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
    TestOneConversion("UTC", Duration(leap1, 0.), Duration(0, 0.), "TAI", Duration(leap1, diff1), Duration(0, 0.));
    // --- Slightly before a leap second is inserted.
    TestOneConversion("UTC", Duration(leap1, -.001), Duration(0, 0.), "TAI", Duration(leap1, diff0 - .001), Duration(0, 0.));
    // --- Same as above, but with a large elapsed time.
    //     Although the total time (origin + elapsed) is large enough to cross two leap second boundaries, still
    //     the earliest leap second should be used because the choice of leap second is based only on the origin time.
    TestOneConversion("UTC", Duration(leap1, -.001), Duration(delta_leap, 2.002), "TAI", Duration(leap1, diff0 - .001), 
      Duration(delta_leap, 2.002));

    // To ensure TAI->UTC is handled correctly, do some tougher conversions, i.e. times which are close to
    // times when leap seconds are inserted.
    // --- At the end of a leap second.
    TestOneConversion("TAI", Duration(leap1, diff1), Duration(0, 0.), "UTC", Duration(leap1, 0.), Duration(0, 0.));
    // --- During a leap second.
    TestOneConversion("TAI", Duration(leap1, diff1 - 0.3), Duration(0, 0.), "UTC", Duration(leap1, 0.), Duration(0, -0.3));
    // --- At the beginning of a leap second.
    TestOneConversion("TAI", Duration(leap1, diff1 - 1.0), Duration(0, 0.), "UTC", Duration(leap1, 0.), Duration(0, -1.0));
    // --- After the end of a leap second.
    TestOneConversion("TAI", Duration(leap1, diff1 + 0.3), Duration(0, 0.), "UTC", Duration(leap1, 0.), Duration(0, +0.3));
    // --- Before the beginning of a leap second.
    TestOneConversion("TAI", Duration(leap1, diff1 - 1.3), Duration(0, 0.), "UTC", Duration(leap1, 0.), Duration(0, -1.3));

    // Test that conversion uses table keyed by TAI times, not by UTC.
    TestOneConversion("TAI", Duration(leap1, -2.), Duration(1, 0.), "UTC", Duration(leap1, -diff0 - 2.), Duration(1, 0.));

    // Test case before first time covered by the current UTC definition. This is "undefined" in the current scheme.
    try {
      TestOneConversion("UTC", Duration(0, 0.), Duration(0, 0.), "TAI", Duration(0, 0.), Duration(0, 0.));
      err() << "Conversion of time 0. MJD UTC to TAI did not throw an exception." << std::endl;
    } catch (const std::exception &) {
      // That's OK!
    }
    try {
      TestOneConversion("TAI", Duration(0, 0.), Duration(0, 0.), "UTC", Duration(0, 0.), Duration(0, 0.));
      err() << "Conversion of time 0. MJD TAI to UTC did not throw an exception." << std::endl;
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

    using namespace facilities;
    std::string test_leap = commonUtilities::joinPath(commonUtilities::getDataPath("timeSystem"), "bogusls.fits");

    // Test loading specific file for leap second table, by first setting the default file name.
    TimeSystem::setDefaultLeapSecFileName(test_leap);

    if (test_leap != TimeSystem::getDefaultLeapSecFileName()) 
      err() << "After setting default leap second file name, default leap second file name was " <<
        TimeSystem::getDefaultLeapSecFileName() << ", not " << test_leap << " as expected." << std::endl;

    TimeSystem::loadLeapSeconds();

    // Test case after last time covered by the current UTC definition.
    long leap_last = 53737;
    double diff_last = 31.;
    TestOneConversion("UTC", Duration(leap_last, 100.), Duration(0, 0.), "TAI", Duration(leap_last, 100.), Duration(0, diff_last));

    // Reset default leap second file name.
    TimeSystem::setDefaultLeapSecFileName("");

    // Finally, test loading the real leap seconds file.
    TimeSystem::loadLeapSeconds(test_leap);

    // Test case after last time covered by the current UTC definition.
    leap_last = 53737;
    diff_last = 31.;
    TestOneConversion("UTC", Duration(leap_last, 100.), Duration(0, 0.), "TAI", Duration(leap_last, 100.), Duration(0, diff_last));

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
    TestOneConversion("TAI", Duration(leap1, diff1), Duration(0, 0.), "UTC", Duration(leap1, 0.), Duration(0, 0.));
    // --- Slightly before a leap second is removed.
    TestOneConversion("TAI", Duration(leap1, diff1 - .001), Duration(0, 0.), "UTC", Duration(leap1, 0.), Duration(0, -.001));
    // --- Same as above, but with a large elapsed time.
    //     Although the total time (origin + elapsed) is large enough to cross two leap second boundaries, still
    //     the earliest leap second should be used because the choice of leap second is based only on the origin time.
    TestOneConversion("TAI", Duration(leap1, diff1 - .001), Duration(delta_leap, .002), "UTC",
      Duration(leap1, 0.), Duration(delta_leap, .001));

    // To ensure UTC->TAI is handled correctly, do some tougher conversions, i.e. times which are close to
    // times when leap seconds are removed.
    // --- At the end of a leap second.
    TestOneConversion("UTC", Duration(leap1, 0.), Duration(0, 0.), "TAI", Duration(leap1, diff1), Duration(0, 0.));
    // --- During a leap second.
    TestOneConversion("UTC", Duration(leap1, -0.3), Duration(0, 0.), "TAI", Duration(leap1, diff1), Duration(0, 0.));
    // --- At the beginning of a leap second.
    TestOneConversion("UTC", Duration(leap1, -1.0), Duration(0, 0.), "TAI", Duration(leap1, diff1), Duration(0, 0.));
    // --- After the end of a leap second.
    TestOneConversion("UTC", Duration(leap1, +0.3), Duration(0, 0.), "TAI", Duration(leap1, diff1 + 0.3), Duration(0, 0.));
    // --- Before the beginning of a leap second.
    TestOneConversion("UTC", Duration(leap1, -1.3), Duration(0, 0.), "TAI", Duration(leap1, diff0 - 1.3), Duration(0, 0.));

    // Test computeTimeDifference method.
    double deltat = 20.;
    std::list<std::pair<Duration, double> > test_input;
    test_input.push_back(std::make_pair(Duration(51910, 100.),  0.)); // middle of nowhere
    test_input.push_back(std::make_pair(Duration(leap2 - 1, SecPerDay() - 10.), +1.)); // leap second insertion
    test_input.push_back(std::make_pair(Duration(leap1 - 1, SecPerDay() - 10.), -1.)); // leap second removal
    test_input.push_back(std::make_pair(Duration(leap1 - 1, SecPerDay() - .7),  -.7)); // non-existing time in UTC

    Duration tolerance(0, 1.e-9); // 1 nanosecond.

    for (std::list<std::pair<Duration, double> >::iterator itor_test = test_input.begin(); itor_test != test_input.end(); ++itor_test) {
      Duration mjd1(itor_test->first + Duration(0, deltat));
      Duration mjd2(itor_test->first);

      std::map<std::string, Duration> expected_diff;
      expected_diff["TAI"] = Duration(0, deltat);
      expected_diff["TDB"] = Duration(0, deltat);
      expected_diff["TT"]  = Duration(0, deltat);
      expected_diff["UTC"] = Duration(0, deltat + itor_test->second);

      for (std::map<std::string, Duration>::iterator itor_exp = expected_diff.begin(); itor_exp != expected_diff.end(); ++itor_exp) {
        std::string time_system_name = itor_exp->first;
        const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
        Duration time_diff = time_system.computeTimeDifference(mjd1, mjd2);
        if (!time_diff.equivalentTo(expected_diff[time_system_name], tolerance)) {
          err() << "computeTimeDifference(mjd1, mjd2) of " << time_system_name << " returned " << time_diff <<
            " for mjd1 = " << mjd1 << " and mjd2 = " << mjd2 << ", not equivalent to the expected result, " <<
            expected_diff[time_system_name] << ", with tolerance of " << tolerance << "." << std::endl;
        }
      }
    }

    // Test computeMjd method.
    std::list<std::pair<Moment, Duration> > test_input_moment;
    // Middle of nowhere
    test_input_moment.push_back(std::make_pair(Moment(Duration(51910, 0.), Duration(0, 100.)), Duration(51910, 100.)));
    // Across leap second insertion in UTC
    test_input_moment.push_back(std::make_pair(Moment(Duration(leap2 - 1, SecPerDay() - 10.), Duration(0, deltat)),
      Duration(leap2, deltat - 10. - 1.)));
    // Across leap second removal in UTC
    test_input_moment.push_back(std::make_pair(Moment(Duration(leap1 - 1, SecPerDay() - 10.), Duration(0, deltat)),
      Duration(leap1, deltat - 10. + 1.)));
    // Non-existing time in UTC
    test_input_moment.push_back(std::make_pair(Moment(Duration(leap1 - 1, SecPerDay() - .7), Duration(0, deltat)), Duration(leap1, deltat)));

    // Tests at times close to times when leap seconds are inserted (in UTC).
    // --- Before the beginning of a leap second.
    test_input_moment.push_back(std::make_pair(Moment(Duration(leap2 - 1, 0.), Duration(0, SecPerDay() - 0.3)), Duration(leap2, -0.3)));
    // --- At the beginning of a leap second.
    test_input_moment.push_back(std::make_pair(Moment(Duration(leap2 - 1, 0.), Duration(0, SecPerDay())),       Duration(leap2, 0.)));
    // --- During a leap second.
    test_input_moment.push_back(std::make_pair(Moment(Duration(leap2 - 1, 0.), Duration(0, SecPerDay() + 0.3)), Duration(leap2, 0.)));
    // --- At the end of a leap second.
    test_input_moment.push_back(std::make_pair(Moment(Duration(leap2 - 1, 0.), Duration(0, SecPerDay() + 1.0)), Duration(leap2, 0.)));
    // --- After the end of a leap second.
    test_input_moment.push_back(std::make_pair(Moment(Duration(leap2 - 1, 0.), Duration(0, SecPerDay() + 1.3)), Duration(leap2, 0.3)));

    // Tests at times close to times when leap seconds are removed (in UTC).
    // --- Before the beginning of a leap second.
    test_input_moment.push_back(std::make_pair(Moment(Duration(leap1 - 1, SecPerDay() - 1.3), Duration(0, 0.)), Duration(leap1, -1.3)));
    // --- At the beginning of a leap second.
    test_input_moment.push_back(std::make_pair(Moment(Duration(leap1 - 1, SecPerDay() - 1.0), Duration(0, 0.)), Duration(leap1, 0.)));
    // --- During a leap second.
    test_input_moment.push_back(std::make_pair(Moment(Duration(leap1 - 1, SecPerDay() - 0.3), Duration(0, 0.)), Duration(leap1, 0.)));
    // --- At the end of a leap second.
    test_input_moment.push_back(std::make_pair(Moment(Duration(leap1 - 1, SecPerDay()),       Duration(0, 0.)), Duration(leap1, 0.)));
    // --- After the end of a leap second.
    test_input_moment.push_back(std::make_pair(Moment(Duration(leap1 - 1, SecPerDay() + 0.3), Duration(0, 0.)), Duration(leap1, 0.3)));

    for (std::list<std::pair<Moment, Duration> >::iterator itor_test = test_input_moment.begin(); itor_test != test_input_moment.end();
      ++itor_test) {
      Moment time = itor_test->first;
      Duration mjd_utc = itor_test->second;

      std::map<std::string, Duration> expected_mjd;
      expected_mjd["TAI"] = time.first + time.second;
      expected_mjd["TDB"] = time.first + time.second;
      expected_mjd["TT"]  = time.first + time.second;
      expected_mjd["UTC"] = mjd_utc;

      for (std::map<std::string, Duration>::iterator itor_exp = expected_mjd.begin(); itor_exp != expected_mjd.end(); ++itor_exp) {
        std::string time_system_name = itor_exp->first;
        const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
        Duration mjd = time_system.computeMjd(time);
        if (!mjd.equivalentTo(expected_mjd[time_system_name], tolerance)) {
          err() << "computeMjd of " << time_system_name << " returned " << mjd <<
            " for Moment(" << time.first << ", " << time.second << "), not equivalent to the expected result, " <<
            expected_mjd[time_system_name] << ", with tolerance of " << tolerance << "." << std::endl;
        }
      }
    }

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

    // Test of conversion condition of TAI-to-UTC conversion.
    // Below must produce identical result for test input of Moment tai_moment(Duration(51910, 0.), Duration(0, 100.));
    Moment tai_moment(Duration(0, 100.), Duration(51910, 0.));
    Moment expected_moment(Duration(51910, 0.), Duration(0, 100. - diff2));
    try {
      TestOneConversion("TAI", tai_moment.first, tai_moment.second, "UTC", expected_moment.first, expected_moment.second);
    } catch (const std::exception &) {
      err() << "Conversion of TAI to UTC for Moment(" << tai_moment.first << ", " << tai_moment.second << ") threw an exception." <<
        std::endl;
    }

    // Test computeMjd method of UTC for a time during a leap second being inserted.
    Moment utc_moment = Moment(Duration(leap2 - 1, SecPerDay() - 1.), Duration(0, 1. +  2./3.));
    Duration result = TimeSystem::getSystem("UTC").computeMjd(utc_moment);
    Duration expected_result(leap2, 0.);
    if (result != expected_result) {
      err() << "UTC system's computeMjd(" << utc_moment.first << ", " << utc_moment.second << ") returned " <<
        result << ", not exactly equal to " << expected_result << " as expected." << std::endl;
    }

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

    // Test of origin for UTC time that must be after MJD 41317.0 (January 1st, 1972) by definition.
    long oldest_mjd = 41317;
    tai_moment = Moment(Duration(oldest_mjd - 1, 0.), Duration(0, SecPerDay() * 2.));
    utc_moment = TimeSystem::getSystem("UTC").convertFrom(TimeSystem::getSystem("TAI"), tai_moment);
    if (Duration(oldest_mjd, 0.) > utc_moment.first) {
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

    Duration mjd_origin(51910);
    Duration duration(0, 1000.); // MET 1000. seconds

    // Create an absolute time corresponding to MET 1000. s.
    AbsoluteTime abs_time("TDB", mjd_origin, duration);

    // Display this time.
    std::cout << "Testing AbsoluteTime::write by writing this time: " << abs_time << std::endl;

    // Test adding an elapsed time to this time.
    // Create an absolute time corresponding to 100. s MET TDB to verify adding an elapsed time.
    Duration delta_t(0, 100.);
    ElapsedTime elapsed_time("TDB", delta_t);
    AbsoluteTime result = abs_time + elapsed_time;
    AbsoluteTime expected_result("TDB", mjd_origin, duration + delta_t);
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
    expected_result = AbsoluteTime("TDB", mjd_origin, duration - delta_t);
    if (!result.equivalentTo(expected_result, epsilon))
      err() << "Sum of absolute time and elapsed time using operator -= was " << result << ", not " <<
        expected_result << " as expected." << std::endl;

    // Test adding in reverse order.
    result = elapsed_time + abs_time;
    expected_result = AbsoluteTime("TDB", mjd_origin, duration + delta_t);
    if (!result.equivalentTo(expected_result, epsilon))
      err() << "Sum of elapsed time and absolute time in that order was " << result << ", not " <<
        expected_result << " as expected." << std::endl;

    // Test subtraction of elapsed time from absolute time.
    result = abs_time - elapsed_time;
    expected_result = AbsoluteTime("TDB", mjd_origin, duration - delta_t);
    if (!result.equivalentTo(expected_result, epsilon))
      err() << "Elapsed time subtracted from absolute time gave " << result << ", not " <<
        expected_result << " as expected." << std::endl;

    // Make a test time which is later than the first time.
    AbsoluteTime later_time("TDB", mjd_origin, duration + Duration(0, 100.));

    // Test comparison operators: >, >=, <, and <=.
    CompareAbsoluteTime(abs_time, later_time);

    // Test comparison operators (>, >=, <, and <=) in UTC system.
    long mjd_leap = 51179;
    AbsoluteTime abs_time_utc("UTC", Duration(mjd_leap - 1, 86390.0), Duration(0, 10.8));
    AbsoluteTime later_time_utc("UTC", Duration(mjd_leap, 0.2), Duration(0, 0.));
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
    Duration difference = (later_time - abs_time).computeElapsedTime("TDB").getTime();
    if (!expected_diff.equivalentTo(difference, tolerance))
      err() << "Absolute time [" << abs_time << "] subtracted from absolute time [" << later_time << "] gave " << difference <<
        ", not " << expected_diff << " as expected." << std::endl;

    // Test subtraction of absolute time from absolute time in UTC system
    expected_diff = Duration(0, .4);
    difference = (later_time_utc - abs_time_utc).computeElapsedTime("UTC").getTime();
    if (!expected_diff.equivalentTo(difference, tolerance))
      err() << "Absolute time [" << abs_time_utc << "] subtracted from absolute time [" << later_time_utc << "] gave " << difference <<
        ", not " << expected_diff << " as expected." << std::endl;
  }

  void TestElapsedTime() {
    s_os.setMethod("TestElapsedTime");

    Duration expected_dur(0, 1000.);
    Duration tolerance(0, 1.e-9); // 1 ns.
    ElapsedTime elapsed("TDB", expected_dur);
    Duration returned_dur = elapsed.getTime();
    if (!returned_dur.equivalentTo(expected_dur, tolerance)) {
      err() << "After ElapsedTime elapsed(\"TDB\", " << expected_dur << "), its getTime() returned " << returned_dur <<
        ", not equivalent to " << expected_dur << " with tolerance of " << tolerance << "." << std::endl;
    }

    ElapsedTime negative_elapsed = -elapsed;
    expected_dur = Duration(0, -1000.);
    returned_dur = negative_elapsed.getTime();
    if (!returned_dur.equivalentTo(expected_dur, tolerance)) {
      err() << "After ElapsedTime negative_elapsed = -elapsed, where elapsed = " << elapsed <<
        ", its getTime() returned " << returned_dur << ", not equivalent to " << expected_dur <<
        " with tolerance of " << tolerance << "." << std::endl;
    }
  }

  static void CompareIntFracPair(const std::string & hint, const IntFracPair & value, long expected_int_part, double expected_frac_part) {
    if (expected_int_part != value.getIntegerPart()) {
      err() << hint << ", integer part of the IntFracPair object was " <<
        value.getIntegerPart() << ", not " << expected_int_part << " as expected." << std::endl;
    }

    double epsilon = std::numeric_limits<double>::epsilon() * 10.;
    if (epsilon < std::fabs(expected_frac_part - value.getFractionalPart())) {
      err() << hint << ", fractional part of the IntFracPair object was " <<
        value.getFractionalPart() << ", not " << expected_frac_part << " as expected." << std::endl;
    }
  }

  void TestIntFracPair() {
    s_os.setMethod("TestIntFracPair");

    // Construction a test object.
    IntFracPair int_frac;

    // Setup string stream for error message.
    std::ostringstream os;
    os.precision(s_os.err().precision());
    std::string context;

    // Test construction from a pair of long and double.
    long int_part = 100;
    double frac_part = .56789567895678956789;
    os << int_part << ", " << frac_part;
    context = os.str();
    os.str("");

    int_frac = IntFracPair(int_part, frac_part);
    CompareIntFracPair("After int_frac = IntFracPair(" + context + ")", int_frac, int_part, frac_part);

    // Test construction from a double, making sure the separate int and fractional parts are as expected.
    double dval = 56789.56789567895678956789;
    int_part = 56789;
    frac_part = .567895678900000;
    os << "dval = " << dval;
    context = os.str();
    os.str("");

    int_frac = IntFracPair(dval);
    CompareIntFracPair("After int_frac = IntFracPair(" + context + ")", int_frac, int_part, frac_part);

    // Test construction from a std::string, making sure the separate int and fractional parts are as expected.
    std::string sval = "00050089.56789567895678956789";
    int_part = 50089;
    frac_part = .56789567895678900000;
    os << "sval = \"" << sval << "\"";
    context = os.str();
    os.str("");

    int_frac = IntFracPair(sval);
    CompareIntFracPair("After int_frac = IntFracPair(" + context + ")", int_frac, int_part, frac_part);

    sval = "  +1e+3  ";
    int_part = 1000;
    frac_part = 0.;
    os << "sval = \"" << sval << "\"";
    context = os.str();
    os.str("");
    int_frac = IntFracPair(sval);
    CompareIntFracPair("After int_frac = IntFracPair(" + context + ")", int_frac, int_part, frac_part);

    sval = "  -2e+3";
    int_part = -2000;
    frac_part = 0.;
    os << "sval = \"" << sval << "\"";
    context = os.str();
    os.str("");
    int_frac = IntFracPair(sval);
    CompareIntFracPair("After int_frac = IntFracPair(" + context + ")", int_frac, int_part, frac_part);

    sval = "3e+3  ";
    int_part = 3000;
    frac_part = 0.;
    os << "sval = \"" << sval << "\"";
    context = os.str();
    os.str("");
    int_frac = IntFracPair(sval);
    CompareIntFracPair("After int_frac = IntFracPair(" + context + ")", int_frac, int_part, frac_part);

    // Test errors resulting from bad string conversions.
    try {
      sval = "! 1.e6";
      int_frac = IntFracPair(sval);
      err() << "IntFracPair(\"" << sval << "\") did not throw exception." << std::endl;
    } catch (const std::exception &) {
      // That's good.
    }

    try {
      sval = "1.e6 0";
      int_frac = IntFracPair(sval);
      err() << "IntFracPair(\"" << sval << "\") did not throw exception." << std::endl;
    } catch (const std::exception &) {
      // That's good.
    }

    try {
      sval = "1..e6";
      int_frac = IntFracPair(sval);
      err() << "IntFracPair(\"" << sval << "\") did not throw exception." << std::endl;
    } catch (const std::exception &) {
      // That's good.
    }

    try {
      sval = "1 + 6";
      int_frac = IntFracPair(sval);
      err() << "IntFracPair(\"" << sval << "\") did not throw exception." << std::endl;
    } catch (const std::exception &) {
      // That's good.
    }

    try {
      sval = "0. 0.";
      int_frac = IntFracPair(sval);
      err() << "IntFracPair(\"" << sval << "\") did not throw exception." << std::endl;
    } catch (const std::exception &) {
      // That's good.
    }

    int_frac = IntFracPair(125, .0123456789012345);
    int_part = int_frac.getIntegerPart();
    frac_part = int_frac.getFractionalPart();
    dval = int_frac.getDouble();
    double dval_expected = int_part + frac_part;

    if (dval != dval_expected)
      err() << "getDouble returned " << dval << ", not " << dval_expected << ", as expected." << std::endl;
  }

  void TestTimeInterval() {
    s_os.setMethod("TestTimeInterval");

    // Create some test inputs.
    AbsoluteTime time1("TDB", Duration(51910), Duration(0, 1000.));
    AbsoluteTime time2("TDB", Duration(51910), Duration(0, 2000.));

    // Test creating a time interval directly from two absolute times.
    TimeInterval interval_a(time1, time2);

    // Test creating a time interval by subtracting two absolute times.
    TimeInterval interval_b = time2 - time1;

    // Compare the durations as computed in the TDB system.
    Duration dur_a = interval_a.computeElapsedTime("TDB").getTime();
    Duration dur_b = interval_b.computeElapsedTime("TDB").getTime();

    // Make sure they agree.
    if (dur_a != dur_b) {
      err() << "After creating interval_a and interval_b, they are not the same." << std::endl;
    }
  }

  void TestTimeRep() {
    s_os.setMethod("TestTimeRep");
    static const double epsilon = std::numeric_limits<double>::epsilon() * 10.;
    long mjd_ref_int = 51910;
    double mjd_ref_frac = 64.814 / 86400.;
    double met = 86400. * 100.;
    double delta_t = 100.;
    double expected_met = met;

    // Create a test time object.
    MetRep glast_tdb_object("TDB", mjd_ref_int, mjd_ref_frac, met);
    TimeRep & glast_tdb(glast_tdb_object);

    glast_tdb.get("", met);
    if (epsilon < std::fabs(met - expected_met)) {
      err() << "Right after construction, glast_tdb.get(\"\", met) returned " << met << ", not " << expected_met <<
        " as expected." << std::endl;
    }

    // Test construction directly from IntFracPair.
    MetRep int_frac_glast_tdb("TDB", IntFracPair(mjd_ref_int, mjd_ref_frac), met);

    int_frac_glast_tdb.get("", met);
    if (epsilon < std::fabs(met - expected_met)) {
      err() << "Right after construction, int_frac_glast_tdb.get(\"\", met) returned " << met << ", not " << expected_met <<
        " as expected." << std::endl;
    }

    // Create an absolute time from this time expression.
    AbsoluteTime abs_time(glast_tdb);

    // Compute something which changes this time.
    abs_time = abs_time + ElapsedTime("TDB", Duration(0, delta_t));

    // Put the changed time back into the representation.
    glast_tdb = abs_time;

    // Compare final value.
    expected_met = met + delta_t;
    glast_tdb.get("", met);
    if (epsilon < std::fabs(met - expected_met)) {
      err() << "After computation, glast_tdb.get(\"\", met) returned " << met << ", not " << expected_met <<
        " as expected." << std::endl;
    }

    // Test assignment from string.
    glast_tdb.assign("125.0123456");
    expected_met = 125.0123456;
    glast_tdb.get("", met);
    if (epsilon < std::fabs(met - expected_met)) {
      err() << "After assign(\"125.0123456\"), glast_tdb.get(\"\", met) returned " << met << ", not " << expected_met <<
        " as expected." << std::endl;
    }
    
    // Test set(string, double).
    glast_tdb.set("", 125.0123);
    expected_met = 125.0123;
    glast_tdb.get("", met);
    if (epsilon < std::fabs(met - expected_met)) {
      err() << "After set(\"\", 125.0123), glast_tdb.get(\"\", met) returned " << met << ", not " << expected_met <<
        " as expected." << std::endl;
    }
    
    // Test conversions to string.
    std::string expected_string = "125.0123 MET (TDB) [MJDREF=51910.000750162037037]";
    std::string glast_tdb_string = glast_tdb.getString();
    if (expected_string != glast_tdb_string) {
      err() << "glast_tdb.getString() returned string \"" << glast_tdb_string << "\", not \"" << expected_string <<
        "\", as expected." << std::endl;
    }

    // Create a test time object using MJD representation.
    MjdRep mjd_tdb_object("TDB", mjd_ref_int, mjd_ref_frac);
    TimeRep & mjd_tdb(mjd_tdb_object);

    IntFracPair expected_mjd(mjd_ref_int, mjd_ref_frac);
    long mjd_int = 0; double mjd_frac = 0.;
    mjd_tdb.get("MJDI", mjd_int); mjd_tdb.get("MJDF", mjd_frac);
    if (expected_mjd.getIntegerPart() != mjd_int ||
      epsilon < std::fabs(expected_mjd.getFractionalPart() - mjd_frac)) {
      err() << "Right after construction, mjd_tdb contains " << mjd_int << " + " << mjd_frac << ", not " << expected_mjd <<
        " as expected." << std::endl;
    }

    // Put the previous MET time back into the MJD representation.
    mjd_tdb = abs_time;

    // Compare final value.
    expected_mjd = IntFracPair(mjd_ref_int + 100, mjd_ref_frac + 100. / 86400.);
    mjd_int = 0; mjd_frac = 0.;
    mjd_tdb.get("MJDI", mjd_int); mjd_tdb.get("MJDF", mjd_frac);
    if (expected_mjd.getIntegerPart() != mjd_int ||
      epsilon < std::fabs(expected_mjd.getFractionalPart() - mjd_frac)) {
      err() << "After assignment from abs_time, mjd_tdb contains " << mjd_int << " + " << mjd_frac << ", not " << expected_mjd <<
        " as expected." << std::endl;
    }

    // Test assignment from string.
    mjd_tdb.assign("137.1250123456");
    expected_mjd = IntFracPair(137, .1250123456);
    mjd_int = 0; mjd_frac = 0.;
    mjd_tdb.get("MJDI", mjd_int); mjd_tdb.get("MJDF", mjd_frac);
    if (expected_mjd.getIntegerPart() != mjd_int ||
      epsilon < std::fabs(expected_mjd.getFractionalPart() - mjd_frac)) {
      err() << "After assign(\"137.1250123456\"), mjd_tdb contains " << mjd_int << " + " << mjd_frac << ", not " << expected_mjd <<
        " as expected." << std::endl;
    }
    
    // Test set for long, double.
    mjd_tdb.set("MJDI", 137l);
    mjd_tdb.set("MJDF", .1250123);
    expected_mjd = IntFracPair(137, .1250123);
    mjd_int = 0; mjd_frac = 0.;
    mjd_tdb.get("MJDI", mjd_int); mjd_tdb.get("MJDF", mjd_frac);
    if (expected_mjd.getIntegerPart() != mjd_int ||
      epsilon < std::fabs(expected_mjd.getFractionalPart() - mjd_frac)) {
      err() << "After set(\"MJDI\", 137); set(\"MJDF\", .1250123), mjd_tdb contains " << mjd_int << " + " << mjd_frac <<
        ", not " << expected_mjd << " as expected." << std::endl;
    }
    
    // Test conversions to string.
    expected_string = "137.1250123 MJD (TDB)";
    std::string mjd_tdb_string = mjd_tdb.getString();
    if (expected_string != mjd_tdb_string) {
      err() << "mjd_tdb.getString() returned string \"" << mjd_tdb_string << "\", not \"" << expected_string <<
        "\", as expected." << std::endl;
    }
  }

  void TestTimeFormat() {
    s_os.setMethod("TestTimeFormat");
    //static const double epsilon = std::numeric_limits<double>::epsilon() * 300.;
    moment_type expected_moment(51910, 64.814);
    long expected_mjd_int = expected_moment.first;
    double expected_mjd_frac = expected_moment.second * DayPerSec();

    // Create a test time object using MJD representation.
    const TimeFormat & time_format_mjd = TimeFormat::getFormat("MJD");
    const MjdFormat & mjd_format = MjdFormat::getMjdFormat();

    //long mjd_day = 0;
    //double mjd_sec = 0.;
    // Test conversion from integer part and fractional part to moment_type.
    moment_type moment(0, 0.);
    mjd_format.convert(expected_mjd_int, expected_mjd_frac, moment);
    double tolerance = 100.e-9; // 100 nano-seconds.
    if (expected_moment.first != moment.first || tolerance < std::fabs(expected_moment.second - moment.second)) {
      err() << "Object mjd_format converted (" << expected_mjd_int << " + " << expected_mjd_frac <<
        ") MJD into moment_type pair (" << moment.first << ", " << moment.second << "), not (" <<
        expected_moment.first << ", " << expected_moment.second << ") as expected." << std::endl;
    }

    // Test conversion from moment_type to integer part and fractional part.
    long mjd_int = 0;
    double mjd_frac = 0.;
    mjd_format.convert(expected_moment, mjd_int, mjd_frac);
    tolerance = 100.e-9 * DayPerSec(); // 100 nano-seconds in units of day.
    if (expected_mjd_int != mjd_int || tolerance < std::fabs(expected_mjd_frac - mjd_frac)) {
      err() << "Object mjd_format converted moment_type pair (" << expected_moment.first << ", " << expected_moment.second <<
        ") into (" << expected_mjd_int << " + " << expected_mjd_frac << ") MJD, not (" << expected_mjd_int << " + " <<
        expected_mjd_frac << ") MJD as expected." << std::endl;
    }

    // Test formatting into string.
    std::string expected_mjd_string = "51910.000750162037037 MJD";
    std::string mjd_string = time_format_mjd.format(expected_moment);
    if (expected_mjd_string != mjd_string) {
      err() << "Reference to mjd_format formatted moment_type pair (" << expected_moment.first << ", " << expected_moment.second <<
        ") into \"" << mjd_string << "\", not \"" << expected_mjd_string << "\" as expected." << std::endl;
    }

    // Test formatting into string, with decimal precision specified.
    expected_mjd_string = "51910.0007502 MJD";
    mjd_string = time_format_mjd.format(expected_moment, 7);
    if (expected_mjd_string != mjd_string) {
      err() << "Reference to mjd_format formatted moment_type pair (" << expected_moment.first << ", " << expected_moment.second <<
        ") into \"" << mjd_string << "\", not \"" << expected_mjd_string << "\" as expected." << std::endl;
    }

    // Test parsing a string.
    mjd_string = "51910.000750162037037";
    moment = time_format_mjd.parse(mjd_string);
    tolerance = 100.e-9; // 100 nano-seconds.
    if (expected_moment.first != moment.first || tolerance < std::fabs(expected_moment.second - moment.second)) {
      err() << "Reference to mjd_format parsed \"" << mjd_string << "\" into moment_type pair (" << moment.first <<
        ", " << moment.second << "), not (" << expected_moment.first << ", " << expected_moment.second << ") as expected." <<
        std::endl;
    }

    // Put the previous MET time back into the MJD representation.
#if 0
    // TODO use appropriate new method to assign or construct MjdFormat from AbsoluteTime.
    mjd_format = abs_time;

    // Compare final value.
    mjd_day = 0;
    mjd_sec = 0.;
    mjd_format.get(mjd_day, mjd_sec);
    if (expected_mjd_day != mjd_day || epsilon < std::fabs(expected_mjd_sec - mjd_sec)) {
      err() << "After assignment from abs_time, mjd_format contains " << mjd_day << " days, " << mjd_sec <<
        " seconds, not " << expected_mjd_day << " days, " << expected_mjd_sec << " seconds as expected." << std::endl;
    }
#endif

  }

  void TestField() {
    s_os.setMethod("TestField");
    Field<double> d_field("Year", 2000.1);

    double d_value = 0.;
    d_field.get(d_value);
    if (2000.1 != d_value) err() << "Field<double>::get(double &) returned " << d_value << ", not 2000.1, as expected" << std::endl;

    long l_value = 0;
    d_field.get(l_value);
    if (2000 != l_value) err() << "Field<double>::get(long &) returned " << l_value << ", not 2000, as expected" << std::endl;

    Field<long> l_field("Year", 2000);

    d_value = 0.;
    l_field.get(d_value);
    if (2000. != d_value) err() << "Field<long>::get(double &) returned " << d_value << ", not 2000., as expected" << std::endl;

    l_value = 0;
    l_field.get(l_value);
    if (2000 != l_value) err() << "Field<long>::get(long &) returned " << l_value << ", not 2000, as expected" << std::endl;
  }

  void TestBaryTimeComputer() {
    s_os.setMethod("TestBaryTimeComputer");

    // Get a barycentric time computer.
    BaryTimeComputer & computer = BaryTimeComputer::getComputer();

    // Prepare a time to be barycentered and an expected result after barycentered.
    GlastMetRep glast_met("TT", 2.123393677090199E+08); // TSTART in my_pulsar_events_v3.fits.
    AbsoluteTime original = glast_met;
    AbsoluteTime result = original;
    glast_met = GlastMetRep("TDB", 2.123393824137859E+08); // TSTART in my_pulsar_events_bary_v3.fits.
    AbsoluteTime expected = glast_met;

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
        const std::string & /*sc_file_name*/, const std::string & /*sc_extension_name*/, const double /*angular_tolerance*/,
        const bool /*read_only*/ = true)
        { return 0; }

    protected:
      virtual AbsoluteTime readTime(const tip::Header & /*header*/, const std::string & /*keyword_name*/,
        const bool /*request_bary_time*/, const double /*ra*/, const double /*dec*/)
        { return AbsoluteTime("TDB", Duration(51910, 0.), Duration(1, 0.)); }

      virtual AbsoluteTime readTime(const tip::TableRecord & /*record*/, const std::string & /*column_name*/,
        const bool /*request_bary_time*/, const double /*ra*/, const double /*dec*/)
        { return AbsoluteTime("TDB", Duration(51910, 0.), Duration(1, 0.)); }

    private:
      BogusTimeHandler1(const std::string & file_name, const std::string & extension_name):
      EventTimeHandler(file_name, extension_name, 0.) {}
  };

  class BogusTimeHandler2: public EventTimeHandler {
    public:
      virtual ~BogusTimeHandler2() {}

      static EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name,
        const std::string & /*sc_file_name*/, const std::string & /*sc_extension_name*/, const double angular_tolerance,
        const bool read_only = true)
        { return new BogusTimeHandler2(file_name, extension_name, angular_tolerance, read_only); }

    protected:
      virtual AbsoluteTime readTime(const tip::Header & /*header*/, const std::string & /*keyword_name*/,
        const bool /*request_bary_time*/, const double /*ra*/, const double /*dec*/)
        { return AbsoluteTime("TDB", Duration(51910, 0.), Duration(2, 0.)); }

      virtual AbsoluteTime readTime(const tip::TableRecord & /*record*/, const std::string & /*column_name*/,
        const bool /*request_bary_time*/, const double /*ra*/, const double /*dec*/)
        { return AbsoluteTime("TDB", Duration(51910, 0.), Duration(2, 0.)); }

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
    std::string sc_file = commonUtilities::joinPath(commonUtilities::getDataPath("timeSystem"), "my_pulsar_spacecraft_data_v3r1.fits");
    GlastMetRep glast_met("TT", 2.123393677090199E+08); // TSTART in my_pulsar_events_v3.fits.
    AbsoluteTime expected_glast = glast_met;
    AbsoluteTime expected_bogus2 = AbsoluteTime("TDB", Duration(51910, 0.), Duration(2, 0.));
    double angular_tolerance = 0.;

    // Test creation of BogusTimeHandler1 (an EventTimeHandler sub-class) through its createInstance method.
    std::auto_ptr<EventTimeHandler> handler(0);
    handler.reset(BogusTimeHandler1::createInstance(event_file, "EVENTS", sc_file, "SC_DATA", angular_tolerance));
    if (handler.get() != 0) {
      err() << "BogusTimeHandler1::createInstance method did not return a null pointer (0)." << std::endl;
    }

    // Test creation of BogusTimeHandler2 (an EventTimeHandler sub-class) through its createInstance method.
    handler.reset(BogusTimeHandler2::createInstance(event_file, "EVENTS", sc_file, "SC_DATA", angular_tolerance));
    AbsoluteTime result = handler->readHeader("TSTART");
    if (!result.equivalentTo(expected_bogus2, time_tolerance)) {
      err() << "BogusTimeHandler2::createInstance method did not return a BogusTimeHandler2 object." << std::endl;
    }

    // Test creation of GlastTimeHandler (an EventTimeHandler sub-class) through its createInstance method.
    handler.reset(GlastTimeHandler::createInstance(event_file, "EVENTS", sc_file, "SC_DATA", angular_tolerance));
    result = handler->readHeader("TSTART");
    if (!result.equivalentTo(expected_glast, time_tolerance)) {
      err() << "GlastTimeHandler::createInstance method did not return a GlastTimeHandler object." << std::endl;
    }

    // Test the decision-making mechanism for cases without a prior setup.
    try {
      handler.reset(IEventTimeHandlerFactory::createHandler(event_file, "EVENTS", sc_file, "SC_DATA", angular_tolerance));
      err() << "IEventTimeHandlerFactory::createHandler method did not throw an exception when no handler was registered." << std::endl;
    } catch (const std::exception &) {
    }

    // Register BogusTimeHandler1 to EventTimeHandlerFactory.
    EventTimeHandlerFactory<BogusTimeHandler1> factory1;

    // Test the decision-making mechanism for cases with only BogusTimeHandler1 registered.
    try {
      handler.reset(IEventTimeHandlerFactory::createHandler(event_file, "EVENTS", sc_file, "SC_DATA", angular_tolerance));
      err() << "IEventTimeHandlerFactory::createHandler method did not throw an exception when only BogusTimeHandler1 was registered."
        << std::endl;
    } catch (const std::exception &) {
    }

    // Register GlastTimeHandler to EventTimeHandlerFactory.
    EventTimeHandlerFactory<GlastTimeHandler> factory2;

    // Test the decision-making mechanism for cases with BogusTimeHandler1 and GlastTimeHandler registered.
    handler.reset(IEventTimeHandlerFactory::createHandler(event_file, "EVENTS", sc_file, "SC_DATA", angular_tolerance));
    result = handler->readHeader("TSTART");
    if (!result.equivalentTo(expected_glast, time_tolerance)) {
      err() << "IEventTimeHandlerFactory::createHandler method did not return a GlastTimeHandler object" <<
        " when it is the only appropriate handler." << std::endl;
    }

    // Register GlastTimeHandler to EventTimeHandlerFactory.
    EventTimeHandlerFactory<BogusTimeHandler2> factory3;

    // Test the decision-making mechanism for cases with BogusTimeHandler1, GlastTimeHandler, and BogusTimeHandler2 registered.
    handler.reset(IEventTimeHandlerFactory::createHandler(event_file, "EVENTS", sc_file, "SC_DATA", angular_tolerance));
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
    handler.reset(IEventTimeHandlerFactory::createHandler(event_file, "EVENTS", sc_file, "SC_DATA", angular_tolerance));
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
    handler.reset(GlastTimeHandler::createInstance(event_file, "EVENTS", sc_file, "SC_DATA", 0.));

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

    // Test reading header keyword value.
    AbsoluteTime result = handler->readHeader("TSTART");
    GlastMetRep glast_met("TT", 2.123393677090199E+08); // TSTART in my_pulsar_events_v3.fits.
    AbsoluteTime expected = glast_met;
    if (!result.equivalentTo(expected, time_tolerance)) {
      err() << "GlastTimeHandler::readHeader(\"TSTART\") returned AbsoluteTime(" << result << "), not equivalent to AbsoluteTime(" <<
        expected << ") with tolerance of " << time_tolerance << "." << std::endl;
    }

    // Test reading header keyword value, requesting barycentering.
    result = handler->readHeader("TSTART", ra, dec);
    glast_met = GlastMetRep("TDB", 2.123393824137859E+08); // TSTART in my_pulsar_events_bary_v3.fits.
    expected = glast_met;
    if (!result.equivalentTo(expected, time_tolerance)) {
      err() << "GlastTimeHandler::readHeader(\"TSTART\", " << ra << ", " << dec << ") returned AbsoluteTime(" << result <<
        "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
    }

    // Test reading TIME column value.
    handler->setFirstRecord(); // Points to the first event.
    handler->setNextRecord();  // Points to the second event.
    handler->setNextRecord();  // Points to the third event.
    result = handler->readColumn("TIME");
    glast_met = GlastMetRep("TT", 2.123393750454886E+08); // TIME of the third row in my_pulsar_events_v3.fits.
    expected = glast_met;
    if (!result.equivalentTo(expected, time_tolerance)) {
      err() << "GlastTimeHandler::readColumn(\"TIME\", " << ra << ", " << dec << ") returned AbsoluteTime(" << result <<
        "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
    }

    // Test reading TIME column value, requesting barycentering.
    handler->setFirstRecord(); // Re-setting to the first event.
    handler->setNextRecord();  // Points to the second event.
    handler->setNextRecord();  // Points to the third event.
    result = handler->readColumn("TIME", ra, dec);
    glast_met = GlastMetRep("TDB", 2.123393897503012E+08); // TIME of the third row in my_pulsar_events_bary_v3.fits.
    expected = glast_met;
    if (!result.equivalentTo(expected, time_tolerance)) {
      err() << "GlastTimeHandler::readColumn(\"TIME\", " << ra << ", " << dec << ") returned AbsoluteTime(" << result <<
        "), not equivalent to AbsoluteTime(" << expected << ") with tolerance of " << time_tolerance << "." << std::endl;
    }

    // Create an GlastTimeHandler object for EVENTS extension of a barycentered event file.
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "EVENTS", sc_file, "SC_DATA", angular_tolerance));

    // Test reading header keyword value, requesting barycentering.
    result = handler->readHeader("TSTART", ra, dec);
    glast_met = GlastMetRep("TDB", 2.123393824137859E+08); // TSTART in my_pulsar_events_bary_v3.fits.
    expected = glast_met;
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
    glast_met = GlastMetRep("TDB", 2.123393897503012E+08); // TIME of the third row in my_pulsar_events_bary_v3.fits.
    expected = glast_met;
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
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "EVENTS", sc_file, "SC_DATA", angular_tolerance));
    try {
      handler->checkSkyPosition(ra, dec);
    } catch (const std::exception &) {
      err() << "GlastTimeHandler::checkSkyPosition(" << ra << ", " << dec << 
        ") threw an exception with angular tolerance of zero (0) degree." << std::endl;
    }

    // Test large angular tolerance of 180 degrees.
    angular_tolerance = 180.;
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "EVENTS", sc_file, "SC_DATA", angular_tolerance));
    try {
      handler->checkSkyPosition(ra_wrong, dec_wrong);
    } catch (const std::exception &) {
      err() << "GlastTimeHandler::checkSkyPosition(" << ra_wrong << ", " << dec_wrong << 
        ") threw an exception with angular tolerance of 180 degrees." << std::endl;
    }

    // Test large angular difference, with small angular tolerance.
    angular_tolerance = 1.e-8;
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "EVENTS", sc_file, "SC_DATA", angular_tolerance));
    try {
      handler->checkSkyPosition(ra_opposite, dec_opposite);
      err() << "GlastTimeHandler::checkSkyPosition(\"TSTART\", " << ra_opposite << ", " << dec_opposite << 
        ") did not throw an exception with angular tolerance of zero (0) degrees." << std::endl;
    } catch (const std::exception &) {
    }

    // Test large angular difference, with large angular tolerance of 180 degrees.
    angular_tolerance = 180.;
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "EVENTS", sc_file, "SC_DATA", angular_tolerance));
    try {
      handler->checkSkyPosition(ra_opposite, dec_opposite);
    } catch (const std::exception &) {
      err() << "GlastTimeHandler::checkSkyPosition(" << ra_opposite << ", " << dec_opposite << 
        ") threw an exception with angular tolerance of 180 degrees." << std::endl;
    }

    // Test checking solar system ephemeris name, with a non-barycentered event extension.
    angular_tolerance = 1.e-8;
    handler.reset(GlastTimeHandler::createInstance(event_file, "EVENTS", sc_file, "SC_DATA", angular_tolerance));
    try {
      handler->checkSolarEph("Bogus Name");
    } catch (const std::exception &) {
      err() << "GlastTimeHandler::checkSolarEph(\"Bogus Name\") threw an exception for non-barycentered event extension." << std::endl;
    }

    // Test checking solar system ephemeris name, with a barycentered event extension (exact match).
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "EVENTS", sc_file, "SC_DATA", angular_tolerance));
    try {
      handler->checkSolarEph("JPL-DE405");
    } catch (const std::exception &) {
      err() << "GlastTimeHandler::checkSolarEph(\"JPL-DE405\") threw an exception for a barycentered event extension." << std::endl;
    }

    // Test checking solar system ephemeris name, with a barycentered event extension (rough match).
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "EVENTS", sc_file, "SC_DATA", angular_tolerance));
    try {
      handler->checkSolarEph("JPL DE405");
    } catch (const std::exception &) {
      err() << "GlastTimeHandler::checkSolarEph(\"JPL DE405\") threw an exception for a barycentered event extension." << std::endl;
    }

    // Test checking solar system ephemeris name, with a barycentered event extension (no match).
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "EVENTS", sc_file, "SC_DATA", angular_tolerance));
    try {
      handler->checkSolarEph("JPL DE200");
      err() << "GlastTimeHandler::checkSolarEph(\"JPL DE200\") did not throw an exception for a barycentered event extension." <<
        std::endl;
    } catch (const std::exception &) {
    }

    // Test checking solar system ephemeris name, with a barycentered GTI extension (rough match).
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "GTI", sc_file, "SC_DATA", angular_tolerance));
    try {
      handler->checkSolarEph("JPL DE405");
    } catch (const std::exception &) {
      err() << "GlastTimeHandler::checkSolarEph(\"JPL DE405\") threw an exception for a barycentered GTI extension." << std::endl;
    }

    // Test checking solar system ephemeris name, with a barycentered GTI extension (no match).
    handler.reset(GlastTimeHandler::createInstance(event_file_bary, "GTI", sc_file, "SC_DATA", angular_tolerance));
    try {
      handler->checkSolarEph("JPL DE200");
      err() << "GlastTimeHandler::checkSolarEph(\"JPL DE200\") did not throw an exception for a barycentered GTI extension." <<
        std::endl;
    } catch (const std::exception &) {
    }

  }
}

StAppFactory<TestTimeSystemApp> g_factory("test_timeSystem");
