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

  };

  class TdbSystem : public TimeSystem {
    public:
      TdbSystem(): TimeSystem("TDB") {}

      virtual Duration convertFrom(const TimeSystem & time_system, const Duration & origin, const Duration & time) const;

      Duration computeTdbMinusTt(const Duration & mjd) const;

  };

  class TtSystem : public TimeSystem {
    public:
      TtSystem(): TimeSystem("TT") {}

      virtual Duration convertFrom(const TimeSystem & time_system, const Duration & origin, const Duration & time) const;

  };

  class UtcSystem : public TimeSystem {
    public:
      UtcSystem();

      virtual Duration convertFrom(const TimeSystem & time_system, const Duration & origin, const Duration & time) const;

      Duration computeTaiMinusUtc(const Duration & mjd_utc) const;

      Duration computeUtcMinusTai(const Duration & mjd_tai) const;

    private:
      typedef std::map<Duration, Duration> leaptable_type;
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
        const TimeSystem & utc(getSystem("UTC"));
        const UtcSystem * utc_system = dynamic_cast<const UtcSystem *>(&utc);
        return time + utc_system->computeTaiMinusUtc(origin);
      } else {
        throw std::logic_error("Conversion from " + time_system.getName() + " to " + getName() + " is not implemented");
      }
    }
    return time;
  }

  Duration TdbSystem::computeTdbMinusTt(const Duration & mjd_tt) const {
    static const long jd_minus_mjd_int = 2400000;
    static const double jd_minus_mjd_frac = .5;

    time_type mjd_time = mjd_tt.day();

    return Duration(0, ctatv(mjd_time.first + jd_minus_mjd_int, mjd_time.second + jd_minus_mjd_frac));

#if 0
    // Algorithm taken from function TTtoTDB() in bary.c by Arnold Rots.
    static double tdbtdt ;
    static double tdbtdtdot ;
    static long oldmjd = 0 ;

    if ( mjd_time.first != oldmjd ) {
      oldmjd = mjd_time.first;
      long l = oldmjd + 2400001 ;

      tdbtdt = ctatv (l, 0.0) ;
      tdbtdtdot = ctatv (l, 0.5) - ctatv (l, -0.5) ;
    }

    double diff = tdbtdt + (mjd_time.second - 0.5) * tdbtdtdot;

    return Duration(0, diff);
#endif
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

  UtcSystem::UtcSystem(): TimeSystem("UTC") {
    using namespace st_facilities;
    // Location of leap sec table.
    std::string leap_sec_file_name = Env::appendFileName(Env::getEnv("TIMING_DIR"), "leapsec.fits");

    // Read MJD and number of leap seconds from table.
    std::auto_ptr<const tip::Table> leap_sec_table(tip::IFileSvc::instance().readTable(leap_sec_file_name, "1"));
    double cumulative_leap_sec = 10.;
    for (tip::Table::ConstIterator itor = leap_sec_table->begin(); itor != leap_sec_table->end(); ++itor) {
      // Read the MJD and LEAPSECS from the table.
      double mjd_dbl = (*itor)["MJD"].get();
      cumulative_leap_sec += (*itor)["LEAPSECS"].get();

      // Make sure the MJD for the leap second is a whole number of days.
      long mjd = long(mjd_dbl + .5);
      if (mjd != mjd_dbl) throw std::logic_error("UtcSystem: leapsec.fits unexpectedly contained a non-integral MJD value");

      m_tai_minus_utc[Duration(mjd, 0.)] = Duration(0, cumulative_leap_sec);
    }

    // Create a reverse conversion table.
    for (leaptable_type::iterator itor = m_tai_minus_utc.begin(); itor != m_tai_minus_utc.end(); ++itor)
      m_utc_minus_tai[(itor->first + itor->second)] = -(itor->second);

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

  Duration UtcSystem::computeTaiMinusUtc(const Duration & mjd_utc) const {
    // Start from the end of the leap second table and go backwards, stopping at the first leap time
    // which is <= the given time.
    leaptable_type::const_reverse_iterator itor = m_tai_minus_utc.rbegin();
    for (; (itor != m_tai_minus_utc.rend()) && (mjd_utc < itor->first); ++itor) {}
    if (itor == m_tai_minus_utc.rend()) {
      // Fell of the rend (that is the beginning) so this time is too early.
      std::ostringstream os;
      os << "UtcSystem::computeTaiMinusUtc cannot convert a time before " << m_tai_minus_utc.begin()->first << " MJD (UTC)";
      throw std::runtime_error(os.str());
    }
    return itor->second;
  }

  Duration UtcSystem::computeUtcMinusTai(const Duration & mjd_tai) const {
    // Start from the end of the leap second table and go backwards, stopping at the first leap time
    // which is <= the given time.
    leaptable_type::const_reverse_iterator itor = m_utc_minus_tai.rbegin();
    for (; (itor != m_utc_minus_tai.rend()) && (mjd_tai < itor->first); ++itor) {}
    if (itor == m_utc_minus_tai.rend()) {
      // Fell of the rend (that is the beginning) so this time is too early.
      std::ostringstream os;
      os << "UtcSystem::computeUtcMinusTai cannot convert a time before " << m_utc_minus_tai.begin()->first << " MJD (TAI)";
      throw std::runtime_error(os.str());
    }
    return itor->second;
  }

}

namespace timeSystem {

  const TimeSystem & TimeSystem::getSystem(const std::string & system_name) {
    std::string uc_system_name = system_name;
    for (std::string::iterator itor = uc_system_name.begin(); itor != uc_system_name.end(); ++itor) *itor = toupper(*itor);
    container_type & container(getContainer());

    static TaiSystem s_tai_system;
    static TdbSystem s_tdb_system;
    static TtSystem s_tt_system;
    static UtcSystem s_utc_system;

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
