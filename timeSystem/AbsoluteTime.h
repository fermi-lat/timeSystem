/** \file AbsoluteTime.h
    \brief Declaration of AbsoluteTime class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_AbsoluteTime_h
#define timeSystem_AbsoluteTime_h

#include "timeSystem/TimeFormat.h"
#include "timeSystem/TimeSystem.h"

#include <limits>
#include <string>

namespace st_stream {
  class OStream;
}

namespace timeSystem {

  class ElapsedTime;
  class TimeInterval;

  /** \class AbsoluteTime
      \brief Class which represents an absolute moment in time, expressed as a time elapsed from a specific MJD time, in
             a particular time unit in a particular time system. This class transparently handles all conversions between time
             units and systems. Thus clients can use objects of this class in calculations without explicit knowledge
             of their units or systems.
  */
  class AbsoluteTime {
    public:
      AbsoluteTime(const std::string & time_system_name, long origin_mjd, const Duration & elapsed_time);

      AbsoluteTime(const std::string & time_system_name, long mjd_day, double mjd_sec);

      template <typename TimeRepType>
      AbsoluteTime(const std::string & time_system_name, const TimeRepType & time_rep) { set(time_system_name, time_rep); }

      template <typename TimeRepType>
      AbsoluteTime(const std::string & time_system_name, const TimeFormat<TimeRepType> & time_format, const std::string & time_string) {
        set(time_system_name, time_format, time_string);
      }

      template <typename TimeRepType>
      void get(const std::string & time_system_name, TimeRepType & time_rep) const;

      template <typename TimeRepType>
      void set(const std::string & time_system_name, const TimeRepType & time_rep);

      template <typename TimeRepType>
      void set(const std::string & time_system_name, const TimeFormat<TimeRepType> & time_format, const std::string & time_string);

      template <typename TimeRepType>
      std::string represent(const std::string & time_system_name, const TimeFormat<TimeRepType> & time_format,
        std::streamsize precision = std::numeric_limits<double>::digits10) const;

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

      AbsoluteTime computeAbsoluteTime(const std::string & time_system_name, const Duration & delta_t) const;

      ElapsedTime computeElapsedTime(const std::string & time_system_name, const AbsoluteTime & since) const;

      template <typename StreamType>
      void write(StreamType & os) const;

    private:
      // Prohibited operations:
      // These are not physical because TimeInterval is "anchored" to its endpoints, which are absolute moments in time.
      // In general, neither endpoint of the TimeInterval is the same as "this" AbsoluteTime. Note that similar operators
      // which use ElapsedTime are provided.
      // AbsoluteTime operator +(const TimeInterval &) const;
      // AbsoluteTime operator -(const TimeInterval &) const;
      const TimeSystem * m_time_system;
      moment_type m_moment;
  };

  template <typename TimeRepType>
  inline void AbsoluteTime::get(const std::string & time_system_name, TimeRepType & time_rep) const {
    // Convert time systems.
    const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
    moment_type moment = time_system.convertFrom(*m_time_system, m_moment);

    // Convert time formats.
    const TimeFormat<TimeRepType> & time_format = TimeFormatFactory<TimeRepType>::getFormat();
    datetime_type datetime = time_system.computeDateTime(moment);
    time_rep = time_format.convert(datetime);
  }

  template <typename TimeRepType>
  inline void AbsoluteTime::set(const std::string & time_system_name, const TimeRepType & time_rep) {
    // Set time system.
    m_time_system = &TimeSystem::getSystem(time_system_name);

    // Convert time formats.
    const TimeFormat<TimeRepType> & time_format = TimeFormatFactory<TimeRepType>::getFormat();
    datetime_type datetime = time_format.convert(time_rep);
    m_moment = m_time_system->computeMoment(datetime);
  }

  template <typename TimeRepType>
  inline void AbsoluteTime::set(const std::string & time_system_name, const TimeFormat<TimeRepType> & time_format,
    const std::string & time_string) {
    // Set time system.
    m_time_system = &TimeSystem::getSystem(time_system_name);

    // Parse time string.
    datetime_type datetime = time_format.convert(time_format.parse(time_string));
    m_moment = m_time_system->computeMoment(datetime);
  }

  template <typename TimeRepType>
  inline std::string AbsoluteTime::represent(const std::string & time_system_name, const TimeFormat<TimeRepType> & time_format,
    std::streamsize precision) const {
    // Convert time systems.
    const TimeSystem & time_system(TimeSystem::getSystem(time_system_name));
    moment_type moment = time_system.convertFrom(*m_time_system, m_moment);

    // Format the time into a character string.
    datetime_type datetime = time_system.computeDateTime(moment);
    return time_format.format(time_format.convert(datetime), precision) + " (" + time_system.getName() + ")";
  }

  template <typename StreamType>
  inline void AbsoluteTime::write(StreamType & os) const {
    if (m_moment.second > Duration::zero()) {
      // Write the time in the format of "123.456789 seconds after 54321.0 MJD (TDB)".
      os << m_moment.second << " after " << m_moment.first << ".0 MJD (" << *m_time_system << ")";
    } else if (m_moment.second < Duration::zero()) {
      // Write the time in the format of "123.456789 seconds before 54321.0 MJD (TDB)".
      os << -m_moment.second << " before " << m_moment.first << ".0 MJD (" << *m_time_system << ")";
    } else {
      // Write the time in the format of "54321.0 MJD (TDB)".
      os << m_moment.first << ".0 MJD (" << *m_time_system << ")";
    }
  }

  std::ostream & operator <<(std::ostream & os, const AbsoluteTime & time);

  st_stream::OStream & operator <<(st_stream::OStream & os, const AbsoluteTime & time);

}

#endif
