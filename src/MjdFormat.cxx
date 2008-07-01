/** \file MjdFormat.cxx
    \brief Implementation of MjdFormat and related classes.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/MjdFormat.h"

#include "timeSystem/IntFracPair.h"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {

  using namespace timeSystem;

  /** \class MjdFormat
      \brief Class to convert time representations in MJD format, holding an integer and a fractional part separately.
  */
  class MjdFormat : public TimeFormat<Mjd> {
    public:
      virtual void convert(const datetime_type & datetime, Mjd & time_rep) const;

      virtual void convert(const Mjd & time_rep, datetime_type & datetime) const;

      virtual datetime_type parse(const std::string & time_string) const;

      virtual std::string format(const datetime_type & time_string, std::streamsize precision = std::numeric_limits<double>::digits10)
        const;
  };

  /** \class Mjd1Format
      \brief Class to convert time representations in MJD format, held in one double-precision variable.
  */
  class Mjd1Format : public TimeFormat<Mjd1> {
    public:
      virtual void convert(const datetime_type & datetime, Mjd1 & time_rep) const;

      virtual void convert(const Mjd1 & time_rep, datetime_type & datetime) const;

      virtual datetime_type parse(const std::string & time_string) const;

      virtual std::string format(const datetime_type & time_string, std::streamsize precision = std::numeric_limits<double>::digits10)
        const;
  };

  void MjdFormat::convert(const datetime_type & datetime, Mjd & mjd_rep) const {
    if (datetime.second < SecPerDay()) {
      mjd_rep.m_int = datetime.first;
      mjd_rep.m_frac = datetime.second / SecPerDay();
    } else {
      // During an inserted leap-second.
      std::ostringstream os;
      os << "Unable to compute an MJD number for the given time: " << datetime.second << " seconds of " << datetime.first << " MJD.";
      throw std::runtime_error(os.str());
    }
  }

  void MjdFormat::convert(const Mjd & mjd_rep, datetime_type & datetime) const {
    // Split mjd_frac into integer part and fractional part.
    IntFracPair mjd_frac_split(mjd_rep.m_frac);

    // Set the value to the datetime_type object.
    datetime.first = mjd_rep.m_int + mjd_frac_split.getIntegerPart();
    datetime.second = mjd_frac_split.getFractionalPart() * SecPerDay();
  }

  datetime_type MjdFormat::parse(const std::string & time_string) const {
    IntFracPair int_frac(time_string);
    Mjd mjd_rep(int_frac.getIntegerPart(), int_frac.getFractionalPart());
    datetime_type datetime(0, 0.);
    convert(mjd_rep, datetime);
    return datetime;
  }

  std::string MjdFormat::format(const datetime_type & time_string, std::streamsize precision) const {
    Mjd mjd_rep(0, 0.);
    convert(time_string, mjd_rep);
    IntFracPair int_frac(mjd_rep.m_int, mjd_rep.m_frac);

    std::ostringstream os;
    os.setf(std::ios::fixed);
    os << std::setprecision(precision) << int_frac << " MJD";
    return os.str();
  }

  void Mjd1Format::convert(const datetime_type & datetime, Mjd1 & mjd1_rep) const {
    const TimeFormat<Mjd> & mjd_format(TimeFormatFactory<Mjd>::getFormat());
    Mjd mjd_rep(0, 0.);
    mjd_format.convert(datetime, mjd_rep);
    mjd1_rep.m_day = mjd_rep.m_int + mjd_rep.m_frac;
  }

  void Mjd1Format::convert(const Mjd1 & mjd1_rep, datetime_type & datetime) const {
    const TimeFormat<Mjd> & mjd_format(TimeFormatFactory<Mjd>::getFormat());
    IntFracPair mjd_int_frac(mjd1_rep.m_day);
    Mjd mjd_rep(mjd_int_frac.getIntegerPart(), mjd_int_frac.getFractionalPart());
    mjd_format.convert(mjd_rep, datetime);
  }

  datetime_type Mjd1Format::parse(const std::string & time_string) const {
    std::istringstream iss(time_string);
    Mjd1 mjd1_rep(0.);
    iss >> mjd1_rep.m_day;
    if (iss.fail() || !iss.eof()) throw std::runtime_error("Error parsing \"" + time_string + "\"");

    datetime_type datetime(0, 0.);
    convert(mjd1_rep, datetime);
    return datetime;
  }

  std::string Mjd1Format::format(const datetime_type & time_string, std::streamsize precision) const {
    Mjd1 mjd1_rep(0.);
    convert(time_string, mjd1_rep);

    std::ostringstream os;
    os.setf(std::ios::fixed);
    os << std::setprecision(precision) << mjd1_rep.m_day << " MJD";
    return os.str();
  }

}

namespace timeSystem {

  const TimeFormat<Mjd> & TimeFormatFactory<Mjd>::getFormat() {
    static const MjdFormat s_mjd_format;
    return s_mjd_format;
  }

  const TimeFormat<Mjd1> & TimeFormatFactory<Mjd1>::getFormat() {
    static const Mjd1Format s_mjd1_format;
    return s_mjd1_format;
  }

}
