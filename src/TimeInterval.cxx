/** \file TimeInterval.cxx
    \brief Implementation of TimeInterval class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/AbsoluteTime.h"
#include "timeSystem/ElapsedTime.h"
#include "timeSystem/TimeInterval.h"

namespace timeSystem {

  TimeInterval::TimeInterval(const AbsoluteTime & abs_time1, const AbsoluteTime & abs_time2):
    m_abs_time1(abs_time1), m_abs_time2(abs_time2) {}

  ElapsedTime TimeInterval::computeElapsedTime(const std::string & time_system_name) const {
    return m_abs_time2.computeElapsedTime(time_system_name, m_abs_time1);
  }

  void TimeInterval::computeDuration(const std::string & time_system_name, const std::string & time_unit_name, long & time_value_int,
    double & time_value_frac) const {
    computeElapsedTime(time_system_name).getDuration(time_unit_name, time_value_int, time_value_frac);
  }

  void TimeInterval::computeDuration(const std::string & time_system_name, const std::string & time_unit_name, double & time_value)
    const {
    computeElapsedTime(time_system_name).getDuration(time_unit_name, time_value);
  }

  double TimeInterval::computeDuration(const std::string & time_system_name, const std::string & time_unit_name) const {
    return computeElapsedTime(time_system_name).getDuration(time_unit_name);
  }

  Duration TimeInterval::computeDuration(const std::string & time_system_name) const {
    return computeElapsedTime(time_system_name).getDuration();
  }
}
