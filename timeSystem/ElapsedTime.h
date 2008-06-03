/** \file ElapsedTime.h
    \brief Declaration of ElapsedTime class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_ElapsedTime_h
#define timeSystem_ElapsedTime_h

#include "st_stream/Stream.h"

#include "timeSystem/TimeSystem.h"

#include <string>

namespace timeSystem {

  class AbsoluteTime;
  class Duration;

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
      ElapsedTime(const std::string & time_system_name, const Duration & time_duration);

      AbsoluteTime operator +(const AbsoluteTime & absolute_time) const;

      ElapsedTime operator -() const;

      const timeSystem::TimeSystem & getSystem() const;

      void getDuration(const std::string & time_unit_name, long & time_value_int, double & time_value_frac) const;

      void getDuration(const std::string & time_unit_name, double & time_value) const;

      double getDuration(const std::string & time_unit_name) const;

      Duration getDuration() const;

      template <typename StreamType>
      void write(StreamType & os) const;

    protected:
      ElapsedTime(const TimeSystem * time_system, const Duration & time_duration);

    private:
      // These are not implemented because it is ambiguous whether the ElapsedTime object should be added to the first or last
      // time in the TimeInterval.
      // TimeInterval operator +(const TimeInterval &) const;
      // TimeInterval operator -(const TimeInterval &) const;
      const TimeSystem * m_time_system;
      Duration m_duration;
  };

  template <typename StreamType>
  inline void ElapsedTime::write(StreamType & os) const {
    os << m_duration << " (" << *m_time_system << ")";
  }

  std::ostream & operator <<(std::ostream & os, const ElapsedTime & elapsed_time);

  st_stream::OStream & operator <<(st_stream::OStream & os, const ElapsedTime & elapsed_time);
}

#endif
