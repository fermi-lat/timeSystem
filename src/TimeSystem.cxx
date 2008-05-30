/** \file TimeSystem.cxx
    \brief Implementation of TimeSystem class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "st_facilities/Env.h"

#include "timeSystem/Duration.h"
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

      Duration computeTdbMinusTt(const Duration & mjd) const;

      virtual moment_type convertFrom(const TimeSystem & time_system, const moment_type & moment) const;
  };

  class TtSystem : public TimeSystem {
    public:
      TtSystem(): TimeSystem("TT") {}

      virtual moment_type convertFrom(const TimeSystem & time_system, const moment_type & moment) const;
  };

  class UtcSystem : public TimeSystem {
    public:
      UtcSystem();

      virtual Duration computeTimeDifference(const moment_type & moment1, const moment_type & moment2) const;

      virtual moment_type computeAdvancedTime(const moment_type & moment, const Duration & elapsed) const;

      virtual moment_type convertFrom(const TimeSystem & time_system, const moment_type & moment) const;

      void loadLeapSecTable(const std::string & leap_sec_file_name, bool force_load);

      moment_type convertTaiToUtc(const moment_type & tai_moment) const;

      moment_type convertUtcToTai(const moment_type & utc_moment) const;

    private:
      typedef std::map<long, long> new_leaptable_type;
      new_leaptable_type m_leap_table;
      // Note: The cumulative number of leap seconds since its introduction at the beginning of the date
      //       whose MJD in UTC is given by the key of the std::map object.

      long getCumulativeLeapSec(long mjd) const;
  };

  moment_type TaiSystem::convertFrom(const TimeSystem & time_system, const moment_type & moment) const {
    if (&time_system != this) {
      if ("TDB" == time_system.getName()) {
        const TimeSystem & tt(getSystem("TT"));
        return convertFrom(tt, tt.convertFrom(time_system, moment));
      } else if ("TT" == time_system.getName()) {
        return computeAdvancedTime(moment, Duration(0, TaiMinusTtSec()));
      } else if ("UTC" == time_system.getName()) {
        const UtcSystem & utc_system = dynamic_cast<const UtcSystem &>(getSystem("UTC"));
        return utc_system.convertUtcToTai(moment);
      } else {
        throw std::logic_error("Conversion from " + time_system.getName() + " to " + getName() + " is not implemented");
      }
    }
    return moment;
  }

  Duration TdbSystem::computeTdbMinusTt(const Duration & mjd_tt) const {
    static const long jd_minus_mjd_int = 2400000;
    static const double jd_minus_mjd_frac = .5;

    IntFracPair mjd_time = mjd_tt.getValue(Day);

    return Duration(0, ctatv(mjd_time.getIntegerPart() + jd_minus_mjd_int, mjd_time.getFractionalPart() + jd_minus_mjd_frac));

  }

  moment_type TdbSystem::convertFrom(const TimeSystem & time_system, const moment_type & moment) const {
    if (&time_system != this) {
      if ("TAI" == time_system.getName() || "UTC" == time_system.getName()) {
        const TimeSystem & tt(getSystem("TT"));
        return convertFrom(tt, tt.convertFrom(time_system, moment));
      } else if ("TT" == time_system.getName()) {
        Duration src(moment.first, moment.second);
        Duration delta = computeTdbMinusTt(src);
        return computeAdvancedTime(moment, delta);
      } else {
        throw std::logic_error("Conversion from " + time_system.getName() + " to " + getName() + " is not implemented");
      }
    }
    return moment;
  }

  moment_type TtSystem::convertFrom(const TimeSystem & time_system, const moment_type & moment) const {
    if (&time_system != this) {
      if ("TAI" == time_system.getName()) {
        return computeAdvancedTime(moment, Duration(0, TtMinusTaiSec()));
      } else if ("TDB" == time_system.getName()) {
        // Prepare for time conversion from TDB to TT.
        static const TdbSystem & tdb_system = dynamic_cast<const TdbSystem &>(getSystem("TDB"));
        const int max_iteration = 100;
        const Duration epsilon(0, 100.e-9); // 100 ns, to match Arnold Rots's function ctatv().

        // Compute MJD number for input time
        Duration src(moment.first, moment.second);

        // 1st approximation of MJD time in TT system.
        Duration dest = src;

        // 1st approximation of time difference between TT and TDB.
        Duration diff = tdb_system.computeTdbMinusTt(dest);

        // iterative approximation of dest.
        for (int ii=0; ii<max_iteration; ii++) {

          // compute next candidate of dest.
          dest = src - diff;

          // compute time difference between TT and TDB at dest.
          diff = tdb_system.computeTdbMinusTt(dest);

          // check whether binary demodulation successfully converged or not
          if (src.equivalentTo(dest + diff, epsilon)) {
            long mjd_day = dest.getValue(Day).getIntegerPart();
            double mjd_sec = (dest - Duration(mjd_day, 0.)).getValue(Sec).getDouble();
            return moment_type(mjd_day, mjd_sec);
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

  UtcSystem::UtcSystem(): TimeSystem("UTC") {}

  moment_type UtcSystem::convertFrom(const TimeSystem & time_system, const moment_type & moment) const {
    if (&time_system != this) {
      if ("TAI" == time_system.getName()) {
        return convertTaiToUtc(moment);
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
    long leap1 = getCumulativeLeapSec(moment1.first);
    long leap2 = getCumulativeLeapSec(moment2.first);

    // Compute and return the time difference.
    return Duration(moment1.first - moment2.first, moment1.second - moment2.second + leap1 - leap2);
  }

  moment_type UtcSystem::computeAdvancedTime(const moment_type & moment, const Duration & elapsed) const {
    // Compute candidate MJD for the advanced moment of time.
    moment_type tentative_moment = TimeSystem::computeAdvancedTime(moment, elapsed);
    long mjd_day = tentative_moment.first;

    // Adjust the day part of MJD for potential leap second insersions or removals.
    long mjd_day_adjust = 0;
    for (; elapsed > computeTimeDifference(moment_type(mjd_day + mjd_day_adjust, 0.), moment); ++mjd_day_adjust) {}
    if (mjd_day_adjust > 0) {
      --mjd_day_adjust;
    } else {
      for (; elapsed < computeTimeDifference(moment_type(mjd_day + mjd_day_adjust, 0.), moment); --mjd_day_adjust) {}
    }
    mjd_day += mjd_day_adjust;

    // Compute the number of seconds since the beginning of the MJD.
    Duration residual = elapsed - computeTimeDifference(moment_type(mjd_day, 0.), moment);
    double mjd_sec = residual.getValue(Sec).getDouble();

    // Return the advanced moment of time.
    return moment_type(mjd_day, mjd_sec);
  }

  long UtcSystem::getCumulativeLeapSec(long mjd) const {
    // Find the first entry of the leap second table which is <= the given MJD.
    new_leaptable_type::const_reverse_iterator itor = m_leap_table.rbegin();
    for (; (itor != m_leap_table.rend()) && (mjd < itor->first); ++itor) {}

    // Check if it fell of the rend (that is the beginning) so this time is too early for UTC.
    if (itor == m_leap_table.rend()) {
      std::ostringstream os;
      os << "The leap-second table is looked up for " << mjd << ".0 MJD (UTC), which is before its first entry " <<
        m_leap_table.begin()->first << ".0 MJD (UTC).";
      throw std::runtime_error(os.str());
    }

    // Return the contents of the entry.
    return itor->second;
  }

  void UtcSystem::loadLeapSecTable(const std::string & leap_sec_file_name, bool force_load) {
    // Prevent loading unless it hasn't been done or caller demands it.
    if (!(force_load || m_leap_table.empty())) return;

    // Erase previously loaded leap seconds definitions.
    m_leap_table.clear();

    // Read MJD and number of leap seconds from table.
    std::auto_ptr<const tip::Table> leap_sec_table(tip::IFileSvc::instance().readTable(leap_sec_file_name, "1"));
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
      m_leap_table[mjd] = cumulative_leap_sec;
    }
  }

  moment_type UtcSystem::convertTaiToUtc(const moment_type & tai_moment) const {
    // Rationalize the given time moment, so that the second portion is below 86400.0.
    // TODO: Use TaiSystem::computeAdvancedTime method.
    moment_type tai_moment_rat = TimeSystem::computeAdvancedTime(moment_type(tai_moment.first, 0.), Duration(0, tai_moment.second));

    // Compute UTC - TAI in seconds.
    long utc_minus_tai = -10 - getCumulativeLeapSec(tai_moment_rat.first);

    // Add the UTC - TAI to the given time moment.
    return computeAdvancedTime(tai_moment_rat, Duration(0, utc_minus_tai));
  }

  moment_type UtcSystem::convertUtcToTai(const moment_type & utc_moment) const {
    // Compute TAI - UTC in seconds.
    // Note: The given time moment must be interpreted as is, so that the leap-second table is properly looked up.
    long tai_minus_utc = 10 + getCumulativeLeapSec(utc_moment.first);

    // Add the TAI - UTC to the given time moment.
    // TODO: Use TaiSystem::computeAdvancedTime method.
    return TimeSystem::computeAdvancedTime(utc_moment, Duration(0, tai_minus_utc));
  }

}

namespace timeSystem {

  const TimeSystem & TimeSystem::getSystem(const std::string & system_name) {
    TimeSystem & system(getNonConstSystem(system_name));
    if (system.getName() == "UTC") loadLeapSeconds("", false);
    return system;
  }

  void TimeSystem::loadLeapSeconds(std::string leap_sec_file_name, bool force_load) {
    std::string uc_file_name = leap_sec_file_name;
    for (std::string::iterator itor = uc_file_name.begin(); itor != uc_file_name.end(); ++itor) *itor = std::toupper(*itor);
    if (uc_file_name.empty() || "DEFAULT" == uc_file_name)
      leap_sec_file_name = getDefaultLeapSecFileName();
    UtcSystem & utc_sys(dynamic_cast<UtcSystem &>(getNonConstSystem("UTC")));
    utc_sys.loadLeapSecTable(leap_sec_file_name, force_load);
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

  TimeSystem & TimeSystem::getNonConstSystem(const std::string & system_name) {
    static TaiSystem s_tai_system;
    static TdbSystem s_tdb_system;
    static TtSystem s_tt_system;
    static UtcSystem s_utc_system;

    std::string uc_system_name = system_name;
    for (std::string::iterator itor = uc_system_name.begin(); itor != uc_system_name.end(); ++itor) *itor = std::toupper(*itor);
    container_type & container(getContainer());

    container_type::iterator cont_itor = container.find(uc_system_name);
    if (container.end() == cont_itor) throw std::runtime_error("TimeSystem::getSystem could not find time system " + system_name);
    return *cont_itor->second;
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
    return Duration(moment1.first - moment2.first, moment1.second - moment2.second);
  }

  moment_type TimeSystem::computeAdvancedTime(const moment_type & moment, const Duration & elapsed) const {
    // Compute the total elapsed time since 0.0 MJD.
    Duration elapsed_total = Duration(moment.first, moment.second) + elapsed;

    // Split the total elapsed time into days and seconds.
    long elapsed_day = elapsed_total.getValue(Day).getIntegerPart();
    Duration elapsed_residual = elapsed_total - Duration(elapsed_day, 0.);
    double elapsed_sec = elapsed_residual.getValue(Sec).getDouble();

    // Return the advanced moment of time.
    return moment_type(elapsed_day, elapsed_sec);
  }

  std::ostream & operator <<(std::ostream & os, const TimeSystem & sys) {
    sys.write(os);
    return os;
  }

  st_stream::OStream & operator <<(st_stream::OStream & os, const TimeSystem & sys) {
    sys.write(os);
    return os;
  }

}
