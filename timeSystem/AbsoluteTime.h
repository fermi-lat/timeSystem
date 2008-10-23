/** \file AbsoluteTime.h
    \brief Declaration of AbsoluteTime class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_AbsoluteTime_h
#define timeSystem_AbsoluteTime_h

#include "timeSystem/TimeFormat.h"
#include "timeSystem/TimeSystem.h"

#include <iostream>
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
      /** \brief Construct a AbsoluteTime object from a pair of a time origin and an elapsed time.
          \param time_system_name Name of time system in which this object is defined.
          \param origin_mjd MJD number of the time origin of this object in the given time system.
          \param elapsed_time Duration object that represents an elapsed time from the time origin (above) in the given time system.
      */
      AbsoluteTime(const std::string & time_system_name, long origin_mjd, const Duration & elapsed_time);

      /** \brief Construct a AbsoluteTime object from a fractional MJD number.
          \param time_system_name Name of time system in which this object is defined.
          \param mjd_day Day part of an MJD number.
          \param mjd_sec Second part of an MJD number.
      */
      AbsoluteTime(const std::string & time_system_name, long mjd_day, double mjd_sec);

      /** \brief Construct a AbsoluteTime object from a time representation.
          \param time_system_name Name of time system in which this object is defined.
          \param time_rep Time representation that points to an absolute moment in time.
      */
      template <typename TimeRepType>
      AbsoluteTime(const std::string & time_system_name, const TimeRepType & time_rep) { set(time_system_name, time_rep); }

      /** \brief Construct a AbsoluteTime object from a character string in a given time format.
          \param time_system_name Name of time system in which this object is defined.
          \param time_format Time format in which a given character string is interpreted.
          \param time_string Character string that represents an absolute moment in time.
      */
      template <typename TimeRepType>
      AbsoluteTime(const std::string & time_system_name, const TimeFormat<TimeRepType> & time_format, const std::string & time_string) {
        set(time_system_name, time_format, time_string);
      }

      /** \brief Compute a specified time representation of the stored absolute time, and set it to the argument of this method.
          \param time_system_name Name of time system in which this object is defined.
          \param time_rep Time representation of the stored absolute time is to be set to this argument.
      */
      template <typename TimeRepType>
      void get(const std::string & time_system_name, TimeRepType & time_rep) const;

      /** \brief Set an absolute moment in time to this object, specified by a time representation.
          \param time_system_name Name of time system in which this object is defined.
          \param time_rep Time representation that points to an absolute moment in time to be set to this object.
      */
      template <typename TimeRepType>
      void set(const std::string & time_system_name, const TimeRepType & time_rep);

      /** \brief Set an absolute moment in time to this object, specified by a character string in a given time format.
          \param time_system_name Name of time system in which this object is defined.
          \param time_format Time format in which a given character string is interpreted.
          \param time_string Character string that represents an absolute moment in time to be set to this object.
      */
      template <typename TimeRepType>
      void set(const std::string & time_system_name, const TimeFormat<TimeRepType> & time_format, const std::string & time_string);

      /** \brief Create a character string that represents the stored absolute time in a specified format.
          \param time_system_name Name of time system in which the stored absolute time is evaluated.
          \param time_format Time format in which the stored time is formatted into a character string.
          \param precision Number of digits after a decimal point in a numerical part of the output character string.
      */
      template <typename TimeRepType>
      std::string represent(const std::string & time_system_name, const TimeFormat<TimeRepType> & time_format,
        std::streamsize precision = std::numeric_limits<double>::digits10) const;

      /** \brief Create an AbsoluteTime object that represents a sum of the stored absolute time to a given elapsed time.
          \param elapsed_time Elapsed time to be added.
      */
      AbsoluteTime operator +(const ElapsedTime & elapsed_time) const;

      /** \brief Create an AbsoluteTime object that represents the stored absolute time subtracted by a given elapsed time.
          \param elapsed_time Elapsed time to subtract.
      */
      AbsoluteTime operator -(const ElapsedTime & elapsed_time) const;

      /** \brief Add an elapsed time to the stored absolute time and set it to this object.
          \param elapsed_time Elapsed time to be added.
      */
      AbsoluteTime & operator +=(const ElapsedTime & elapsed_time);

      /** \brief Subtract an elapsed time from the stored absolute time and set it to this object.
          \param elapsed_time Elapsed time to subtract.
      */
      AbsoluteTime & operator -=(const ElapsedTime & elapsed_time);

      /** \brief Create an TimeInterval object that represents a time interval between the stored absolute time and a given
                 absolute time.
          \param abs_time Absolute time to subtract.
      */
      TimeInterval operator -(const AbsoluteTime & abs_time) const;

      /** \brief Return true if the stored absolute time is later than a given absolute time, and return false otherwise.
          \param other Absolute time to compare.
      */
      bool operator >(const AbsoluteTime & other) const;

      /** \brief Return true if the stored absolute time is later than or equal to a given absolute time,
                 and return false otherwise.
          \param other Absolute time to compare.
      */
      bool operator >=(const AbsoluteTime & other) const;

      /** \brief Return true if the stored absolute time is earlier than a given absolute time, and return false otherwise.
          \param other Absolute time to compare.
      */
      bool operator <(const AbsoluteTime & other) const;

      /** \brief Return true if the stored absolute time is earlier than or equal to a given absolute time,
                 and return false otherwise.
          \param other Absolute time to compare.
      */
      bool operator <=(const AbsoluteTime & other) const;

      /** \brief Return true if time difference between the stored absolute time and a given absolute time is smaller than
                 or equal to a given elapsed time, and return false otherwise.
          \param tolerance Maximum allowed elapsed time in comparison.
      */
      bool equivalentTo(const AbsoluteTime & other, const ElapsedTime & tolerance) const;

      /** \brief Create an AbsoluteTime object that represents a sum of the stored absolute time and a specified elapsed time.
          \param time_system_name Name of time system in which an elapsed time to be added is defined.
          \param delta_t Time duration of an elapsed time to be added.
      */
      AbsoluteTime computeAbsoluteTime(const std::string & time_system_name, const Duration & delta_t) const;

      /** \brief Create an ElapsedTime object that represents an elapsed time between the stored absolute time and a given
                 absolute time in a given time system.
          \param time_system_name Name of time system in which an elapsed time is to be computed.
          \param since Absolute time to be subtracted from the stored absolute time.
      */
      ElapsedTime computeElapsedTime(const std::string & time_system_name, const AbsoluteTime & since) const;

      /** \brief Write a text representation of the stored absolute time to an output stream.
          \param os Output stream to write a text representation of the stored absolute time to.
      */
      template <typename StreamType>
      void write(StreamType & os) const;

      /// \brief Create a character string that contains a text description of this object.
      std::string describe() const;

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
    // Convert the time to a unique representation.
    datetime_type datetime = m_time_system->computeDateTime(m_moment);

    // Change precision, saving the current value.
    std::streamsize prec = os.precision(std::numeric_limits<double>::digits10);

    // Write the time.
    if (datetime.second > 0.) {
      // Write the time in the format of "123.456789 seconds after 54321.0 MJD (TDB)".
      os << datetime.second << " seconds after " << datetime.first << ".0 MJD (" << *m_time_system << ")";
    } else if (datetime.second > 0.) {
      // Write the time in the format of "123.456789 seconds before 54321.0 MJD (TDB)".
      // Note: This branch should never be reached because datetime_type.second is non-negative by definition.
      //       Keep this section just in case datetime_type changes its design.
      os << -datetime.second << " seconds before " << datetime.first << ".0 MJD (" << *m_time_system << ")";
    } else {
      // Write the time in the format of "54321.0 MJD (TDB)".
      os << datetime.first << ".0 MJD (" << *m_time_system << ")";
    }

    // Restore the precision.
    os.precision(prec);
  }

  std::ostream & operator <<(std::ostream & os, const AbsoluteTime & time);

  st_stream::OStream & operator <<(st_stream::OStream & os, const AbsoluteTime & time);

}

#endif
