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
    m_time_system(&TimeSystem::getSystem(time_system_name)), m_moment(mjd_day, mjd_sec) {}

  AbsoluteTime::AbsoluteTime(const std::string & time_system_name, const Mjd & mjd) { set(time_system_name, mjd); }
  AbsoluteTime::AbsoluteTime(const std::string & time_system_name, const Mjd1 & mjd) { set(time_system_name, mjd); }

  void AbsoluteTime::get(const std::string & time_system_name, Mjd & mjd) const {
    const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
    moment_type moment = convert(time_system, time_system.convertFrom(*m_time_system, convert(m_moment)));
    const MjdFormat & time_format(MjdFormat::getMjdFormat());
    time_format.convert(moment, mjd.m_int, mjd.m_frac);
  }

  void AbsoluteTime::get(const std::string & time_system_name, Mjd1 & mjd) const {
    const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
    moment_type moment = convert(time_system, time_system.convertFrom(*m_time_system, convert(m_moment)));
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
    moment_type moment = convert(time_system, time_system.convertFrom(*m_time_system, convert(m_moment)));
    const TimeFormat & time_format = TimeFormat::getFormat(time_format_name);
    return time_format.format(moment, precision) + " (" + time_system.getName() + ")";
  }

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
    // Convert this time to a corresponding time in time_system
    const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
    Moment time1 = time_system.convertFrom(*m_time_system, convert(m_moment));

    // Add delta_t in time_system.
    time1.second += delta_t;

    // Return this time expressed as a new absolute time in the input time system.
    moment_type moment(convert(time_system, time1));
    return AbsoluteTime(time_system_name, moment.first, moment.second);
  }

  Moment AbsoluteTime::convert(const moment_type & time) const {
    return Moment(Duration(time.first, 0.), Duration(0, time.second));
  }

  moment_type AbsoluteTime::convert(const TimeSystem & time_system, const Moment & time) const {
    // Compute MJD.
    Duration mjd = time_system.computeMjd(time);
    IntFracPair mjd_int_frac = mjd.getValue(Day);
    long mjd_day = mjd_int_frac.getIntegerPart();
    Duration elapsed = time.second + time_system.computeTimeDifference(time.first, Duration(mjd_day, 0.));
    double mjd_sec = elapsed.getValue(Sec).getDouble();

    // Take care of an inserted leap second.
    if (mjd_sec < 0.) {
      --mjd_day;
      elapsed = time.second + time_system.computeTimeDifference(time.first, Duration(mjd_day, 0.));
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
