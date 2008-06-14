/** \file TimeSystem.cxx
    \brief Implementation of TimeSystem class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "st_facilities/Env.h"

#include "timeSystem/Duration.h"
#include "timeSystem/IntFracPair.h"
#include "timeSystem/TimeConstant.h"
#include "timeSystem/TimeSystem.h"

#include "tip/IFileSvc.h"
#include "tip/Table.h"

extern "C" {
#include "bary.h"
}

#include <cctype>
#include <memory>
#include <sstream>
#include <stdexcept>

using namespace timeSystem;

std::string timeSystem::TimeSystem::s_default_leap_sec_file;

namespace {

  class TaiSystem : public TimeSystem {
    public:
      TaiSystem(): TimeSystem("TAI") {}

      virtual moment_type convertFrom(const TimeSystem & time_system, const moment_type & moment) const;
  };

  class TdbSystem : public TimeSystem {
    public:
      TdbSystem(): TimeSystem("TDB") {}

      virtual moment_type convertFrom(const TimeSystem & time_system, const moment_type & moment) const;
  };

  class TtSystem : public TimeSystem {
    public:
      TtSystem(): TimeSystem("TT") {}

      virtual moment_type convertFrom(const TimeSystem & time_system, const moment_type & moment) const;
  };

  class UtcSystem : public TimeSystem {
    public:
      UtcSystem(): TimeSystem("UTC") {}

      virtual moment_type convertFrom(const TimeSystem & time_system, const moment_type & moment) const;

      virtual Duration computeTimeDifference(const moment_type & moment1, const moment_type & moment2) const;

      virtual datetime_type computeDateTime(const moment_type & moment) const;

      void checkMoment(const moment_type & moment) const;
  };

  class LeapSecTable {
    public:
      static LeapSecTable & getTable();

      std::string getFileName() const;

      void load(const std::string & leap_sec_file_name, bool force_load);

      long getCumulativeLeapSec(long mjd) const;

      long getEarliestMjd() const;

    private:
      typedef std::map<long, long> table_type;
      // A value of this map is the cumulative number of leap seconds since the introduction of leap seconds
      // at the beginning of the date whose MJD in UTC is given by the key of the std::map object.
      table_type m_table;

      std::string m_file_name;

      LeapSecTable() {}
  };

  Duration computeTtMinusTai() {
    static const Duration s_tt_minus_tai(0, 32.184);
    return s_tt_minus_tai;
  }

  moment_type TaiSystem::convertFrom(const TimeSystem & time_system, const moment_type & moment) const {
    if (&time_system != this) {
      if ("TDB" == time_system.getName()) {
        const TimeSystem & tt(getSystem("TT"));
        return convertFrom(tt, tt.convertFrom(time_system, moment));

      } else if ("TT" == time_system.getName()) {
        return moment_type(moment.first, moment.second - computeTtMinusTai());

      } else if ("UTC" == time_system.getName()) {
        // Check whether the given moment is valid in the current UTC system.
        time_system.checkMoment(moment);

        // Compute TAI - UTC in seconds.
        // Note: The given moment must be interpreted as is, so that the leap-second table is properly looked up.
        const LeapSecTable & leap_sec_table(LeapSecTable::getTable());
        long tai_minus_utc = 10 + leap_sec_table.getCumulativeLeapSec(moment.first);

        // Add the TAI - UTC to the given moment in TAI system.
        return moment_type(moment.first, moment.second + Duration(0, tai_minus_utc));

      } else {
        throw std::logic_error("Conversion from " + time_system.getName() + " to " + getName() + " is not implemented");
      }
    }
    return moment;
  }

  // Note: Keep computeTdbMinusTt method global, so that TdbSystem and TtSystem can call it.
  Duration computeTdbMinusTt(const datetime_type & datetime) {
    static const long jd_minus_mjd_int = 2400000;
    static const double jd_minus_mjd_frac = .5;

    long jd_int = datetime.first + jd_minus_mjd_int;
    double jd_frac = datetime.second / SecPerDay() + jd_minus_mjd_frac;

    return Duration(ctatv(jd_int, jd_frac), "Sec");
  }

  moment_type TdbSystem::convertFrom(const TimeSystem & time_system, const moment_type & moment) const {
    if (&time_system != this) {
      if ("TAI" == time_system.getName() || "UTC" == time_system.getName()) {
        const TimeSystem & tt(getSystem("TT"));
        return convertFrom(tt, tt.convertFrom(time_system, moment));

      } else if ("TT" == time_system.getName()) {
        datetime_type tt_datetime = time_system.computeDateTime(moment);
        return moment_type(moment.first, moment.second + computeTdbMinusTt(tt_datetime));

      } else {
        throw std::logic_error("Conversion from " + time_system.getName() + " to " + getName() + " is not implemented");
      }
    }
    return moment;
  }

  moment_type TtSystem::convertFrom(const TimeSystem & time_system, const moment_type & moment) const {
    if (&time_system != this) {
      if ("TAI" == time_system.getName()) {
        return moment_type(moment.first, moment.second + computeTtMinusTai());

      } else if ("TDB" == time_system.getName()) {
        // Prepare for time conversion from TDB to TT.
        const int max_iteration = 100;
        const Duration epsilon(0, 100.e-9); // 100 ns, to match Arnold Rots's function ctatv().

        // Use the input moment as the 1st approximation of MJD time in TT system.
        moment_type tt_moment(moment);

        // Refine the output moment (tt_moment) iteratively.
        moment_type tdb_moment(moment);
        for (int ii=0; ii<max_iteration; ii++) {
          // Compute (TDB - TT) at the candidate TT moment.
          datetime_type tt_datetime = computeDateTime(tt_moment);
          Duration tdb_minus_tt = computeTdbMinusTt(tt_datetime);

          // Compute TDB moment for the candidate TT moment.
          tdb_moment.second = tt_moment.second + tdb_minus_tt;

          // Check if the TDB moment is close enough for the input moment.
          if (tdb_moment.second.equivalentTo(moment.second, epsilon)) {
            // Return the TT moment.
            return tt_moment;

          } else {
            // Compute the next candidate.
            tt_moment.second = moment.second - tdb_minus_tt;
          }
        }

        // Conversion from TDB to TT not converged (error)
        throw std::runtime_error("Conversion from " + time_system.getName() + " to " + getName() + " did not converge");

      } else if ("UTC" == time_system.getName()) {
        const TimeSystem & tai(getSystem("TAI"));
        return convertFrom(tai, tai.convertFrom(time_system, moment));

      } else {
        throw std::logic_error("Conversion from " + time_system.getName() + " to " + getName() + " is not implemented");
      }
    }
    return moment;
  }

  moment_type UtcSystem::convertFrom(const TimeSystem & time_system, const moment_type & moment) const {
    if (&time_system != this) {
      if ("TAI" == time_system.getName()) {
        // Get the leap-second table and the oldest MJD in the table.
        const LeapSecTable & leap_sec_table(LeapSecTable::getTable());
        long earliest_mjd = leap_sec_table.getEarliestMjd();

        // Adjust the origin of the given moment, so that it can be used as the origin of a UTC moment.
        moment_type result_moment(moment);
        if (result_moment.first < earliest_mjd) {
          // Adjust the origin of the given monent, such that it comes after the first entry of the leap-second table.
          result_moment.first = earliest_mjd;
          result_moment.second = time_system.computeTimeDifference(moment, moment_type(earliest_mjd, Duration(0, 0.)));
        }

        // Compute UTC - TAI in seconds, and add it to the given moment in UTC system.
        long utc_minus_tai = -10 - leap_sec_table.getCumulativeLeapSec(result_moment.first);
        result_moment.second += Duration(0, utc_minus_tai);

        // Check whether the resultant moment is valid in the current UTC system.
        checkMoment(result_moment);

        // Return the moment.
        return result_moment;

      } else if ("TDB" == time_system.getName() || "TT" == time_system.getName()) {
        const TimeSystem & tai(getSystem("TAI"));
        return convertFrom(tai, tai.convertFrom(time_system, moment));

      } else {
        throw std::logic_error("Conversion from " + time_system.getName() + " to " + getName() + " is not implemented");
      }
    }
    return moment;
  }

  Duration UtcSystem::computeTimeDifference(const moment_type & moment1, const moment_type & moment2) const {
    // Compute the cumulative numbers of leap seconds at the beginning of MJD given by moment1.first and moment2.first.
    const LeapSecTable & leap_sec_table(LeapSecTable::getTable());
    long leap1 = leap_sec_table.getCumulativeLeapSec(moment1.first);
    long leap2 = leap_sec_table.getCumulativeLeapSec(moment2.first);

    // Compute and return the time difference.
    return Duration(moment1.first - moment2.first, leap1 - leap2) + (moment1.second - moment2.second);
  }

  datetime_type UtcSystem::computeDateTime(const moment_type & moment) const {
    // Compute candidate MJD in day & second format.
    long day_int = 0;
    double day_frac = 0.;
    moment.second.get("Day", day_int, day_frac);
    if (day_frac < 0.) --day_int;
    datetime_type datetime(moment.first + day_int, 0.);

    // Adjust the day part of MJD for potential leap second insersions or removals.
    long mjd_adjust = 0;
    datetime_type prev_datetime(datetime);
    do {
      // Save the current candidate for future reference.
      prev_datetime = datetime;

      // Update the candidate.
      datetime.first += mjd_adjust;
      datetime.second = computeTimeDifference(moment, moment_type(datetime.first, Duration(0, 0.))).get("Sec");

      // Determine the next adjustment.
      // Note: this do-while loop ends with a change of the sign of datetime.second.
      if (datetime.second > 0.) {
        if (mjd_adjust == -1) mjd_adjust = 0;
        else mjd_adjust = +1;
      } else if (datetime.second < 0.) {
        if (mjd_adjust == +1) mjd_adjust = 0;
        else mjd_adjust = -1;
      } else {
        mjd_adjust = 0;
      }
    } while (mjd_adjust);

    // Pick the candidate with the positive number of seconds.
    if (datetime.second < 0.) datetime = prev_datetime;

    // Return the date and time.
    return datetime;
  }

  void UtcSystem::checkMoment(const moment_type & moment) const {
    // Get the leap-second table and the oldest MJD in the table.
    const LeapSecTable & leap_sec_table(LeapSecTable::getTable());
    long earliest_mjd = leap_sec_table.getEarliestMjd();

    // Check validity as a UTC moment.
    if (moment.first < earliest_mjd) {
      // The origin is out of bound.
      std::ostringstream os;
      os << "The given moment has the origin, " << moment.first << ".0 MJD (UTC), which is earlier than the earliest" <<
        " covered by the leap second table in " << leap_sec_table.getFileName() << ", " << earliest_mjd << ".0 MJD (UTC).";
      throw std::runtime_error(os.str());

    } else if (Duration(0, 0.) > computeTimeDifference(moment, moment_type(earliest_mjd, Duration(0, 0.)))) {
      // The represented moment of time is out of bound.
      std::ostringstream os;
      os << "The given moment, " << moment.second << " since " << moment.first << " MJD (UTC), is earlier than the earliest" <<
        " covered by the leap second table in " << leap_sec_table.getFileName() << ", " << earliest_mjd << ".0 MJD (UTC).";
      throw std::runtime_error(os.str());
    }
  }

  LeapSecTable & LeapSecTable::getTable() {
    static LeapSecTable s_leap_sec_table;
    return s_leap_sec_table;
  }

  std::string LeapSecTable::getFileName() const {
    return m_file_name;
  }

  void LeapSecTable::load(const std::string & leap_sec_file_name, bool force_load) {
    // Prevent loading unless it hasn't been done or caller demands it.
    if (!(force_load || m_table.empty())) return;

    // Erase previously loaded leap seconds definitions.
    m_table.clear();

    // Set the leap second file name to the data member for future reference.
    m_file_name = leap_sec_file_name;

    // Read MJD and number of leap seconds from table.
    std::auto_ptr<const tip::Table> leap_sec_table(tip::IFileSvc::instance().readTable(m_file_name, "1"));
    long cumulative_leap_sec = 0;
    for (tip::Table::ConstIterator itor = leap_sec_table->begin(); itor != leap_sec_table->end(); ++itor) {
      // Read the MJD and LEAPSECS from the table.
      double mjd_dbl = (*itor)["MJD"].get();
      double leap_sec_dbl = (*itor)["LEAPSECS"].get();

      // Make sure the MJD for the leap second is a whole number of days.
      long mjd = long(mjd_dbl + .5);
      if (mjd != mjd_dbl) throw std::logic_error("UtcSystem: leapsec.fits unexpectedly contained a non-integral MJD value");

      // Make sure the leap second is a whole number of seconds.
      long leap_sec = long(leap_sec_dbl + (leap_sec_dbl > 0 ? .5 : -.5));
      if (leap_sec != leap_sec_dbl) throw std::logic_error("UtcSystem: leapsec.fits unexpectedly contained a non-integral LEAPSECS value");
      cumulative_leap_sec += leap_sec;

      // Add an entry to conversion tables.
      m_table[mjd] = cumulative_leap_sec;
    }
  }

  long LeapSecTable::getCumulativeLeapSec(long mjd) const {
    // Find the first entry of the leap second table which is <= the given MJD.
    table_type::const_reverse_iterator itor = m_table.rbegin();
    for (; (itor != m_table.rend()) && (mjd < itor->first); ++itor) {}

    // Check if it fell of the rend (that is the beginning) so this time is too early for UTC.
    if (itor == m_table.rend()) {
      std::ostringstream os;
      os << "The leap-second table is looked up for " << mjd << ".0 MJD (UTC), which is before its first entry " <<
        m_table.begin()->first << ".0 MJD (UTC).";
      throw std::runtime_error(os.str());
    }

    // Return the contents of the entry.
    return itor->second;
  }

  long LeapSecTable::getEarliestMjd() const {
    // Look for the first entry.
    table_type::const_iterator itor = m_table.begin();
    if (itor == m_table.end()) throw std::runtime_error("The leap-second table is empty.");

    // Return the MJD value of the first entry.
    return itor->first;
  }

}

namespace timeSystem {

  const TimeSystem & TimeSystem::getSystem(const std::string & system_name) {
    // Create TimeSystem objects.
    static const TaiSystem s_tai_system;
    static const TdbSystem s_tdb_system;
    static const TtSystem s_tt_system;
    static const UtcSystem s_utc_system;

    // Make the given time system name case-insensitive.
    std::string uc_system_name = system_name;
    for (std::string::iterator itor = uc_system_name.begin(); itor != uc_system_name.end(); ++itor) *itor = std::toupper(*itor);

    // Find a requested TimeSystem object.
    container_type & container(getContainer());
    container_type::iterator cont_itor = container.find(uc_system_name);
    if (container.end() == cont_itor) throw std::runtime_error("TimeSystem::getSystem could not find time system " + system_name);
    const TimeSystem & system(*cont_itor->second);

    // Load a leap-second table if UTC system is requested.
    if (system.getName() == "UTC") loadLeapSeconds("", false);

    // Return the time system.
    return system;
  }

  void TimeSystem::loadLeapSeconds(std::string leap_sec_file_name, bool force_load) {
    // Rationalize the leap-second file name.
    std::string uc_file_name = leap_sec_file_name;
    for (std::string::iterator itor = uc_file_name.begin(); itor != uc_file_name.end(); ++itor) *itor = std::toupper(*itor);
    if (uc_file_name.empty() || "DEFAULT" == uc_file_name) leap_sec_file_name = getDefaultLeapSecFileName();

    // Load the leap-second table.
    LeapSecTable & leap_sec_table(LeapSecTable::getTable());
    leap_sec_table.load(leap_sec_file_name, force_load);
  }

  std::string TimeSystem::getDefaultLeapSecFileName() {
    std::string uc_file_name = s_default_leap_sec_file;
    for (std::string::iterator itor = uc_file_name.begin(); itor != uc_file_name.end(); ++itor) *itor = std::toupper(*itor);
    if (uc_file_name.empty() || "DEFAULT" == uc_file_name) {
      using namespace st_facilities;
      // Location of default leap sec table.
      return Env::appendFileName(Env::getEnv("TIMING_DIR"), "leapsec.fits");
    }
    return s_default_leap_sec_file;
  }

  void TimeSystem::setDefaultLeapSecFileName(const std::string & leap_sec_file_name) {
    s_default_leap_sec_file = leap_sec_file_name;
  }

  TimeSystem::container_type & TimeSystem::getContainer() {
    static container_type s_prototype;
    return s_prototype;
  }

  TimeSystem::TimeSystem(const std::string & system_name): m_system_name(system_name) {
    std::string uc_system_name = system_name;
    for (std::string::iterator itor = uc_system_name.begin(); itor != uc_system_name.end(); ++itor) *itor = std::toupper(*itor);
    getContainer()[uc_system_name] = this;
  }

  TimeSystem::~TimeSystem() {}

  std::string TimeSystem::getName() const {
    return m_system_name;
  }

  Duration TimeSystem::computeTimeDifference(const moment_type & moment1, const moment_type & moment2) const {
    return Duration(moment1.first - moment2.first, 0.) + (moment1.second - moment2.second);
  }

  datetime_type TimeSystem::computeDateTime(const moment_type & moment) const {
    // Compute the total elapsed time since 0.0 MJD.
    Duration elapsed_total = Duration(moment.first, 0.) + moment.second;

    // Split the total elapsed time into days and seconds.
    long elapsed_int = 0;
    double elapsed_frac = 0.;
    elapsed_total.get("Day", elapsed_int, elapsed_frac);
    Duration elapsed_residual = elapsed_total - Duration(elapsed_int, 0.);
    double elapsed_sec = elapsed_residual.get("Sec");

    // Return the date and time.
    return datetime_type(elapsed_int, elapsed_sec);
  }

  std::ostream & operator <<(std::ostream & os, const TimeSystem & sys) {
    sys.write(os);
    return os;
  }

  st_stream::OStream & operator <<(st_stream::OStream & os, const TimeSystem & sys) {
    sys.write(os);
    return os;
  }

  void TimeSystem::checkMoment(const moment_type & /* moment */) const {
    // Do nothing for most time systems.
  }

}
