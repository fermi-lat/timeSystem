/** \file test_timeSystem.h
    \brief Unit test for timeSystem package.
    \author Masa Hirayama, James Peachey
*/

#include "st_app/StApp.h"
#include "st_app/StAppFactory.h"

#include "st_stream/st_stream.h"
#include "st_stream/StreamFormatter.h"

#include "timeSystem/AbsoluteTime.h"
#include "timeSystem/ElapsedTime.h"
#include "timeSystem/Duration.h"
#include "timeSystem/TimeInterval.h"
#include "timeSystem/TimeSystem.h"
#include "timeSystem/TimeValue.h"

#include <cmath>
#include <exception>
#include <limits>

namespace {

  bool s_failed = false;
  st_stream::StreamFormatter s_os("test_timeSystem", "", 2);

  void TestDuration();

  void TestTimeSystem();

  void TestAbsoluteTime();

  void TestElapsedTime();

  void TestTimeInterval();

  void TestTimeValue();
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

  // Test TimeValue class.
  TestTimeValue();

  // Interpret failure flag to report error.
  if (s_failed) throw std::runtime_error("Unit test failure");
}

namespace {

  st_stream::OStream & err() {
    s_failed = true;
    return s_os.err() << prefix;
  }

  void TestDuration() {
    s_os.setMethod("TestDuration");

    TimeValue time_value;

    // Tests of Duration::day() and sec() methods for positive Durations.
    Duration six_days(6);
    time_value = six_days.day();
    if (!(6 == time_value.getIntegerPart() && 0. == time_value.getFractionalPart())) {
      err() << "After Duration six_days(6), six_days.day() returned (" << time_value.getIntegerPart() <<
	", " << time_value.getFractionalPart() << "), not (6, 0.) as expected." << std::endl;
    }
    time_value = six_days.sec();
    if (!(6 * 86400 == time_value.getIntegerPart() && 0. == time_value.getFractionalPart())) {
      err() << "After Duration six_days(6), six_days.sec() returned (" << time_value.getIntegerPart() <<
        ", " << time_value.getFractionalPart() << "), not " << 6 * 86400 << ", 0.) as expected." << std::endl;
    }

    Duration six_sec(0, 6.);
    double nano_sec = 1.e-9 / 86400.;
    time_value = six_sec.day();
    if (!(fabs(6. / 86400. - time_value.getFractionalPart()) < nano_sec && 0 == time_value.getIntegerPart())) {
      err() << "After Duration six_sec(0, 6.), six_sec.day() returned (" << time_value.getIntegerPart() <<
        ", " << time_value.getFractionalPart() << "), not (0, " << 6. / 86400. << ") as expected." << std::endl;
    }
    time_value = six_sec.sec();
    if (!(6 == time_value.getIntegerPart() && 0. == time_value.getFractionalPart())) {
      err() << "After Duration six_sec(0, 6.), six_sec.sec() returned (" << time_value.getIntegerPart() <<
        ", " << time_value.getFractionalPart() << "), not (6, 0.) as expected." << std::endl;
    }

    // Tests of Duration::day() and sec() methods for negative Durations.
    Duration neg_six_days(-6);
    time_value = neg_six_days.day();
    if (!(-6 == time_value.getIntegerPart() && 0. == time_value.getFractionalPart())) {
      err() << "After Duration neg_six_days(-6), neg_six_days.day() returned (" << time_value.getIntegerPart() <<
	", " << time_value.getFractionalPart() << "), not (-6, 0.) as expected." << std::endl;
    }
    time_value = neg_six_days.sec();
    if (!(-6 * 86400 == time_value.getIntegerPart() && 0. == time_value.getFractionalPart())) {
      err() << "After Duration neg_six_days(-6), neg_six_days.sec() returned (" << time_value.getIntegerPart() <<
        ", " << time_value.getFractionalPart() << "), not " << -6 * 86400 << ", 0.) as expected." << std::endl;
    }

    Duration neg_six_sec(0, -6.);
    time_value = neg_six_sec.day();
    if (!(fabs(-6. / 86400. - time_value.getFractionalPart()) < nano_sec && 0 == time_value.getIntegerPart())) {
      err() << "After Duration neg_six_sec(0, -6.), neg_six_sec.day() returned (" << time_value.getIntegerPart() <<
        ", " << time_value.getFractionalPart() << "), not (0, " << -6. / 86400. << ") as expected." << std::endl;
    }
    time_value = neg_six_sec.sec();
    if (!(-6 == time_value.getIntegerPart() && 0. == time_value.getFractionalPart())) {
      err() << "After Duration neg_six_sec(0, -6.), neg_six_sec.sec() returned (" << time_value.getIntegerPart() <<
        ", " << time_value.getFractionalPart() << "), not (-6, 0.) as expected." << std::endl;
    }

    // Tests of equality and inequality operators.
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
        " with tolerance of " << tight_tol << " not false as expected." << std::endl;
    if (seven_sec.equivalentTo(about_seven, tight_tol))
      err() << "After Duration seven_sec(0, 7.), seven_sec.equivalentTo returned true for " << about_seven <<
        " with tolerance of " << tight_tol << " not false as expected." << std::endl;

    // Make comparisons which succeed.
    Duration loose_tol(0, .1);
    if (!about_seven.equivalentTo(seven_sec, loose_tol))
      err() << "After Duration about_seven(0, 7.1), about_seven.equivalentTo returned false for " << seven_sec <<
        " with tolerance of " << loose_tol << " not true as expected." << std::endl;
    if (!seven_sec.equivalentTo(about_seven, loose_tol))
      err() << "After Duration seven_sec(0, 7.1), seven_sec.equivalentTo returned false for " << about_seven <<
        " with tolerance of " << loose_tol << " not true as expected." << std::endl;

    // Tests for detection of overflow/underflow:
    // A case which should *not* overflow, but is close to overflowing.
    try {
      Duration sec_max(0, std::numeric_limits<long>::max() + .4);
      sec_max.sec();
    } catch (const std::exception & x) {
      err() << "A test which should not overflow unexpectedly caught: " << x.what() << std::endl;
    }
    // A case which should *not* underflow, but is close to underflowing.
    try {
      Duration sec_min(0, std::numeric_limits<long>::min() - .4);
      sec_min.sec();
    } catch (const std::exception & x) {
      err() << "A test which should not underflow unexpectedly caught: " << x.what() << std::endl;
    }
    // A case which should overflow.
    double overflow_sec = std::numeric_limits<long>::max() + 1.1;
    try {
      Duration sec_max(0, overflow_sec);
      sec_max.sec();
      err() << "Duration::sec method unexpectedly did not overflow for " << overflow_sec << " seconds." << std::endl;
    } catch (const std::exception &) {
    }
    // A case which should underflow.
    double underflow_sec = std::numeric_limits<long>::min() - 1.1;
    try {
      Duration sec_min(0, underflow_sec);
      sec_min.sec();
      err() << "Duration::sec method unexpectedly did not underflow for " << underflow_sec << " seconds." << std::endl;
    } catch (const std::exception &) {
    }

    // TODO test all Duration comparison operators and all other math.
  }

