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

    protected:
      typedef std::map<std::string, TimeFormat *> container_type;

      static container_type & getContainer();

      TimeFormat(const std::string & format_name);

      // TODO: Remove m_format_name (not used at all).
      std::string m_format_name;
  };

  /** \class MjdFormat
      \brief Class to represent MJD format of time representation.
  */
  class MjdFormat : public TimeFormat {
    public:
      static const MjdFormat & getMjdFormat();

      virtual std::string format(const datetime_type & value, std::streamsize precision = std::numeric_limits<double>::digits10) const;

      virtual datetime_type parse(const std::string & value) const;

      void convert(const datetime_type & moment, long & mjd_int, double & mjd_frac) const;

      void convert(const datetime_type & moment, double & mjd) const;

      void convert(long mjd_int, double mjd_frac, datetime_type & moment) const;

      void convert(double mjd, datetime_type & moment) const;

    private:
      MjdFormat();
  };

}

#endif
