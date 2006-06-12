/** \file TimeRep.h
    \brief Declaration of TimeRep and related classes.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_TimeRep_h
#define timeSystem_TimeRep_h

#include "timeSystem/Duration.h"

#include <string>

namespace timeSystem {

  class AbsoluteTime;
  class TimeSystem;

  /** \class TimeRep
      \brief
  */
  class TimeRep {
    public:
      virtual ~TimeRep();

      virtual AbsoluteTime getTime() const = 0;

      virtual void setTime(const TimeSystem & system, const Duration & origin, const Duration & elapsed) = 0;

      void setAbsoluteTime(const AbsoluteTime & time);
  };

  /** \class MetRep
      \brief
  */
  class MetRep : public TimeRep {
    public:
      MetRep(const std::string & system_name, long mjd_ref_int, double mjd_ref_frac, double met);

      virtual AbsoluteTime getTime() const;

      virtual void setTime(const TimeSystem & system, const Duration & origin, const Duration & elapsed);

      double getValue() const;

      void setValue(double met);

    private:
      const TimeSystem * m_system;
      const Duration m_mjd_ref;
      double m_met;
  };

  /** \class MjdRep
      \brief
  */
  class MjdRep : public TimeRep {
    public:
      MjdRep(const std::string & system_name, long mjd_int, double mjd_frac);

      virtual AbsoluteTime getTime() const;

      virtual void setTime(const TimeSystem & system, const Duration & origin, const Duration & elapsed);

      IntFracPair getValue() const;

      void setValue(long mjd_int, double mjd_frac);

    private:
      const TimeSystem * m_system;
      Duration m_mjd;
  };

}

#endif
