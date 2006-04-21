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
      TimeInterval(const AbsoluteTime & time1, const AbsoluteTime & time2);

      ElapsedTime computeElapsedTime(const std::string & time_system_name) const;

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
      AbsoluteTime m_time1;
      AbsoluteTime m_time2;
  };

}

#endif
