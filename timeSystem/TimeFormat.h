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

      virtual void convert(const datetime_type & datetime, TimeRepType & time_rep) const = 0;

      virtual void convert(const TimeRepType & time_rep, datetime_type & datetime) const = 0;

      virtual datetime_type parse(const std::string & time_string) const = 0;

      virtual std::string format(const datetime_type & time_string, std::streamsize precision = std::numeric_limits<double>::digits10)
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
        throw std::runtime_error("Time format not supported for the requested time representation");
      }
  };
}

#endif
