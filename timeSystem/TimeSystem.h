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

      static std::string getDefaultLeapSecFileName();

      static void setDefaultLeapSecFileName(const std::string & leap_sec_file_name);

      virtual ~TimeSystem();

      virtual moment_type convertFrom(const TimeSystem & time_system, const moment_type & moment) const = 0;

      virtual Duration computeTimeDifference(const moment_type & moment1, const moment_type & moment2) const;

      virtual moment_type computeAdvancedTime(const moment_type & moment, const Duration & elapsed) const;

      std::string getName() const;

      template <typename StreamType>
      void write(StreamType & os) const;

    protected:
      typedef std::map<std::string, const TimeSystem *> container_type;

      static container_type & getContainer();

      static std::string s_default_leap_sec_file;

      TimeSystem(const std::string & system_name);

      std::string m_system_name;
  };

  template <typename StreamType>
  inline void TimeSystem::write(StreamType & os) const { os << m_system_name; }

  std::ostream & operator <<(std::ostream & os, const TimeSystem & sys);

  st_stream::OStream & operator <<(st_stream::OStream & os, const TimeSystem & sys);

}

#endif
