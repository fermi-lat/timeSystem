/** \file TimeRep.cxx
    \brief Implementation of TimeRep and related classes.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/AbsoluteTime.h"
#include "timeSystem/TimeRep.h"
#include "timeSystem/TimeSystem.h"

#include <iomanip>
#include <limits>
#include <sstream>

namespace {
  int s_digits = std::numeric_limits<double>::digits10;
}

namespace timeSystem {

  TimeRep::~TimeRep() {}

  void TimeRep::setAbsoluteTime(const AbsoluteTime & time) { time.getTime(*this); }


  MetRep::MetRep(const std::string & system_name, long mjd_ref_int, double mjd_ref_frac, double met):
    m_system(&TimeSystem::getSystem(system_name)), m_mjd_ref(IntFracPair(mjd_ref_int, mjd_ref_frac), Day), m_met(met) {}

  AbsoluteTime MetRep::getTime() const { return AbsoluteTime(m_system->getName(), m_mjd_ref, Duration(0, m_met)); }

  void MetRep::setTime(const TimeSystem & system, const Duration & origin, const Duration & elapsed) {
    // Convert from the given time into "this" system.
    Moment my_time = m_system->convertFrom(system, Moment(origin, elapsed));

    // Now compute met from my_time in this system.
    Duration met = my_time.second + m_system->computeTimeDifference(my_time.first, m_mjd_ref);
    IntFracPair met_pair = met.getValue(Sec);
    m_met = met_pair.getIntegerPart() + met_pair.getFractionalPart();
  }

  std::string MetRep::getString() const {
    std::ostringstream os;
    os << std::setprecision(s_digits) << getValue() << " MET (" << *m_system << ") [MJDREF=" << m_mjd_ref.getValue(Day) << "]";
    return os.str();
  }

  double MetRep::getValue() const { return m_met; }

  void MetRep::setValue(double met) { m_met = met; }


  MjdRep::MjdRep(const std::string & system_name, long mjd_int, double mjd_frac):
    m_system(&TimeSystem::getSystem(system_name)), m_mjd(IntFracPair(mjd_int, mjd_frac), Day) {}

  AbsoluteTime MjdRep::getTime() const { return AbsoluteTime(m_system->getName(), m_mjd, Duration(0, 0.)); }

  void MjdRep::setTime(const TimeSystem & system, const Duration & origin, const Duration & elapsed) {
    // Convert from the given time into "this" system.
    Moment my_time = m_system->convertFrom(system, Moment(origin, elapsed));

    // Now compute mjd from my_time in this system.
    m_mjd = m_system->computeMjd(my_time);
  }

  std::string MjdRep::getString() const {
    std::ostringstream os;
    os << std::setprecision(s_digits) << getValue() << " MJD (" << *m_system << ")";
    return os.str();
  }

  IntFracPair MjdRep::getValue() const { return m_mjd.getValue(Day); }

  void MjdRep::setValue(long mjd_int, double mjd_frac) { m_mjd = Duration(IntFracPair(mjd_int, mjd_frac), Day); }

}
