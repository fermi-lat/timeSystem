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
      /** \brief Construct an ElapsedTime object.
          \param time_system_name Name of time system in which an elapsed time is defined.
          \param time_duration Time duration of an elapsed time to be created.
      */
      ElapsedTime(const std::string & time_system_name, const Duration & time_duration);

      /** \brief Add this elapsed time to a given absolute time, and return the result.
          \param absolute_time Absolute time to add this elapsed time to.
      */
      AbsoluteTime operator +(const AbsoluteTime & absolute_time) const;

      /** \brief Create a new ElapsedTime object which represents the same elapsed time as this object, but with an opposite sign,
                 and return it.
      */
      ElapsedTime operator -() const;

      /// \brief Return the name of the time system in which this object is defined.
      const timeSystem::TimeSystem & getSystem() const;

      /** \brief Compute time duration of this object in a specified time unit, and set the result to the arguments of this method.
          \param time_unit_name Name of time unit to be used to compute time duration.
          \param time_value_int Integer part of the result (i.e., time duration in a specified time unit) is set to this argument
                 as a return value.
          \param time_value_frac Fractional part of the result (i.e., time duration in a specified time unit) is set to this
                 argument as a return value.
      */
      void getDuration(const std::string & time_unit_name, long & time_value_int, double & time_value_frac) const;

      /** \brief Compute time duration of this object in a specified time unit, and set the result to the argument of this method.
          \param time_unit_name Name of time unit to be used to compute time duration.
          \param time_value The result (i.e., time duration in a specified time unit) is set to this argument as a return value.
      */
      void getDuration(const std::string & time_unit_name, double & time_value) const;

      /** \brief Compute time duration of this object in a specified time unit, and return the result.
          \param time_unit_name Name of time unit to be used to compute time duration.
      */
      double getDuration(const std::string & time_unit_name) const;

      /// \brief Return a Duration object that represents time duration stored in this object.
      Duration getDuration() const;

      /** \brief Write a text representation of this object to a given output stream.
          \parm os Output stream to write a text representation of this object to.
      */
      template <typename StreamType>
      void write(StreamType & os) const;

    protected:
      /** \brief Construct an ElapsedTime object.
          \param time_system Pointer to a TimeSystem object to specify a time system in which an elapsed time is defined.
          \param time_duration Time duration of an elapsed time to be created.
      */
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
