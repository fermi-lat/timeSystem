/** \file TimeInterval.h
    \brief Declaration of TimeInterval class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_TimeInterval_h
#define timeSystem_TimeInterval_h

#include <string>

namespace timeSystem {

  class AbsoluteTime;
  class ElapsedTime;

  /** \class TimeInterval
      \brief Class which represents a time difference between two specific absolute moments in time. Objects of
             this class can be converted between different time systems, but other computations are physically
             meaningless. Note this seems similar to ElapsedTime but it is very different because ElapsedTime is
             associated with one particular TimeSystem, but not associated with any absolute moment in time.
             By contrast TimeInterval represents the interval between two specific absolute times, but it may be
             expressed in any TimeSystem.
  */
  class TimeInterval {
    public:
      /** \brief Construct a TimeInterval object.
          \param  abs_time1 Start time of a time interval to create.
          \param  abs_time2 Stop time of a time interval to create.
      */
      TimeInterval(const AbsoluteTime & abs_time1, const AbsoluteTime & abs_time2);

      /** \brief Compute an elapsed time of this time interval in a given time system, and return an ElapsedTime object.
          \param time_system_name Name of the time system in which an elapsed time is computed.
      */
      ElapsedTime computeElapsedTime(const std::string & time_system_name) const;

      /** \brief Compute time duration of this time interval in a given time system and a given time unit.
          \param time_system_name Name of the time system in which time duration is computed.
          \param time_unit_name Name of the time unit in which time duration is computed.
          \param time_value_int Integer part of the computed time duration is set to this argument as a return value.
          \param time_value_frac Fractional part of the computed time duration is set to this argument as a return value.
      */
      void computeDuration(const std::string & time_system_name, const std::string & time_unit_name, long & time_value_int,
        double & time_value_frac) const;

      /** \brief Compute time duration of this time interval in a given time system and a given time unit.
          \param time_system_name Name of the time system in which time duration is computed.
          \param time_unit_name Name of the time unit in which time duration is computed.
          \param time_value The computed time duration is set to this argument as a return value.
      */
      void computeDuration(const std::string & time_system_name, const std::string & time_unit_name, double & time_value) const;

      /** \brief Compute time duration of this time interval in a given time system and a given time unit, and return it.
          \param time_system_name Name of the time system in which time duration is computed.
          \param time_unit_name Name of the time unit in which time duration is computed.
      */
      double computeDuration(const std::string & time_system_name, const std::string & time_unit_name) const;

      /** \brief Compute time duration of this time interval in a given time system, and return it.
          \param time_system_name Name of the time system in which time duration is computed.
      */
      Duration computeDuration(const std::string & time_system_name) const;

    private:
      // Prohibited operations:
      // These operations are not physical because TimeInterval is "anchored" to its endpoints, which are absolute moments
      // in time. In general, endpoints of the two TimeIntervals do not coincide, making it meaningless to perform arithmetic
      // with them.
      // TimeInterval operator +(const TimeInterval &) const;
      // TimeInterval operator -(const TimeInterval &) const;
      //
      // These are not implemented because it is ambiguous whether the ElapsedTime object should be added to the first or last
      // time in the TimeInterval.
      // TimeInterval operator +(const ElapsedTime &) const;
      // TimeInterval operator -(const ElapsedTime &) const;
      AbsoluteTime m_abs_time1;
      AbsoluteTime m_abs_time2;
  };

}

#endif
