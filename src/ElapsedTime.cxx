/** \file ElapsedTime.cxx
    \brief Implementation of ElapsedTime class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/AbsoluteTime.h"
#include "timeSystem/Duration.h"
#include "timeSystem/ElapsedTime.h"

namespace timeSystem {

  ElapsedTime::ElapsedTime(const std::string & time_system_name, const Duration & time_duration):
    m_time_system(&TimeSystem::getSystem(time_system_name)), m_duration(time_duration) {}

  ElapsedTime::ElapsedTime(const TimeSystem * time_system, const Duration & time_duration):
    m_time_system(time_system), m_duration(time_duration) {}

  AbsoluteTime ElapsedTime::operator +(const AbsoluteTime & absolute_time) const {
    return absolute_time.computeAbsoluteTime(m_time_system->getName(), m_duration);
  }

  ElapsedTime ElapsedTime::operator -() const { return ElapsedTime(m_time_system, -m_duration); }

  const timeSystem::TimeSystem & ElapsedTime::getSystem() const { return *m_time_system; }

  void ElapsedTime::getDuration(const std::string & time_unit_name, long & time_value_int, double & time_value_frac) const {
    m_duration.get(time_unit_name, time_value_int, time_value_frac);
  }

  void ElapsedTime::getDuration(const std::string & time_unit_name, double & time_value) const {
    m_duration.get(time_unit_name, time_value);
  }

  double ElapsedTime::getDuration(const std::string & time_unit_name) const {
    return m_duration.get(time_unit_name);
  }

  Duration ElapsedTime::getDuration() const { return m_duration; }

  std::ostream & operator <<(std::ostream & os, const ElapsedTime & elapsed_time) {
    elapsed_time.write(os);
    return os;
  }

  st_stream::OStream & operator <<(st_stream::OStream & os, const ElapsedTime & elapsed_time) {
    elapsed_time.write(os);
    return os;
  }

}
