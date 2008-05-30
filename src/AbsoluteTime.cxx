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

  AbsoluteTime::AbsoluteTime(const std::string & time_system_name, long mjd_day, double mjd_sec):
    m_time_system(&TimeSystem::getSystem(time_system_name)), m_moment(0, 0.) {
    // Rationalize the given MJD time in the given time system.
    m_moment = m_time_system->computeAdvancedTime(moment_type(mjd_day, 0.), Duration(0, mjd_sec));
  }

  AbsoluteTime::AbsoluteTime(const std::string & time_system_name, const Mjd & mjd) { set(time_system_name, mjd); }
  AbsoluteTime::AbsoluteTime(const std::string & time_system_name, const Mjd1 & mjd) { set(time_system_name, mjd); }

  void AbsoluteTime::get(const std::string & time_system_name, Mjd & mjd) const {
    const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
    moment_type moment = time_system.convertFrom(*m_time_system, m_moment);
    const MjdFormat & time_format(MjdFormat::getMjdFormat());
    time_format.convert(moment, mjd.m_int, mjd.m_frac);
  }

  void AbsoluteTime::get(const std::string & time_system_name, Mjd1 & mjd) const {
    const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
    moment_type moment = time_system.convertFrom(*m_time_system, m_moment);
    const MjdFormat & time_format(MjdFormat::getMjdFormat());
    time_format.convert(moment, mjd.m_day);
  }

  void AbsoluteTime::set(const std::string & time_system_name, const Mjd & mjd) {
    m_time_system = &TimeSystem::getSystem(time_system_name);
    const MjdFormat & time_format(MjdFormat::getMjdFormat());
    time_format.convert(mjd.m_int, mjd.m_frac, m_moment);
  }

  void AbsoluteTime::set(const std::string & time_system_name, const Mjd1 & mjd) {
    m_time_system = &TimeSystem::getSystem(time_system_name);
    const MjdFormat & time_format(MjdFormat::getMjdFormat());
    time_format.convert(mjd.m_day, m_moment);
  }

  void AbsoluteTime::set(const std::string & time_system_name, const std::string & time_format_name, const std::string & time_string) {
    m_time_system = &TimeSystem::getSystem(time_system_name);
    const TimeFormat & time_format = TimeFormat::getFormat(time_format_name);
    m_moment = time_format.parse(time_string);
  }

  std::string AbsoluteTime::represent(const std::string & time_system_name, const std::string & time_format_name,
    std::streamsize precision) const {
    const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
    moment_type moment = time_system.convertFrom(*m_time_system, m_moment);
    const TimeFormat & time_format = TimeFormat::getFormat(time_format_name);
    return time_format.format(moment, precision) + " (" + time_system.getName() + ")";
  }

  AbsoluteTime AbsoluteTime::operator +(const ElapsedTime & elapsed_time) const { return elapsed_time + *this; }

  AbsoluteTime AbsoluteTime::operator -(const ElapsedTime & elapsed_time) const { return -elapsed_time + *this; }

  AbsoluteTime & AbsoluteTime::operator +=(const ElapsedTime & elapsed_time) { *this = elapsed_time + *this; return *this; }

  AbsoluteTime & AbsoluteTime::operator -=(const ElapsedTime & elapsed_time) { *this = -elapsed_time + *this; return *this; }

  TimeInterval AbsoluteTime::operator -(const AbsoluteTime & time) const { return TimeInterval(time, *this); }

  bool AbsoluteTime::operator >(const AbsoluteTime & other) const {
    moment_type other_moment = m_time_system->convertFrom(*other.m_time_system, other.m_moment);
    return m_moment.first > other_moment.first || (m_moment.first == other_moment.first && m_moment.second > other_moment.second);
  }

  bool AbsoluteTime::operator >=(const AbsoluteTime & other) const {
    moment_type other_moment = m_time_system->convertFrom(*other.m_time_system, other.m_moment);
    return m_moment.first > other_moment.first || (m_moment.first == other_moment.first && m_moment.second >= other_moment.second);
  }

  bool AbsoluteTime::operator <(const AbsoluteTime & other) const {
    moment_type other_moment = m_time_system->convertFrom(*other.m_time_system, other.m_moment);
    return m_moment.first < other_moment.first || (m_moment.first == other_moment.first && m_moment.second < other_moment.second);
  }

  bool AbsoluteTime::operator <=(const AbsoluteTime & other) const {
    moment_type other_moment = m_time_system->convertFrom(*other.m_time_system, other.m_moment);
    return m_moment.first < other_moment.first || (m_moment.first == other_moment.first && m_moment.second <= other_moment.second);
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
    moment_type moment1 = time_system.convertFrom(*m_time_system, m_moment);

    // Add delta_t in time_system.
    moment_type moment2 = time_system.computeAdvancedTime(moment1, delta_t);

    // Return this time expressed as a new absolute time in the input time system.
    return AbsoluteTime(time_system_name, moment2.first, moment2.second);
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
