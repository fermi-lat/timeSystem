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

      virtual Moment convertFrom(const TimeSystem & time_system, const Moment & time) const;

      virtual Duration computeTimeDifference(const Duration & mjd1, const Duration & mjd2) const;

      virtual Duration computeMjd(const Moment & time) const;

  };

  class TdbSystem : public TimeSystem {
    public:
      TdbSystem(): TimeSystem("TDB") {}

      virtual Moment convertFrom(const TimeSystem & time_system, const Moment & time) const;

      virtual Duration computeTimeDifference(const Duration & mjd1, const Duration & mjd2) const;

      virtual Duration computeMjd(const Moment & time) const;

      Duration computeTdbMinusTt(const Duration & mjd) const;

  };

  class TtSystem : public TimeSystem {
    public:
      TtSystem(): TimeSystem("TT") {}

      virtual Moment convertFrom(const TimeSystem & time_system, const Moment & time) const;

      virtual Duration computeTimeDifference(const Duration & mjd1, const Duration & mjd2) const;

      virtual Duration computeMjd(const Moment & time) const;

  };

  class UtcSystem : public TimeSystem {
    public:
      UtcSystem();

      virtual Moment convertFrom(const TimeSystem & time_system, const Moment & time) const;

      virtual Duration computeTimeDifference(const Duration & mjd1, const Duration & mjd2) const;

      virtual Duration computeMjd(const Moment & time) const;

      void loadLeapSecTable(const std::string & leap_sec_file_name, bool force_load);

      Moment convertTaiToUtc(const Moment & tai_time) const;

      Moment convertUtcToTai(const Moment & utc_time) const;

    private:
      struct leapdata_type {
	leapdata_type(): m_leap_end(Duration(0, 0.)), m_inserted(true), m_time_diff(Duration(0, 0.)), m_leap_end_dest(Duration(0, 0.)) {}
	leapdata_type(const Duration & leap_end, const bool & inserted, const Duration & time_diff, const Duration & leap_end_dest):
          m_leap_end(leap_end), m_inserted(inserted), m_time_diff(time_diff), m_leap_end_dest(leap_end_dest) {}

        Duration m_leap_end;       // MJD number for a moment immediately after a leap second is inserted or removed.
        bool m_inserted;           // flag to store whether a leap second is inserted (true) or removed (false).
        Duration m_time_diff;      // time difference between TAI and UTC after a leap second is inserted or removed.
        Duration m_leap_end_dest;  // Same as m_leap_end, but in a time system to be converted to.
      };
      typedef std::map<Duration, leapdata_type> leaptable_type;
      leaptable_type m_tai_minus_utc;
      leaptable_type m_utc_minus_tai;

  };

  Moment TaiSystem::convertFrom(const TimeSystem & time_system, const Moment & time) const {
    if (&time_system != this) {
      if ("TDB" == time_system.getName()) {
        const TimeSystem & tt(getSystem("TT"));
        return convertFrom(tt, tt.convertFrom(time_system, time));
      } else if ("TT" == time_system.getName()) {
        return Moment(time.first, time.second + Duration(0, TaiMinusTtSec()));
      } else if ("UTC" == time_system.getName()) {
        const UtcSystem & utc_system = dynamic_cast<const UtcSystem &>(getSystem("UTC"));
        return utc_system.convertUtcToTai(time);
      } else {
        throw std::logic_error("Conversion from " + time_system.getName() + " to " + getName() + " is not implemented");
      }
    }
    return time;
  }

  Duration TaiSystem::computeTimeDifference(const Duration & mjd1, const Duration & mjd2) const {
    return mjd1 - mjd2;
  }

  Duration TaiSystem::computeMjd(const Moment & time) const {
    return time.first + time.second;
  }

  Duration TdbSystem::computeTdbMinusTt(const Duration & mjd_tt) const {
    static const long jd_minus_mjd_int = 2400000;
    static const double jd_minus_mjd_frac = .5;

    IntFracPair mjd_time = mjd_tt.getValue(Day);

    return Duration(0, ctatv(mjd_time.getIntegerPart() + jd_minus_mjd_int, mjd_time.getFractionalPart() + jd_minus_mjd_frac));

  }

  Moment TdbSystem::convertFrom(const TimeSystem & time_system, const Moment & time) const {
    if (&time_system != this) {
      if ("TAI" == time_system.getName() || "UTC" == time_system.getName()) {
        const TimeSystem & tt(getSystem("TT"));
        return convertFrom(tt, tt.convertFrom(time_system, time));
      } else if ("TT" == time_system.getName()) {
        Duration src = time.first + time.second;
        Duration dest = src + computeTdbMinusTt(src);
        return Moment(dest, Duration(0, 0.));
      } else {
        throw std::logic_error("Conversion from " + time_system.getName() + " to " + getName() + " is not implemented");
      }
    }
    return time;
  }

  Duration TdbSystem::computeTimeDifference(const Duration & mjd1, const Duration & mjd2) const {
    return mjd1 - mjd2;
  }

  Duration TdbSystem::computeMjd(const Moment & time) const {
    return time.first + time.second;
  }

  Moment TtSystem::convertFrom(const TimeSystem & time_system, const Moment & time) const {
    if (&time_system != this) {
      if ("TAI" == time_system.getName()) {
        return Moment(time.first, time.second + Duration(0, TtMinusTaiSec()));
      } else if ("TDB" == time_system.getName()) {
        // Prepare for time conversion from TDB to TT.
        static const TdbSystem & tdb_system = dynamic_cast<const TdbSystem &>(getSystem("TDB"));
        const int max_iteration = 100;
        const Duration epsilon(0, 100.e-9); // 100 ns, to match Arnold Rots's function ctatv().

        // Compute MJD number for input time
        Duration src = time.first + time.second;

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
          if (src.equivalentTo(dest + diff, epsilon)) return Moment(dest, Duration(0, 0.));
        }

        // Conversion from TDB to TT not converged (error)
        throw std::runtime_error("Conversion from " + time_system.getName() + " to " + getName() + " did not converge");
      } else if ("UTC" == time_system.getName()) {
        const TimeSystem & tai(getSystem("TAI"));
        return convertFrom(tai, tai.convertFrom(time_system, time));
      } else {
        throw std::logic_error("Conversion from " + time_system.getName() + " to " + getName() + " is not implemented");
      }
    }
    return time;
  }

  Duration TtSystem::computeTimeDifference(const Duration & mjd1, const Duration & mjd2) const {
    return mjd1 - mjd2;
  }

  Duration TtSystem::computeMjd(const Moment & time) const {
    return time.first + time.second;
  }

  UtcSystem::UtcSystem(): TimeSystem("UTC") {}

  Moment UtcSystem::convertFrom(const TimeSystem & time_system, const Moment & time) const {
    if (&time_system != this) {
      if ("TAI" == time_system.getName()) {
        return convertTaiToUtc(time);
      } else if ("TDB" == time_system.getName() || "TT" == time_system.getName()) {
        const TimeSystem & tai(getSystem("TAI"));
        return convertFrom(tai, tai.convertFrom(time_system, time));
      } else {
        throw std::logic_error("Conversion from " + time_system.getName() + " to " + getName() + " is not implemented");
      }
    }
    return time;
  }

  Duration UtcSystem::computeTimeDifference(const Duration & mjd1, const Duration & mjd2) const {
    Moment tai1 = convertUtcToTai(Moment(mjd1, Duration(0, 0.)));
    Moment tai2 = convertUtcToTai(Moment(mjd2, Duration(0, 0.)));
    return (tai1.first - tai2.first) + (tai1.second - tai2.second);
  }

  Duration UtcSystem::computeMjd(const Moment & time) const {
    const TimeSystem & tai_system(TimeSystem::getSystem("TAI"));

    // "Add" elapsed time to origin. To do it correctly, it should be
    // done in a time system without a leap second, such as TAI.
    Moment tai_time = tai_system.convertFrom(*this, time);

    // Produce a combined single tai Duration.
    Duration mjd_tai = tai_time.first + tai_time.second;

    // Start from the end of the leap second table and go backwards, stopping at the first leap time
    // which is <= the given time.
    leaptable_type::const_reverse_iterator itor = m_utc_minus_tai.rbegin();
    for (; (itor != m_utc_minus_tai.rend()) && (mjd_tai < itor->first); ++itor) {}
    if (itor == m_utc_minus_tai.rend()) {
      // Fell of the rend (that is the beginning) so this time is too early.
      std::ostringstream os;
      os << "UtcSystem::computeMjd cannot compute MJD of the Moment(" << time.first << ", " << time.second << ") (UTC)";
      throw std::runtime_error(os.str());
    }

    Duration result;
    if ((!(itor->second.m_inserted)) || (itor->second.m_leap_end <= mjd_tai)) {
      // General case: a numerical difference between MJD (UTC) and MJD (TAI) is constant over time.
      result = mjd_tai + itor->second.m_time_diff;
    } else {
      // Leap second(s) being inserted: MJD (UTC) stays constant while MJD (TAI) grows.
      // In this case, the end of the leap-second insertion is chosen as a resultant MJD number,
      // to avoid any loss of precision in computating a time origin near a leap-second insertion, where
      // a tiny difference in MJD number can make a big difference in time being pointed by one second.
      result = itor->second.m_leap_end_dest;
    }

    return result;
  }

  void UtcSystem::loadLeapSecTable(const std::string & leap_sec_file_name, bool force_load) {
    // Prevent loading unless it hasn't been done or caller demands it.
    if (!(force_load || m_tai_minus_utc.empty() || m_utc_minus_tai.empty())) return;

    // Erase previously loaded leap seconds definitions.
    m_tai_minus_utc.clear();
    m_utc_minus_tai.clear();

    // Read MJD and number of leap seconds from table.
    std::auto_ptr<const tip::Table> leap_sec_table(tip::IFileSvc::instance().readTable(leap_sec_file_name, "1"));
    double time_diff = 10.;
    for (tip::Table::ConstIterator itor = leap_sec_table->begin(); itor != leap_sec_table->end(); ++itor) {
      // Read the MJD and LEAPSECS from the table.
      double mjd_dbl = (*itor)["MJD"].get();
      double leap_sec = (*itor)["LEAPSECS"].get();

      // Make sure the MJD for the leap second is a whole number of days.
      long mjd = long(mjd_dbl + .5);
      if (mjd != mjd_dbl) throw std::logic_error("UtcSystem: leapsec.fits unexpectedly contained a non-integral MJD value");

      // Pre-compute leap second data for conversion tables.
      time_diff += leap_sec;
      bool inserted = (leap_sec > 0);
      Duration mjd_end_utc(mjd, 0.);
      Duration mjd_end_tai(mjd, time_diff);
      Duration mjd_start_utc = (inserted ? mjd_end_utc : Duration(mjd, leap_sec));
      Duration mjd_start_tai = (inserted ? Duration(mjd, time_diff - leap_sec) : mjd_end_tai);

      // Add an entry to conversion tables.
      m_tai_minus_utc[mjd_start_utc] = leapdata_type(mjd_end_utc, inserted, Duration(0, time_diff), mjd_end_tai);
      m_utc_minus_tai[mjd_start_tai] = leapdata_type(mjd_end_tai, inserted, Duration(0, -time_diff), mjd_end_utc);
    }
  }

  Moment UtcSystem::convertTaiToUtc(const Moment & tai_time) const {
    // Produce a combined single MJD number in TAI system.
    Duration mjd_tai = tai_time.first + tai_time.second;

    // Start from the end of the leap second table and go backwards, stopping at the first leap time
    // which is <= the given time.
    leaptable_type::const_reverse_iterator itor = m_utc_minus_tai.rbegin();
    for (; (itor != m_utc_minus_tai.rend()) && (mjd_tai < itor->first); ++itor) {}
    if (itor == m_utc_minus_tai.rend()) {
      // Fell of the rend (that is the beginning) so this time is too early.
      std::ostringstream os;
      os << "UtcSystem::convertTaiToUtc cannot convert to UTC the Moment(" << tai_time.first << ", " << tai_time.second <<
        " in TAI.";
      throw std::runtime_error(os.str());
    }

    Moment result;
    if ((!(itor->second.m_inserted)) || (itor->second.m_leap_end <= mjd_tai)) {
      // General case: a numerical difference between MJD (UTC) and MJD (TAI) is constant over time.
      result = Moment(mjd_tai + itor->second.m_time_diff, Duration(0, 0.));
    } else {
      // Leap second(s) being inserted: MJD (UTC) stays constant while MJD (TAI) grows.
      // In this case, the end of the leap-second insertion is chosen as a time origin of returning Moment,
      // to avoid any loss of precision in computating a time origin near a leap-second insertion, where
      // a tiny difference in MJD number can make a big difference in time being pointed by one second.
      result = Moment(itor->second.m_leap_end_dest, mjd_tai - itor->second.m_leap_end);
    }

    return result;
  }

  Moment UtcSystem::convertUtcToTai(const Moment & utc_time) const {
    // Start from the end of the leap second table and go backwards, stopping at the first leap time
    // which is <= the given time.
    leaptable_type::const_reverse_iterator itor = m_tai_minus_utc.rbegin();
    for (; (itor != m_tai_minus_utc.rend()) && (utc_time.first < itor->first); ++itor) {}
    if (itor == m_tai_minus_utc.rend()) {
      // Fell of the rend (that is the beginning) so this time is too early.
      std::ostringstream os;
      os << "UtcSystem::convertUtcToTai cannot convert to TAI the Moment(" << utc_time.first << ", " << utc_time.second <<
        ") in UTC.";
      throw std::runtime_error(os.str());
    }

    Moment result;
    if (itor->second.m_inserted || (itor->second.m_leap_end <= utc_time.first)) {
      // General case: a numerical difference between MJD (UTC) and MJD (TAI) is constant over time.
      result = Moment(utc_time.first + itor->second.m_time_diff, utc_time.second);
    } else {
      // Leap second(s) being removed: any MJD (UTC) corresponds to a single MJD (TAI).
      // In this case, utc_time.first MJD (UTC) points to an unphysical time during leap second(s) being removed
      // where any MJD (UTC) is interpreted as the end time of the leap-second removal.
      result = Moment(itor->second.m_leap_end_dest, utc_time.second);
    }

    return result;
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

  std::ostream & operator <<(std::ostream & os, const TimeSystem & sys) {
    sys.write(os);
    return os;
  }

  st_stream::OStream & operator <<(st_stream::OStream & os, const TimeSystem & sys) {
    sys.write(os);
    return os;
  }

}
