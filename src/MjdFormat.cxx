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
      virtual Mjd convert(const datetime_type & datetime) const;

      virtual datetime_type convert(const Mjd & time_rep) const;

      virtual Mjd parse(const std::string & time_string) const;

      virtual std::string format(const Mjd & time_rep, std::streamsize precision = std::numeric_limits<double>::digits10) const;
  };

  /** \class Mjd1Format
      \brief Class to convert time representations in MJD format, held in one double-precision variable.
  */
  class Mjd1Format : public TimeFormat<Mjd1> {
    public:
      virtual Mjd1 convert(const datetime_type & datetime) const;

      virtual datetime_type convert(const Mjd1 & time_rep) const;

      virtual Mjd1 parse(const std::string & time_string) const;

      virtual std::string format(const Mjd1 & time_rep, std::streamsize precision = std::numeric_limits<double>::digits10) const;
  };

  Mjd MjdFormat::convert(const datetime_type & datetime) const {
    // Check whether the second part is in bounds.
    if (datetime.second >= SecPerDay()) {
      // During an inserted leap-second.
      std::ostringstream os;
      os << "Unable to compute an MJD number for the given time: " << datetime.second << " seconds of " << datetime.first << " MJD.";
      throw std::runtime_error(os.str());
    }

    // Return an Mjd object.
    return Mjd(datetime.first, datetime.second / SecPerDay());
  }

  datetime_type MjdFormat::convert(const Mjd & mjd_rep) const {
    // Split mjd_frac into integer part and fractional part.
    IntFracPair mjd_frac_split(mjd_rep.m_frac);

    // Set the value to the datetime_type object.
    return datetime_type(mjd_rep.m_int + mjd_frac_split.getIntegerPart(), mjd_frac_split.getFractionalPart() * SecPerDay());
  }

  Mjd MjdFormat::parse(const std::string & time_string) const {
    IntFracPair int_frac(time_string);
    return Mjd(int_frac.getIntegerPart(), int_frac.getFractionalPart());
  }

  std::string MjdFormat::format(const Mjd & time_rep, std::streamsize precision) const {
    std::ostringstream os;
    os.setf(std::ios::fixed);
    os << std::setprecision(precision) << IntFracPair(time_rep.m_int, time_rep.m_frac) << " MJD";
    return os.str();
  }

  Mjd1 Mjd1Format::convert(const datetime_type & datetime) const {
    const TimeFormat<Mjd> & mjd_format(TimeFormatFactory<Mjd>::getFormat());
    Mjd mjd_rep = mjd_format.convert(datetime);
    return Mjd1(mjd_rep.m_int + mjd_rep.m_frac);
  }

  datetime_type Mjd1Format::convert(const Mjd1 & mjd1_rep) const {
    const TimeFormat<Mjd> & mjd_format(TimeFormatFactory<Mjd>::getFormat());
    IntFracPair mjd_int_frac(mjd1_rep.m_day);
    Mjd mjd_rep(mjd_int_frac.getIntegerPart(), mjd_int_frac.getFractionalPart());
    return mjd_format.convert(mjd_rep);
  }

  Mjd1 Mjd1Format::parse(const std::string & time_string) const {
    std::istringstream iss(time_string);
    Mjd1 mjd1_rep(0.);
    iss >> mjd1_rep.m_day;
    if (iss.fail() || !iss.eof()) throw std::runtime_error("Error parsing \"" + time_string + "\"");

    return mjd1_rep;
  }

  std::string Mjd1Format::format(const Mjd1 & time_rep, std::streamsize precision) const {
    std::ostringstream os;
    os.setf(std::ios::fixed);
    os << std::setprecision(precision) << time_rep.m_day << " MJD";
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
