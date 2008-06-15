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
#include "timeSystem/TimeFormat.h"
#include "timeSystem/TimeSystem.h"

namespace timeSystem {

  AbsoluteTime::AbsoluteTime(const std::string & time_system_name, long origin_mjd, const Duration & elapsed_time):
    m_time_system(&TimeSystem::getSystem(time_system_name)), m_moment(origin_mjd, elapsed_time) {
  }

  AbsoluteTime::AbsoluteTime(const std::string & time_system_name, long mjd_day, double mjd_sec):
    m_time_system(&TimeSystem::getSystem(time_system_name)), m_moment(mjd_day, Duration(mjd_sec, "Sec")) {
  }

  void AbsoluteTime::set(const std::string & time_system_name, const std::string & time_format_name, const std::string & time_string) {
    m_time_system = &TimeSystem::getSystem(time_system_name);
    const TimeFormat & time_format = TimeFormat::getFormat(time_format_name);
    datetime_type datetime = time_format.parse(time_string);
    m_moment = moment_type(datetime.first, Duration(datetime.second, "Sec"));
  }

  std::string AbsoluteTime::represent(const std::string & time_system_name, const std::string & time_format_name,
    std::streamsize precision) const {
    const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
    moment_type moment = time_system.convertFrom(*m_time_system, m_moment);
    datetime_type datetime = time_system.computeDateTime(moment);
    const TimeFormat & time_format = TimeFormat::getFormat(time_format_name);
    return time_format.format(datetime, precision) + " (" + time_system.getName() + ")";
  }

  AbsoluteTime AbsoluteTime::operator +(const ElapsedTime & elapsed_time) const { return elapsed_time + *this; }

  AbsoluteTime AbsoluteTime::operator -(const ElapsedTime & elapsed_time) const { return -elapsed_time + *this; }

  AbsoluteTime & AbsoluteTime::operator +=(const ElapsedTime & elapsed_time) { *this = elapsed_time + *this; return *this; }

  AbsoluteTime & AbsoluteTime::operator -=(const ElapsedTime & elapsed_time) { *this = -elapsed_time + *this; return *this; }

  TimeInterval AbsoluteTime::operator -(const AbsoluteTime & time) const { return TimeInterval(time, *this); }

  bool AbsoluteTime::operator >(const AbsoluteTime & other) const {
    moment_type other_moment = m_time_system->convertFrom(*other.m_time_system, other.m_moment);
    return m_time_system->computeTimeDifference(m_moment, other_moment) > Duration::zero();
  }

  bool AbsoluteTime::operator >=(const AbsoluteTime & other) const {
    moment_type other_moment = m_time_system->convertFrom(*other.m_time_system, other.m_moment);
    return m_time_system->computeTimeDifference(m_moment, other_moment) >= Duration::zero();
  }

  bool AbsoluteTime::operator <(const AbsoluteTime & other) const {
    moment_type other_moment = m_time_system->convertFrom(*other.m_time_system, other.m_moment);
    return m_time_system->computeTimeDifference(m_moment, other_moment) < Duration::zero();
  }

  bool AbsoluteTime::operator <=(const AbsoluteTime & other) const {
    moment_type other_moment = m_time_system->convertFrom(*other.m_time_system, other.m_moment);
    return m_time_system->computeTimeDifference(m_moment, other_moment) <= Duration::zero();
  }

  bool AbsoluteTime::equivalentTo(const AbsoluteTime & other, const ElapsedTime & tolerance) const {
    return (*this > other ? (*this <= other + tolerance) : (other <= *this + tolerance));
  }

  ElapsedTime AbsoluteTime::computeElapsedTime(const std::string & time_system_name, const AbsoluteTime & since) const {
    const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
    // Convert both times into the given time system.
    moment_type minuend = time_system.convertFrom(*m_time_system, m_moment);
    moment_type subtrahend = time_system.convertFrom(*(since.m_time_system), since.m_moment);

    // Subtract the subtahend from the minuend.
    Duration time_diff = time_system.computeTimeDifference(minuend, subtrahend);
    return ElapsedTime(time_system_name, time_diff);
  }

  AbsoluteTime AbsoluteTime::computeAbsoluteTime(const std::string & time_system_name, const Duration & delta_t) const {
    // Convert this time to a corresponding time in time_system
    const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
    moment_type moment = time_system.convertFrom(*m_time_system, m_moment);

    // Add delta_t in time_system.
    moment.second += delta_t;

    // Return this time expressed as a new absolute time in the input time system.
    return AbsoluteTime(time_system_name, moment.first, moment.second);
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
