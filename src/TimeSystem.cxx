/** \file TimeSystem.cxx
    \brief Implementation of TimeSystem class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "st_facilities/Env.h"

#include "timeSystem/AbsoluteTime.h"
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

namespace {

  class TaiSystem : public TimeSystem {
    public:
      TaiSystem(): TimeSystem("TAI") {}

      virtual Duration convertFrom(const TimeSystem & time_system, const Duration & origin, const Duration & time) const;

      virtual Duration computeTimeDifference(const Duration & mjd1, const Duration & mjd2) const;

  };

  class TdbSystem : public TimeSystem {
    public:
      TdbSystem(): TimeSystem("TDB") {}

      virtual Duration convertFrom(const TimeSystem & time_system, const Duration & origin, const Duration & time) const;

      virtual Duration computeTimeDifference(const Duration & mjd1, const Duration & mjd2) const;

      Duration computeTdbMinusTt(const Duration & mjd) const;

  };

  class TtSystem : public TimeSystem {
    public:
      TtSystem(): TimeSystem("TT") {}

      virtual Duration convertFrom(const TimeSystem & time_system, const Duration & origin, const Duration & time) const;

      virtual Duration computeTimeDifference(const Duration & mjd1, const Duration & mjd2) const;

  };

  class UtcSystem : public TimeSystem {
    public:
      UtcSystem();

      virtual Duration convertFrom(const TimeSystem & time_system, const Duration & origin, const Duration & time) const;

      virtual Duration computeTimeDifference(const Duration & mjd1, const Duration & mjd2) const;

      void loadLeapSecTable(const std::string & leap_sec_file_name);

      Duration computeTaiMinusUtc(const Duration & mjd_utc) const;

      Duration computeUtcMinusTai(const Duration & mjd_tai) const;

    private:
      struct leapdata_type {
	leapdata_type(): m_leap_end(Duration(0, 0.)), m_inserted(true), m_time_diff(Duration(0, 0.)) {}
	leapdata_type(const Duration & leap_end, const bool & inserted, const Duration & time_diff):
          m_leap_end(leap_end), m_inserted(inserted), m_time_diff(time_diff) {}

        Duration m_leap_end;  // MJD number for a moment immediately after a leap second is inserted or removed.
        bool m_inserted;      // flag to store whether a leap second is inserted (true) or removed (false).
        Duration m_time_diff; // time difference between TAI and UTC after a leap second is inserted or removed.
      };
      typedef std::map<Duration, leapdata_type> leaptable_type;
      leaptable_type m_tai_minus_utc;
      leaptable_type m_utc_minus_tai;

  };

  Duration TaiSystem::convertFrom(const TimeSystem & time_system, const Duration & origin, const Duration & time) const {
    if (&time_system != this) {
      if ("TDB" == time_system.getName()) {
        const TimeSystem & tt(getSystem("TT"));
        return convertFrom(tt, origin, tt.convertFrom(time_system, origin, time));
      } else if ("TT" == time_system.getName()) {
        return time + Duration(0, TaiMinusTtSec());
      } else if ("UTC" == time_system.getName()) {
        const UtcSystem & utc_system = dynamic_cast<const UtcSystem &>(getSystem("UTC"));
        return time + utc_system.computeTaiMinusUtc(origin);
      } else {
        throw std::logic_error("Conversion from " + time_system.getName() + " to " + getName() + " is not implemented");
      }
    }
    return time;
  }

  Duration TaiSystem::computeTimeDifference(const Duration & mjd1, const Duration & mjd2) const {
    return mjd1 - mjd2;
  }

  Duration TdbSystem::computeTdbMinusTt(const Duration & mjd_tt) const {
    static const long jd_minus_mjd_int = 2400000;
    static const double jd_minus_mjd_frac = .5;

    TimeValue mjd_time = mjd_tt.getValue(Day);

    return Duration(0, ctatv(mjd_time.getIntegerPart() + jd_minus_mjd_int, mjd_time.getFractionalPart() + jd_minus_mjd_frac));

  }

  Duration TdbSystem::convertFrom(const TimeSystem & time_system, const Duration & origin, const Duration & time) const {
    if (&time_system != this) {
      if ("TAI" == time_system.getName() || "UTC" == time_system.getName()) {
        const TimeSystem & tt(getSystem("TT"));
        return convertFrom(tt, origin, tt.convertFrom(time_system, origin, time));
      } else if ("TT" == time_system.getName()) {
        Duration src = origin + time;
        Duration dest = src + computeTdbMinusTt(src);
        return dest - origin;
      } else {
        throw std::logic_error("Conversion from " + time_system.getName() + " to " + getName() + " is not implemented");
      }
    }
    return time;
  }

  Duration TdbSystem::computeTimeDifference(const Duration & mjd1, const Duration & mjd2) const {
    return mjd1 - mjd2;
  }

  Duration TtSystem::convertFrom(const TimeSystem & time_system, const Duration & origin, const Duration & time) const {
    if (&time_system != this) {
      if ("TAI" == time_system.getName()) {
        return time + Duration(0, TtMinusTaiSec());
      } else if ("TDB" == time_system.getName()) {
        // Prepare for time conversion from TDB to TT.
        static const TdbSystem & tdb_system = dynamic_cast<const TdbSystem &>(getSystem("TDB"));
        const int max_iteration = 100;
        const Duration epsilon(0, 100.e-9); // 100 ns, to match Arnold Rots's function ctatv().

        // Compute MJD number for input time
        Duration src = origin + time;

        // 1st approximation of MJD time in TT system.
        Duration dest = src;

        // iterative approximation of dest.
        for (int ii=0; ii<max_iteration; ii++) {

          // compute next candidate of dest.
          dest = src - tdb_system.computeTdbMinusTt(dest);

          // check whether binary demodulation successfully converged or not
          if (src.equivalentTo(dest + tdb_system.computeTdbMinusTt(dest), epsilon)) return dest - origin;
        }

        // Conversion from TDB to TT not converged (error)
        throw std::runtime_error("Conversion from " + time_system.getName() + " to " + getName() + " did not converge");
      } else if ("UTC" == time_system.getName()) {
        const TimeSystem & tai(getSystem("TAI"));
        return convertFrom(tai, origin, tai.convertFrom(time_system, origin, time));
      } else {
        throw std::logic_error("Conversion from " + time_system.getName() + " to " + getName() + " is not implemented");
      }
    }
    return time;
  }

  Duration TtSystem::computeTimeDifference(const Duration & mjd1, const Duration & mjd2) const {
    return mjd1 - mjd2;
  }

  UtcSystem::UtcSystem(): TimeSystem("UTC") {
    using namespace st_facilities;
    // Location of default leap sec table.
    std::string leap_sec_file_name = Env::appendFileName(Env::getEnv("TIMING_DIR"), "leapsec.fits");
    // TODO: load leap seconds table on demand rather than automatically here in the constructor.
    loadLeapSecTable(leap_sec_file_name);
  }

  Duration UtcSystem::convertFrom(const TimeSystem & time_system, const Duration & origin, const Duration & time) const {
    if (&time_system != this) {
      if ("TAI" == time_system.getName()) {
        return time + computeUtcMinusTai(origin);
      } else if ("TDB" == time_system.getName() || "TT" == time_system.getName()) {
        const TimeSystem & tai(getSystem("TAI"));
        return convertFrom(tai, origin, tai.convertFrom(time_system, origin, time));
      } else {
        throw std::logic_error("Conversion from " + time_system.getName() + " to " + getName() + " is not implemented");
      }
    }
    
    return time;
  }

  Duration UtcSystem::computeTimeDifference(const Duration & mjd1, const Duration & mjd2) const {
    return (mjd1 + computeTaiMinusUtc(mjd1)) - (mjd2 + computeTaiMinusUtc(mjd2));
  }

  void UtcSystem::loadLeapSecTable(const std::string & leap_sec_file_name) {
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

      // Pre-compute leap second data for TAI-minus-UTC table.
      time_diff += leap_sec;
      bool inserted = (leap_sec > 0);
      Duration mjd_end(mjd, 0.);
      Duration mjd_start = (inserted ? mjd_end : mjd_end + Duration(0, leap_sec));

      // Add an entry to TAI-minus-UTC table.
      m_tai_minus_utc[mjd_start] = leapdata_type(mjd_end, inserted, Duration(0, time_diff));

      // Pre-compute leap second data for UTC-minus-TAI table.
      mjd_end = Duration(mjd, time_diff);
      mjd_start = (inserted ? mjd_end - Duration(0, leap_sec) : mjd_end);

      // Add an entry to UTC-minus-TAI table.
      m_utc_minus_tai[mjd_start] = leapdata_type(mjd_end, inserted, Duration(0, -time_diff));
    }
  }

  Duration UtcSystem::computeTaiMinusUtc(const Duration & mjd_utc) const {
    // Start from the end of the leap second table and go backwards, stopping at the first leap time
    // which is <= the given time.
    leaptable_type::const_reverse_iterator itor = m_tai_minus_utc.rbegin();
    for (; (itor != m_tai_minus_utc.rend()) && (mjd_utc < itor->first); ++itor) {}
    if (itor == m_tai_minus_utc.rend()) {
      // Fell of the rend (that is the beginning) so this time is too early.
      std::ostringstream os;
      os << "UtcSystem::computeTaiMinusUtc cannot compute TAI - UTC for time " << mjd_utc << " MJD (UTC)";
      throw std::runtime_error(os.str());
    }

    if (itor->second.m_inserted || (itor->second.m_leap_end <= mjd_utc)) {
      return itor->second.m_time_diff;
    }
    return itor->second.m_time_diff + (itor->second.m_leap_end - mjd_utc);
  }

  Duration UtcSystem::computeUtcMinusTai(const Duration & mjd_tai) const {
    // Start from the end of the leap second table and go backwards, stopping at the first leap time
    // which is <= the given time.
    leaptable_type::const_reverse_iterator itor = m_utc_minus_tai.rbegin();
    for (; (itor != m_utc_minus_tai.rend()) && (mjd_tai < itor->first); ++itor) {}
    if (itor == m_utc_minus_tai.rend()) {
      // Fell of the rend (that is the beginning) so this time is too early.
      std::ostringstream os;
      os << "UtcSystem::computeUtcMinusTai cannot compute UTC - TAI for time " << mjd_tai << " MJD (TAI)";
      throw std::runtime_error(os.str());
    }

    if ((!(itor->second.m_inserted)) || (itor->second.m_leap_end <= mjd_tai)) {
      return itor->second.m_time_diff;
    }
    return itor->second.m_time_diff + (itor->second.m_leap_end - mjd_tai);
  }

}

namespace timeSystem {

  const TimeSystem & TimeSystem::getSystem(const std::string & system_name) {
    static TaiSystem s_tai_system;
    static TdbSystem s_tdb_system;
    static TtSystem s_tt_system;
    static UtcSystem s_utc_system;

    return getNonConstSystem(system_name);
  }

  void TimeSystem::loadLeapSeconds(const std::string & leap_sec_file_name) {
    UtcSystem & utc_sys(dynamic_cast<UtcSystem &>(getNonConstSystem("UTC")));
    utc_sys.loadLeapSecTable(leap_sec_file_name);
  }

  TimeSystem & TimeSystem::getNonConstSystem(const std::string & system_name) {
    std::string uc_system_name = system_name;
    for (std::string::iterator itor = uc_system_name.begin(); itor != uc_system_name.end(); ++itor) *itor = toupper(*itor);
    container_type & container(getContainer());

    container_type::iterator itor = container.find(uc_system_name);
    if (container.end() == itor) throw std::runtime_error("TimeSystem::getSystem could not find time system " + system_name);
    return *itor->second;
  }

  TimeSystem::container_type & TimeSystem::getContainer() {
    static container_type s_prototype;
    return s_prototype;
  }

  TimeSystem::TimeSystem(const std::string & system_name): m_system_name(system_name) {
    std::string uc_system_name = system_name;
    for (std::string::iterator itor = uc_system_name.begin(); itor != uc_system_name.end(); ++itor) *itor = toupper(*itor);
    getContainer()[uc_system_name] = this;
  }

  TimeSystem::~TimeSystem() {}

  std::string TimeSystem::getName() const {
    return m_system_name;
  }

  void TimeSystem::write(st_stream::OStream & os) const { os << m_system_name; }

  st_stream::OStream & operator <<(st_stream::OStream & os, const TimeSystem & sys) {
    sys.write(os);
    return os;
  }

}
