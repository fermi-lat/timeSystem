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

  typedef std::pair<long, Duration> moment_type;
  typedef std::pair<long, double> datetime_type;

  /** \class TimeSystem
      \brief Class that represents a time system, e.g., TDB (barycentric dynamical time), TT (terrestrial time),
             UTC (coordinated universal time), etc. It converts absolute times and time intervals between different time systems,
  */
  class TimeSystem {
    public:
      /** \brief Return a named TimeSystem object.
          \param system_name Name of a time system to get.
      */
      static const TimeSystem & getSystem(const std::string & system_name);

      /** \brief Load leap-second data from a given file.
          \param leap_sec_file_name Name of a leap-second file in the FITS format.
          \param force_load Set to true to load new leap-second data even if already loaded.
                            Set to false not to load them in case leap-second data have already been loaded.
      */
      static void loadLeapSeconds(std::string leap_sec_file_name = "", bool force_load = true);

      /// \brief Return the name of a default leap-second file currently set.
      static std::string getDefaultLeapSecFileName();

      /** \brief Set the name of a default leap-second file.
          \param leap_sec_file_name Name of the default leap-second file.
      */
      static void setDefaultLeapSecFileName(const std::string & leap_sec_file_name);

      /// \brief Destruct this TimeSystem object.
      virtual ~TimeSystem();

      /** \brief Convert a time moment expressed in a different time system to the one in this time system, and return it.
          \param time_system Time system to conver a time moment from.
          \param moment Time moment to convert.
      */
      virtual moment_type convertFrom(const TimeSystem & time_system, const moment_type & moment) const = 0;

      /** \brief Compute time difference between two moments of time, and return it.
          \param moment1 Time moment from which the other time moment is to be subtracted.
          \param moment2 Time moment which is subtracted from the other time moment.
      */
      virtual Duration computeTimeDifference(const moment_type & moment1, const moment_type & moment2) const;

      /** \brief Compute date and time of a given time moment, and return it.
          \param moment Time moment to compute date and time for.
      */
      virtual datetime_type computeDateTime(const moment_type & moment) const;

      /** \brief Compute a time moment of a given date and time, and return it.
          \param datetime Date and time to compute a time moment for.
      */
      virtual moment_type computeMoment(const datetime_type & datetime) const;

      /** \brief Check validity of a given time moment.
          \param moment Time moment to be tested.
      */
      virtual void checkMoment(const moment_type & moment) const;

      /// \brief Return the name of this time system.
      std::string getName() const;

      /** \brief Write text representation of this time system into a given stream.
          \param os Stream to write to.
      */
      template <typename StreamType>
      void write(StreamType & os) const;

    protected:
      typedef std::map<std::string, const TimeSystem *> container_type;

      /// \brief Return a container of registered time systems.
      static container_type & getContainer();

      static std::string s_default_leap_sec_file;

      /** \brief Construct a TimeSystem object.
          \param system_name Name of the time system to construct.
      */
      TimeSystem(const std::string & system_name);

      std::string m_system_name;
  };

  template <typename StreamType>
  inline void TimeSystem::write(StreamType & os) const { os << m_system_name; }

  std::ostream & operator <<(std::ostream & os, const TimeSystem & sys);

  st_stream::OStream & operator <<(st_stream::OStream & os, const TimeSystem & sys);

}

#endif
