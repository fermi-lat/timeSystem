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
      virtual ~TimeFormat() {}

      virtual TimeRepType convert(const datetime_type & datetime) const = 0;

      virtual datetime_type convert(const TimeRepType & time_rep) const = 0;

      virtual TimeRepType parse(const std::string & time_string) const = 0;

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
      static const TimeFormat<TimeRepType> & getFormat() {
        throw std::runtime_error("Requested time representation not supported");
      }
  };
}

#endif
