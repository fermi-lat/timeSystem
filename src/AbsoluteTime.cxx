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
#include "timeSystem/TimeSystem.h"

namespace timeSystem {

  AbsoluteTime::AbsoluteTime(const std::string & time_system_name, const Duration & origin, const Duration & time):
    m_time_system(&TimeSystem::getSystem(time_system_name)), m_time(origin, time) {}

  AbsoluteTime AbsoluteTime::operator +(const ElapsedTime & elapsed_time) const { return elapsed_time + *this; }

  AbsoluteTime AbsoluteTime::operator -(const ElapsedTime & elapsed_time) const { return -elapsed_time + *this; }

  TimeInterval AbsoluteTime::operator -(const AbsoluteTime & time) const { return TimeInterval(time, *this); }

  bool AbsoluteTime::operator >(const AbsoluteTime & other) const {
    Moment other_time = m_time_system->convertFrom(*other.m_time_system, other.m_time);
    return m_time.second > other_time.second + m_time_system->computeTimeDifference(other_time.first, m_time.first);
  }

  bool AbsoluteTime::operator >=(const AbsoluteTime & other) const {
    Moment other_time = m_time_system->convertFrom(*other.m_time_system, other.m_time);
    return m_time.second >= other_time.second + m_time_system->computeTimeDifference(other_time.first, m_time.first);
  }

  bool AbsoluteTime::operator <(const AbsoluteTime & other) const {
    Moment other_time = m_time_system->convertFrom(*other.m_time_system, other.m_time);
    return m_time.second < other_time.second + m_time_system->computeTimeDifference(other_time.first, m_time.first);
  }

  bool AbsoluteTime::operator <=(const AbsoluteTime & other) const {
    Moment other_time = m_time_system->convertFrom(*other.m_time_system, other.m_time);
    return m_time.second <= other_time.second + m_time_system->computeTimeDifference(other_time.first, m_time.first);
  }

  bool AbsoluteTime::equivalentTo(const AbsoluteTime & other, const ElapsedTime & tolerance) const {
    return (*this > other ? (*this <= other + tolerance) : (other <= *this + tolerance));
  }

  Duration AbsoluteTime::getTime() const { return m_time.second; }

  void AbsoluteTime::setTime(const Duration & time) { m_time.second = time; }

  ElapsedTime AbsoluteTime::computeElapsedTime(const std::string & time_system_name, const AbsoluteTime & since) const {
    const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
    // Convert both times into the given time system.
    Moment minuend = time_system.convertFrom(*m_time_system, m_time);
    Moment subtrahend = time_system.convertFrom(*(since.m_time_system), since.m_time);

    // Subtract the subtahend from the minuend.
    return ElapsedTime(time_system_name, minuend.second - subtrahend.second
                                         + m_time_system->computeTimeDifference(minuend.first, subtrahend.first));
  }

  AbsoluteTime AbsoluteTime::computeAbsoluteTime(const std::string & time_system_name, const Duration & delta_t) const {
    const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
    // Convert this time to a corresponding time in time_system
    Moment time1 = time_system.convertFrom(*m_time_system, m_time);

    // Add delta_t in time_system.
    time1.second += delta_t;

    // Convert time1 to a corresponding time in *m_time_system.
    Moment time2 = m_time_system->convertFrom(time_system, time1);

    // Create an absolute time to return
    //return AbsoluteTime(m_time_system->getName(), time2.first, time2.second);
    // TODO: replace below with above after TimeFormat is added to AbsoluteTime.
    return AbsoluteTime(m_time_system->getName(), m_time.first, time2.second + m_time_system->computeTimeDifference(time2.first, m_time.first));
  }

  void AbsoluteTime::write(st_stream::OStream & os) const {
    // "123 days 456.789 seconds since 54321.987 MJD (TDB)"
    os << m_time.second << " since " << m_time.first << " MJD (" << m_time_system->getName() << ")";
  }

  st_stream::OStream & operator <<(st_stream::OStream & os, const AbsoluteTime & time) {
    time.write(os);
    return os;
  }
}
