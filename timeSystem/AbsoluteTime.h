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

  /** \class Mjd
      \brief Class to hold an MJD number in two parts, the integer part and the fractional part, in order to
             hold it in a precision required by GLAST.
  */
  struct Mjd {
    Mjd(long int_part, double frac_part): m_int(int_part), m_frac(frac_part) {}
    long m_int;
    double m_frac;
  };

  /** \class Mjd1
      \brief Class to hold an MJD number in one double-precision variable.
  */
  struct Mjd1 {
    Mjd1(double day): m_day(day) {}
    double m_day;
  };

  // TODO: Implement struct Jd, Jd1, MonthDay, WeekDay, YearDay.
  // NOTE: Data members of struct MonthDay: m_year, m_mon,  m_day, m_hour, m_min, and m_sec (cf. struct tm in C).
  // NOTE: Data members of struct WeekDay:  m_year, m_week, m_day, m_hour, m_min, and m_sec (cf. struct tm in C).
  // NOTE: Data members of struct YearDay:  m_year,         m_day, m_hour, m_min, and m_sec (cf. struct tm in C).

  /** \class AbsoluteTime
      \brief Class which represents an absolute moment in time, expressed as a time elapsed from a specific MJD time, in
             a particular time unit in a particular time system. This class transparently handles all conversions between time
             units and systems. Thus clients can use objects of this class in calculations without explicit knowledge
             of their units or systems.
  */
  class AbsoluteTime {
    public:
      // TODO: Remove constructor taking two Duration objects.
      AbsoluteTime(const std::string & time_system_name, const Duration & origin, const Duration & time);

      AbsoluteTime(const std::string & time_system_name, long mjd_day, double mjd_sec);

      AbsoluteTime(const std::string & time_system_name, const Mjd & mjd);
      AbsoluteTime(const std::string & time_system_name, const Mjd1 & mjd);

      void get(const std::string & time_system_name, Mjd & mjd) const;
      void get(const std::string & time_system_name, Mjd1 & mjd) const;

      void set(const std::string & time_system_name, const Mjd & mjd);
      void set(const std::string & time_system_name, const Mjd1 & mjd);

      void set(const std::string & time_system_name, const std::string & time_format_name, const std::string & time_string);

      std::string represent(const std::string & time_system_name, const std::string & time_format_name,
        std::streamsize precision = std::numeric_limits<double>::digits10) const;

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
      moment_type convert(const TimeSystem & time_system, const Moment & time) const;

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
    // Write the time in the format of "123.456789 seconds after 54321.0 MJD (TDB)".
    os << m_moment.second << " seconds after " << m_moment.first << ".0 MJD (" << *m_time_system << ")";
  }

  std::ostream & operator <<(std::ostream & os, const AbsoluteTime & time);

  st_stream::OStream & operator <<(st_stream::OStream & os, const AbsoluteTime & time);

}

#endif
