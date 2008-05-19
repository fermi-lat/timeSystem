/** \file TimeInterval.cxx
    \brief Implementation of TimeInterval class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/AbsoluteTime.h"
#include "timeSystem/ElapsedTime.h"
#include "timeSystem/TimeInterval.h"

namespace timeSystem {

  TimeInterval::TimeInterval(const AbsoluteTime & time1, const AbsoluteTime & time2): m_time1(time1), m_time2(time2) {}

  ElapsedTime TimeInterval::computeElapsedTime(const std::string & time_system_name) const {
    return m_time2.computeElapsedTime(time_system_name, m_time1);
  }
}
