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
    m_time_system(&TimeSystem::getSystem(time_system_name)), m_origin(origin), m_time(time) {}

  AbsoluteTime AbsoluteTime::operator +(const ElapsedTime & elapsed_time) const { return elapsed_time + *this; }

  AbsoluteTime AbsoluteTime::operator -(const ElapsedTime & elapsed_time) const { return -elapsed_time + *this; }

  TimeInterval AbsoluteTime::operator -(const AbsoluteTime & time) const { return TimeInterval(time, *this); }

  bool AbsoluteTime::operator >(const AbsoluteTime & other) const {
    Duration other_time = m_time_system->convertFrom(*other.m_time_system, other.m_origin, other.m_time);
    return m_time + m_origin > other_time + other.m_origin;
  }

  bool AbsoluteTime::operator >=(const AbsoluteTime & other) const {
    Duration other_time = m_time_system->convertFrom(*other.m_time_system, other.m_origin, other.m_time);
    return m_time + m_origin >= other_time + other.m_origin;
  }

  bool AbsoluteTime::operator <(const AbsoluteTime & other) const {
    Duration other_time = m_time_system->convertFrom(*other.m_time_system, other.m_origin, other.m_time);
    return m_time + m_origin < other_time + other.m_origin;
  }

  bool AbsoluteTime::operator <=(const AbsoluteTime & other) const {
    Duration other_time = m_time_system->convertFrom(*other.m_time_system, other.m_origin, other.m_time);
    return m_time + m_origin <= other_time + other.m_origin;
  }

  bool AbsoluteTime::equivalentTo(const AbsoluteTime & other, const ElapsedTime & tolerance) const {
    return (*this > other ? (*this <= other + tolerance) : (other <= *this + tolerance));
  }

  Duration AbsoluteTime::getTime() const { return m_time; }

  void AbsoluteTime::setTime(const Duration & time) { m_time = time; }

  ElapsedTime AbsoluteTime::computeElapsedTime(const std::string & time_system_name, const AbsoluteTime & since) const {
    const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));

    // Convert both times into the given time system.
    Duration minuend_time = time_system.convertFrom(*m_time_system, m_origin, m_time);
    Duration subtrahend_time = time_system.convertFrom(*(since.m_time_system), since.m_origin, since.m_time);

    // Subtract the subtahend's origin and duration from the minuend's and sum the two remainders.
    return ElapsedTime(time_system_name, (m_origin - since.m_origin) + (minuend_time - subtrahend_time));
  }

  AbsoluteTime AbsoluteTime::computeAbsoluteTime(const TimeSystem & time_system, const Duration & delta_t) const {
    // Compute m_time for an absolute time to return in time_system
    Duration time1 = time_system.convertFrom(*m_time_system, m_origin, m_time) + delta_t;

    // Compute m_time for an absolute time to return in *m_time_system.
    Duration time2 = m_time_system->convertFrom(time_system, m_origin, time1);

    // Create an absolute time to return
    return AbsoluteTime(m_time_system->getName(), m_origin, time2);
  }

  void AbsoluteTime::write(st_stream::OStream & os) const {
    // "123 days 456.789 seconds since 54321.987 MJD (TDB)"
    os << m_time << " since " << m_origin << " MJD (" << m_time_system->getName() << ")";
  }

  st_stream::OStream & operator <<(st_stream::OStream & os, const AbsoluteTime & time) {
    time.write(os);
    return os;
  }
}
