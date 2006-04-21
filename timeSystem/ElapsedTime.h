/** \file ElapsedTime.h
    \brief Declaration of ElapsedTime class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_ElapsedTime_h
#define timeSystem_ElapsedTime_h

#include "st_stream/Stream.h"

#include <string>

namespace timeSystem {

  class AbsoluteTime;
  class Duration;
  class TimeSystem;

  /** \class ElapsedTime
      \brief Class which represents a duration of time measured in a particular time system ("delta T"). By their nature,
             objects of this class cannot be converted to other time systems in a physically meaningful way, but can only
             be added to other objects which were computed in the same time system. Note this seems similar to TimeInterval
             but it is very different because ElapsedTime is associated with one particular TimeSystem, but not associated
             with any absolute moment in time. By contrast TimeInterval represents the interval between two specific absolute
             times, but it may be expressed in any TimeSystem.
  */
  class ElapsedTime {
    public:
      ElapsedTime(const std::string & time_system_name, const Duration & time);

      AbsoluteTime operator +(const AbsoluteTime & absolute_time) const;

      ElapsedTime operator -() const;

      Duration getTime() const;

      void setTime(const Duration & time);

      void write(st_stream::OStream & os) const;

    protected:
      ElapsedTime(const TimeSystem * time_system, const Duration & time);

    private:
      // These are not implemented because it is ambiguous whether the ElapsedTime object should be added to the first or last
      // time in the TimeInterval.
      // TimeInterval operator +(const TimeInterval &) const;
      // TimeInterval operator -(const TimeInterval &) const;
      const TimeSystem * m_time_system;
      Duration m_time;
  };

  st_stream::OStream & operator <<(st_stream::OStream & os, const ElapsedTime & time);
}

#endif
