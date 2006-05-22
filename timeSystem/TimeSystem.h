/** \file TimeSystem.h
    \brief Declaration of TimeSystem class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_TimeSystem_h
#define timeSystem_TimeSystem_h

#include "st_stream/Stream.h"

#include "timeSystem/Duration.h"

#include <map>
#include <string>

namespace timeSystem {

  /** \class TimeSystem
      \brief Class to convert absolute times and time intervals between different time systems, i.e. TDB (barycentric
             dynamical time), TT (terrestrial time), UTC (coordinated universal time), etc.
  */
  class TimeSystem {
    public:
      static const TimeSystem & getSystem(const std::string & system_name);

      static void loadLeapSeconds(std::string leap_sec_file_name = "", bool force_load = true);

      virtual ~TimeSystem();

      virtual Moment convertFrom(const TimeSystem & time_system, const Moment & time) const = 0;

      virtual Duration computeTimeDifference(const Duration & mjd1, const Duration & mjd2) const = 0;

      virtual Duration findMjdExpression(const Moment & time) const = 0;

      std::string getName() const;

      template <typename StreamType>
      void write(StreamType & os) const;

    protected:
      typedef std::map<std::string, TimeSystem *> container_type;

      static container_type & getContainer();

      TimeSystem(const std::string & system_name);

      std::string m_system_name;

    private:
      static TimeSystem & getNonConstSystem(const std::string & system_name);
  };

  template <typename StreamType>
  inline void TimeSystem::write(StreamType & os) const { os << m_system_name; }

  std::ostream & operator <<(std::ostream & os, const TimeSystem & sys);

  st_stream::OStream & operator <<(st_stream::OStream & os, const TimeSystem & sys);

}

#endif