  void TestOneConversion(const std::string & src_name, const Duration & src_origin, const Duration & src,
    const std::string & dest_name, const Duration & expected_dest, double tolerance = 1.e-9) {
    s_os.setMethod("TestOneConversion");
    const TimeSystem & src_sys(TimeSystem::getSystem(src_name));
    const TimeSystem & dest_sys(TimeSystem::getSystem(dest_name));
    Duration dest = dest_sys.convertFrom(src_sys, src_origin, src);
    if (!dest.equivalentTo(expected_dest, Duration(0, tolerance))) {
      err() << "Result of converting from " << src << " " << src_sys << " to " << dest_sys << " was " <<
        dest << ", not " << expected_dest << " as expected." << std::endl;
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
    TestOneConversion("TAI", Duration(51910, 0.), tai_ref_time, "TAI", tai_ref_time);
    TestOneConversion("TDB", Duration(51910, 0.), tdb_ref_time, "TDB", tdb_ref_time);
    TestOneConversion("TT", Duration(51910, 0.), tt_ref_time, "TT", tt_ref_time);
    TestOneConversion("UTC", Duration(51910, 0.), utc_ref_time, "UTC", utc_ref_time);

    // Test conversions from TAI to...
    TestOneConversion("TAI", Duration(51910, 0.), tai_ref_time, "TDB", tdb_ref_time, tdb_tolerance);
    TestOneConversion("TAI", Duration(51910, 0.), tai_ref_time, "TT", tt_ref_time);
    TestOneConversion("TAI", Duration(51910, 0.), tai_ref_time, "UTC", utc_ref_time);

    // Test conversions from TDB to...
    TestOneConversion("TDB", Duration(51910, 0.), tdb_ref_time, "TAI", tai_ref_time, tdb_tolerance);
    TestOneConversion("TDB", Duration(51910, 0.), tdb_ref_time, "TT", tt_ref_time, tdb_tolerance);
    TestOneConversion("TDB", Duration(51910, 0.), tdb_ref_time, "UTC", utc_ref_time, tdb_tolerance);

    // Test conversions from TT to...
    TestOneConversion("TT", Duration(51910, 0.), tt_ref_time, "TAI", tai_ref_time);
    TestOneConversion("TT", Duration(51910, 0.), tt_ref_time, "TDB", tdb_ref_time, tdb_tolerance);
    TestOneConversion("TT", Duration(51910, 0.), tt_ref_time, "UTC", utc_ref_time);

    // Test conversions from UTC to...
    TestOneConversion("UTC", Duration(51910, 0.), utc_ref_time, "TAI", tai_ref_time);
    TestOneConversion("UTC", Duration(51910, 0.), utc_ref_time, "TDB", tdb_ref_time, tdb_tolerance);
    TestOneConversion("UTC", Duration(51910, 0.), utc_ref_time, "TT", tt_ref_time);

    // Use three leap seconds for generating tests.
    double diff0 = 31.;
    double diff1 = 32.;
    // double diff2 = 33.;

    // Use times for three leap seconds for generating tests.
    // long leap0 = 50630;
    long leap1 = 51179;
    long leap2 = 53736;
    long delta_leap = leap2 - leap1;

    // To ensure UTC->TAI is handled correctly, do some tougher conversions, i.e. times which are close to
    // times when leap seconds are inserted.
    TestOneConversion("UTC", Duration(leap1, 0.), Duration(0, 0.), "TAI", Duration(0, diff1));
    TestOneConversion("UTC", Duration(leap1, -.001), Duration(0, 0.), "TAI", Duration(0, diff0));
    // Although the total time (origin + elapsed) is large enough to cross two leap second boundaries, still
    // the earliest leap second should be used because the choice of leap second is based only on the origin time.
    TestOneConversion("UTC", Duration(leap1, -.001), Duration(delta_leap, 2.002), "TAI", Duration(delta_leap, diff0 + 2.002));

    // To ensure TAI->UTC is handled correctly, do some tougher conversions, i.e. times which are close to
    // times when leap seconds are inserted.
    TestOneConversion("TAI", Duration(leap1, diff1), Duration(0, 0.), "UTC", Duration(0, -diff1));
    TestOneConversion("TAI", Duration(leap1, diff1 - .001), Duration(0, 0.), "UTC", Duration(0, -diff0));
    // Although the total time (origin + elapsed) is large enough to cross two leap second boundaries, still
    // the earliest leap second should be used because the choice of leap second is based only on the origin time.
    TestOneConversion("TAI", Duration(leap1, diff1 - .001), Duration(delta_leap, 2.002), "UTC",
      Duration(delta_leap, -diff0 + 2.002));

    // Test that conversion uses table keyed by TAI times, not by UTC.
    TestOneConversion("TAI", Duration(leap1, -2.), Duration(1, 0.), "UTC", Duration(1, -diff1 + 1.));

    // Test case before first time covered by the current UTC definition. This is "undefined" in the current scheme.
    try {
      TestOneConversion("UTC", Duration(0, 0.), Duration(0, 0.), "TAI", Duration(0, 0.));
      err() << "Conversion of time 0. MJD UTC to TAI did not throw an exception." << std::endl;
    } catch (const std::exception &) {
      // That's OK!
    }
  }

  void TestAbsoluteTime() {
    s_os.setMethod("TestAbsoluteTime");

    Duration mjd_origin(51910);
    Duration duration(0, 1000.); // MET 1000. seconds

    // Create an absolute time corresponding to MET 1000. s.
    AbsoluteTime abs_time("TDB", mjd_origin, duration);

    // Test adding an elapsed time to this time.
    // Create an absolute time corresponding to 100. s MET TDB to verify adding an elapsed time.
    Duration delta_t(0, 100.);
    ElapsedTime elapsed_time("TDB", delta_t);
    AbsoluteTime result = abs_time + elapsed_time;
    AbsoluteTime expected_result("TDB", mjd_origin, duration + delta_t);
    Duration difference = (result - expected_result).computeElapsedTime("TDB").getTime();
    Duration expected_diff(0, 0.);
    if (difference != expected_diff) {
      err() << "Sum of absolute time and elapsed time's duration was " << result.getTime() << ", not " <<
        expected_result.getTime() << " as expected." << std::endl;
    }

    // Test adding in reverse order.
    result = elapsed_time + abs_time;
    difference = (result - expected_result).computeElapsedTime("TDB").getTime();

    if (difference != expected_diff) {
      err() << "Sum of elapsed time and absolute time's duration was " << result.getTime() << ", not " <<
        expected_result.getTime() << " as expected." << std::endl;
    }

    // Test subtraction of elapsed time from absolute time.
    result = abs_time - elapsed_time;
    expected_result = AbsoluteTime("TDB", mjd_origin, duration - delta_t);
    difference = (result - expected_result).computeElapsedTime("TDB").getTime();
    if (difference != expected_diff) {
      err() << "Elapsed time subtracted from absolute time gave " << result.getTime() << ", not " <<
        expected_result.getTime() << " as expected." << std::endl;
    }

    // Make a test time which is later than the first time.
    AbsoluteTime later_time("TDB", mjd_origin, duration + Duration(0, 100.));

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
  }

  void TestElapsedTime() {
    s_os.setMethod("TestElapsedTime");

    // TODO Check the contents somehow?
    ElapsedTime elapsed("TDB", Duration(0, 1000.));

    // TODO Finish this test of unary minus by verifying equality?
    ElapsedTime negative_elapsed = -elapsed;
    ElapsedTime negative_elapsed_expected("TDB", Duration(0, -1000.));
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

  static void CompareTimeValue(const std::string & hint, const TimeValue & value, long expected_int_part,
    double expected_frac_part) {
    if (expected_int_part != value.getIntegerPart()) {
      err() << hint << ", integer part of time value was " <<
        value.getIntegerPart() << ", not " << expected_int_part << " as expected." << std::endl;
    }

    if (expected_frac_part != value.getFractionalPart()) {
      err() << hint << ", fractional part of time value was " <<
        value.getFractionalPart() << ", not " << expected_frac_part << " as expected." << std::endl;
    }
  }

  void TestTimeValue() {
    s_os.setMethod("TestTimeValue");
    long int_part = 100;
    double frac_part = .56789567895678956789;

    // Test construction from separate int and frac parts.
    TimeValue tv(int_part, frac_part);

    std::ostringstream os;
    os.precision(s_os.err().precision());
    os << "After TimeValue tv(" << int_part << ", " << frac_part << ")";
    CompareTimeValue(os.str(), tv, int_part, frac_part);

    // Make sure this value is rounded off in the expected way when converted to a mere double.
    double expected_dval = 100.567895678957;
    double dval = tv.reduceToDouble();
    if (10. * std::numeric_limits<double>::epsilon() < std::fabs((dval - expected_dval) / expected_dval)) {
      err() << os.str() << ", tv.reduceToDouble() returned " << dval << ", not " << expected_dval << " as expected." << std::endl;
    }

    // Test construction from a double, making sure the separate int and fractional parts are as expected.
    dval = 56789.56789567895678956789;
    tv = TimeValue(dval);
    os.str("");
    os << "After tv = TimeValue(" << dval << ")";
    CompareTimeValue(os.str(), tv, 56789, .567895678900000);
  }

}

StAppFactory<TestTimeSystemApp> g_factory("test_timeSystem");
