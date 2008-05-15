/** \file AbsoluteTime.h
    \brief Declaration of AbsoluteTime class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_AbsoluteTime_h
#define timeSystem_AbsoluteTime_h

#include <string>

#include "timeSystem/Duration.h"

namespace st_stream {
  class OStream;
}

namespace timeSystem {

  class ElapsedTime;
  class TimeInterval;
  class TimeRep;
  class TimeSystem;

  /** \class AbsoluteTime
      \brief Class which represents an absolute moment in time, expressed as a time elapsed from a specific MJD time, in
             a particular time unit in a particular time system. This class transparently handles all conversions between time
             units and systems. Thus clients can use objects of this class in calculations without explicit knowledge
             of their units or systems.
  */
  class AbsoluteTime {
    public:
      AbsoluteTime(const std::string & time_system_name, const Duration & origin, const Duration & time);

      AbsoluteTime(const TimeRep & rep);

      AbsoluteTime operator +(const ElapsedTime & elapsed_time) const;

      AbsoluteTime operator -(const ElapsedTime & elapsed_time) const;

      AbsoluteTime & operator +=(const ElapsedTime & elapsed_time);

      AbsoluteTime & operator -=(const ElapsedTime & elapsed_time);

      TimeInterval operator -(const AbsoluteTime & time) const;

      bool operator >(const AbsoluteTime & other) const;

      bool operator >=(const AbsoluteTime & other) const;

      bool operator <(const AbsoluteTime & other) const;

      bool operator <=(const AbsoluteTime & other) const;

      bool equivalentTo(const AbsoluteTime & other, const ElapsedTime & tolerance) const;

      void exportTimeRep(TimeRep & rep) const;

      AbsoluteTime computeAbsoluteTime(const std::string & time_system_name, const Duration & delta_t) const;

      ElapsedTime computeElapsedTime(const std::string & time_system_name, const AbsoluteTime & since) const;

      template <typename StreamType>
      void write(StreamType & os) const;

    private:
      void importTimeRep(const TimeRep & rep);

      // TODO: Remove convert methods below, by modifying TimeSystem class to accept a moment_type object.
      Moment convert(const moment_type & time) const;
      moment_type convert(const Moment & time) const;

      // Prohibited operations:
      // These are not physical because TimeInterval is "anchored" to its endpoints, which are absolute moments in time.
      // In general, neither endpoint of the TimeInterval is the same as "this" AbsoluteTime. Note that similar operators
      // which use ElapsedTime are provided.
      // AbsoluteTime operator +(const TimeInterval &) const;
      // AbsoluteTime operator -(const TimeInterval &) const;
      const TimeSystem * m_time_system;
      moment_type m_moment;
  };

  template <typename StreamType>
  inline void AbsoluteTime::write(StreamType & os) const {
    // "123 days 456.789 seconds since 54321.987 MJD (TDB)"
    Moment this_time = convert(m_moment);
    os << this_time.second << " since " << this_time.first << " MJD (" << *m_time_system << ")";
  }

  std::ostream & operator <<(std::ostream & os, const AbsoluteTime & time);

  st_stream::OStream & operator <<(st_stream::OStream & os, const AbsoluteTime & time);

}

#endif
