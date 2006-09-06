/** \file TimeRep.h
    \brief Declaration of TimeRep and related classes.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_TimeRep_h
#define timeSystem_TimeRep_h

#include "timeSystem/Duration.h"

#include <string>

namespace tip {
  class Header;
}

namespace timeSystem {

  class AbsoluteTime;
  class TimeSystem;

  class MjdRefDatabase {
    public:
      IntFracPair operator ()(const tip::Header & header) const;
  };

  /** \class TimeRep
      \brief
  */
  class TimeRep {
    public:
      virtual ~TimeRep();

      virtual TimeRep & operator =(const AbsoluteTime & abs_time);

      virtual void get(std::string & system_name, Duration & origin, Duration & elapsed) const = 0;

      virtual void set(const std::string & system_name, const Duration & origin, const Duration & elapsed) = 0;

      virtual std::string getString() const = 0;

      virtual void assign(const std::string & value) = 0;

      template <typename StreamType>
      void write(StreamType & os) const;
  };

  template <typename StreamType>
  inline void TimeRep::write(StreamType & os) const { os << getString(); }

  std::ostream & operator <<(std::ostream & os, const TimeRep & time_rep);

  st_stream::OStream & operator <<(st_stream::OStream & os, const TimeRep & time_rep);

  /** \class MetRep
      \brief
  */
  class MetRep : public TimeRep {
    public:
      MetRep(const std::string & system_name, long mjd_ref_int, double mjd_ref_frac, double met);

      MetRep(const std::string & system_name, const IntFracPair & mjd_ref, double met);

      virtual MetRep & operator =(const AbsoluteTime & abs_time);

      virtual void get(std::string & system_name, Duration & origin, Duration & elapsed) const;

      virtual void set(const std::string & system_name, const Duration & origin, const Duration & elapsed);

      virtual std::string getString() const;

      virtual void assign(const std::string & value);

      double getValue() const;

      void setValue(double met);

    private:
      const TimeSystem * m_system;
      Duration m_mjd_ref;
      double m_met;
  };

  /** \class MjdRep
      \brief
  */
  class MjdRep : public TimeRep {
    public:
      MjdRep(const std::string & system_name, long mjd_int, double mjd_frac);

      virtual MjdRep & operator =(const AbsoluteTime & abs_time);

      virtual void get(std::string & system_name, Duration & origin, Duration & elapsed) const;

      virtual void set(const std::string & system_name, const Duration & origin, const Duration & elapsed);

      virtual std::string getString() const;

      virtual void assign(const std::string & value);

      IntFracPair getValue() const;

      void setValue(long mjd_int, double mjd_frac);

    private:
      const TimeSystem * m_system;
      Duration m_mjd;
  };

}

#endif
