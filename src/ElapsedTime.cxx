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

  auto ElapsedTime::operator +(const AbsoluteTime & absolute_time) const -> AbsoluteTime {
    return absolute_time.computeAbsoluteTime(m_time_system->getName(), m_duration);
  }

  auto ElapsedTime::operator -() const -> ElapsedTime { return ElapsedTime(m_time_system, -m_duration); }

  auto ElapsedTime::getSystem() const -> const TimeSystem & { return *m_time_system; }

  void ElapsedTime::getDuration(const std::string & time_unit_name, long & time_value_int, double & time_value_frac) const {
    m_duration.get(time_unit_name, time_value_int, time_value_frac);
  }

  void ElapsedTime::getDuration(const std::string & time_unit_name, double & time_value) const {
    m_duration.get(time_unit_name, time_value);
  }

  auto ElapsedTime::getDuration(const std::string & time_unit_name) const -> double {
    return m_duration.get(time_unit_name);
  }

  auto ElapsedTime::getDuration() const -> Duration { return m_duration; }

  auto operator <<(std::ostream & os, const ElapsedTime & elapsed_time) -> std::ostream & {
    elapsed_time.write(os);
    return os;
  }

  auto operator <<(st_stream::OStream & os, const ElapsedTime & elapsed_time) -> st_stream::OStream & {
    elapsed_time.write(os);
    return os;
  }

}
