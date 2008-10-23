/** \file TimeFormat.h
    \brief Declaration of TimeFormat and related classes.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_TimeFormat_h
#define timeSystem_TimeFormat_h

#include "timeSystem/TimeSystem.h"

#include <limits>
#include <map>
#include <stdexcept>
#include <string>

namespace timeSystem {

  /** \class TimeFormat
      \brief Base class to represent time format, such as MJD and Calender Day of ISO 8601.
  */
  template <typename TimeRepType>
  class TimeFormat {
    public:
      /// Destruct this TimeFormat object.
      virtual ~TimeFormat() {}

      /** \brief Convert date and time to a specific time representation.
          \param datetime Date and time to convert.
      */
      virtual TimeRepType convert(const datetime_type & datetime) const = 0;

      /** \brief Convert a specific time representation to date and time.
          \param time_rep Time representation to convert.
      */
      virtual datetime_type convert(const TimeRepType & time_rep) const = 0;

      /** \brief Interpret a character string as a specific time representation, and return the time representation.
          \param time_string Character string to interpret as a specific time representation.
      */
      virtual TimeRepType parse(const std::string & time_string) const = 0;

      /** \brief Create a character string representing a time moment in a specific time representation, and return it.
          \param time_rep Time representation to create a character string for.
          \param precision Number of digits of a floating-point number in the returned character string.
                           Precise interpretation of this argument depends on time format.
      */
      virtual std::string format(const TimeRepType & time_rep, std::streamsize precision = std::numeric_limits<double>::digits10)
        const = 0;
  };

  /** \class TimeFormatFactory
      \brief Class to create an instance of a concrete TimeFormat subclass. A concrete factory should be implemented
             for each time representation via template specialization.
  */
  template <typename TimeRepType>
  class TimeFormatFactory {
    public:
      /** \brief Return a TimeFormat object. This is a default behavior in case the named time representation is not implemented.
      */
      static const TimeFormat<TimeRepType> & getFormat() {
        throw std::runtime_error("Requested time representation not supported");
      }
  };
}

#endif
