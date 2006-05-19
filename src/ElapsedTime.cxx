/** \file ElapsedTime.cxx
    \brief Implementation of ElapsedTime class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/AbsoluteTime.h"
#include "timeSystem/Duration.h"
#include "timeSystem/ElapsedTime.h"

namespace timeSystem {

  ElapsedTime::ElapsedTime(const std::string & time_system_name, const Duration & time):
    m_time_system(&TimeSystem::getSystem(time_system_name)), m_time(time) {}

  ElapsedTime::ElapsedTime(const TimeSystem * time_system, const Duration & time):
    m_time_system(time_system), m_time(time) {}

  AbsoluteTime ElapsedTime::operator +(const AbsoluteTime & absolute_time) const {
    return absolute_time.computeAbsoluteTime(m_time_system->getName(), m_time);
  }

  ElapsedTime ElapsedTime::operator -() const { return ElapsedTime(m_time_system, -m_time); }

  Duration ElapsedTime::getTime() const { return m_time; }

  void ElapsedTime::setTime(const Duration & time) { m_time = time; }

  std::ostream & operator <<(std::ostream & os, const ElapsedTime & time) {
    time.write(os);
    return os;
  }

  st_stream::OStream & operator <<(st_stream::OStream & os, const ElapsedTime & time) {
    time.write(os);
    return os;
  }

}
