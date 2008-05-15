/** \file AbsoluteTime.cxx
    \brief Implementation of AbsoluteTime class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "st_stream/Stream.h"

#include "timeSystem/AbsoluteTime.h"
#include "timeSystem/Duration.h"
#include "timeSystem/ElapsedTime.h"
#include "timeSystem/TimeInterval.h"
#include "timeSystem/TimeRep.h"
#include "timeSystem/TimeSystem.h"

namespace timeSystem {

  AbsoluteTime::AbsoluteTime(const std::string & time_system_name, const Duration & origin, const Duration & time):
    m_time_system(&TimeSystem::getSystem(time_system_name)), m_moment(convert(Moment(origin, time))) {}

  AbsoluteTime::AbsoluteTime(const TimeRep & rep): m_time_system(0), m_moment() { importTimeRep(rep); }

  AbsoluteTime AbsoluteTime::operator +(const ElapsedTime & elapsed_time) const { return elapsed_time + *this; }

  AbsoluteTime AbsoluteTime::operator -(const ElapsedTime & elapsed_time) const { return -elapsed_time + *this; }

  AbsoluteTime & AbsoluteTime::operator +=(const ElapsedTime & elapsed_time) { *this = elapsed_time + *this; return *this; }

  AbsoluteTime & AbsoluteTime::operator -=(const ElapsedTime & elapsed_time) { *this = -elapsed_time + *this; return *this; }

  TimeInterval AbsoluteTime::operator -(const AbsoluteTime & time) const { return TimeInterval(time, *this); }

  bool AbsoluteTime::operator >(const AbsoluteTime & other) const {
    Moment other_time = m_time_system->convertFrom(*other.m_time_system, other.convert(other.m_moment));
    Moment this_time = convert(m_moment);
    return this_time.second > other_time.second + m_time_system->computeTimeDifference(other_time.first, this_time.first);
  }

  bool AbsoluteTime::operator >=(const AbsoluteTime & other) const {
    Moment other_time = m_time_system->convertFrom(*other.m_time_system, other.convert(other.m_moment));
    Moment this_time = convert(m_moment);
    return this_time.second >= other_time.second + m_time_system->computeTimeDifference(other_time.first, this_time.first);
  }

  bool AbsoluteTime::operator <(const AbsoluteTime & other) const {
    Moment other_time = m_time_system->convertFrom(*other.m_time_system, other.convert(other.m_moment));
    Moment this_time = convert(m_moment);
    return this_time.second < other_time.second + m_time_system->computeTimeDifference(other_time.first, this_time.first);
  }

  bool AbsoluteTime::operator <=(const AbsoluteTime & other) const {
    Moment other_time = m_time_system->convertFrom(*other.m_time_system, other.convert(other.m_moment));
    Moment this_time = convert(m_moment);
    return this_time.second <= other_time.second + m_time_system->computeTimeDifference(other_time.first, this_time.first);
  }

  bool AbsoluteTime::equivalentTo(const AbsoluteTime & other, const ElapsedTime & tolerance) const {
    return (*this > other ? (*this <= other + tolerance) : (other <= *this + tolerance));
  }

  void AbsoluteTime::exportTimeRep(TimeRep & rep) const {
    Moment this_time = convert(m_moment);
    rep.set(m_time_system->getName(), this_time.first, this_time.second);
  }

  void AbsoluteTime::importTimeRep(const TimeRep & rep) {
    std::string system_name;
    Duration origin;
    Duration elapsed;
    rep.get(system_name, origin, elapsed);
    m_time_system = &TimeSystem::getSystem(system_name);
    m_moment = convert(Moment(origin, elapsed));
  }

  ElapsedTime AbsoluteTime::computeElapsedTime(const std::string & time_system_name, const AbsoluteTime & since) const {
    const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
    // Convert both times into the given time system.
    Moment minuend = time_system.convertFrom(*m_time_system, convert(m_moment));
    Moment subtrahend = time_system.convertFrom(*(since.m_time_system), since.convert(since.m_moment));

    // Subtract the subtahend from the minuend.
    return ElapsedTime(time_system_name, minuend.second - subtrahend.second
                                         + m_time_system->computeTimeDifference(minuend.first, subtrahend.first));
  }

  AbsoluteTime AbsoluteTime::computeAbsoluteTime(const std::string & time_system_name, const Duration & delta_t) const {
    const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
    // Convert this time to a corresponding time in time_system
    Moment time1 = time_system.convertFrom(*m_time_system, convert(m_moment));

    // Add delta_t in time_system.
    time1.second += delta_t;

    // Return this time expressed as a new absolute time in the input time system.
    return AbsoluteTime(time_system_name, time1.first, time1.second);
  }

  Moment AbsoluteTime::convert(const moment_type & time) const {
    return Moment(Duration(time.first, 0.), Duration(0, time.second));
  }

  moment_type AbsoluteTime::convert(const Moment & time) const {
    // Compute MJD.
    Duration mjd = m_time_system->computeMjd(time);
    IntFracPair mjd_int_frac = mjd.getValue(Day);
    long mjd_day = mjd_int_frac.getIntegerPart();
    Duration elapsed = time.second + m_time_system->computeTimeDifference(time.first, Duration(mjd_day, 0.));
    double mjd_sec = elapsed.getValue(Sec).getDouble();

    // Take care of an inserted leap second.
    if (mjd_sec < 0.) {
      --mjd_day;
      elapsed = time.second + m_time_system->computeTimeDifference(time.first, Duration(mjd_day, 0.));
      mjd_sec = elapsed.getValue(Sec).getDouble();
    }

    // Return the MJD.
    return moment_type(mjd_day, mjd_sec);
  }

  std::ostream & operator <<(std::ostream & os, const AbsoluteTime & time) {
    time.write(os);
    return os;
  }

  st_stream::OStream & operator <<(st_stream::OStream & os, const AbsoluteTime & time) {
    time.write(os);
    return os;
  }

}
