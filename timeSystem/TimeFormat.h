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
#include <string>

namespace timeSystem {

  /** \class TimeFormat
      \brief Base class to represent time format, such as MJD and Calender Day of ISO 8601.
  */
  class TimeFormat {
    public:
      virtual ~TimeFormat();

      static const TimeFormat & getFormat(const std::string & format_name);

      virtual std::string format(const datetime_type & value, std::streamsize precision = std::numeric_limits<double>::digits10) const = 0;

      virtual datetime_type parse(const std::string & value) const = 0;

      template <typename TimeRepType>
      static void convert(const datetime_type & datetime, TimeRepType & time_rep);

      template <typename TimeRepType>
      static void convert(const TimeRepType & time_rep, datetime_type & datetime);

    protected:
      typedef std::map<std::string, TimeFormat *> container_type;

      static container_type & getContainer();

      TimeFormat(const std::string & format_name);
  };

}

#endif
