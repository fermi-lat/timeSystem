/** \file TimeRep.cxx
    \brief Implementation of TimeRep and related classes.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/AbsoluteTime.h"
#include "timeSystem/GlastMetRep.h"
#include "timeSystem/TimeRep.h"
#include "timeSystem/TimeSystem.h"

#include "tip/Header.h"

#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {
  int s_digits = std::numeric_limits<double>::digits10;
}

namespace timeSystem {

  IntFracPair MjdRefDatabase::operator ()(const tip::Header & header) const {
    IntFracPair mjd_ref_pair;
    bool found_mjd_ref = false;

    if (!found_mjd_ref) {
      try {
        int mjd_ref_int = 0;
        double mjd_ref_frac = 0.;
        header["MJDREFI"].get(mjd_ref_int);
        header["MJDREFF"].get(mjd_ref_frac);
        mjd_ref_pair = IntFracPair(mjd_ref_int, mjd_ref_frac);
        found_mjd_ref = true;
      } catch (const std::exception &) {}
    }

    if (!found_mjd_ref) {
      try {
        double mjd_ref = 0.;
        header["MJDREF"].get(mjd_ref);
        IntFracPair mjd(mjd_ref);
        int mjd_ref_int = mjd.getIntegerPart();
        double mjd_ref_frac = mjd.getFractionalPart();
        mjd_ref_pair = IntFracPair(mjd_ref_int, mjd_ref_frac);
        found_mjd_ref = true;
      } catch (const std::exception &) {}
    }

#if 0
    if (!found_mjd_ref) {
      try {
        std::string telescope = header["TELESCOP"].get();
        for (std::string::iterator itor = telescope.begin(); itor != telescope.end(); ++itor) *itor = std::toupper(*itor);
        // TODO To support more missions, use prototypes looked up by telescope name instead of hardwired GlastMetRep.
        if (telescope == "GLAST") {
          GlastMetRep glast_met(system_name, met);
          m_mjd_ref = glast_met.m_mjd_ref;
          found_mjd_ref = true;
        }
      } catch (const std::exception &) {}
    }
#endif

    if (!found_mjd_ref) {
      throw std::runtime_error("MetRep could not find MJDREFI/MJDREFF or MJDREF.");
    }

    return mjd_ref_pair;
  }

  TimeRep::~TimeRep() {
    for(cont_type::iterator itor = m_field_cont.begin(); itor != m_field_cont.end(); ++itor)
      delete itor->second;
  }

  TimeRep & TimeRep::operator =(const AbsoluteTime & abs_time) {
    abs_time.exportTimeRep(*this);
    return *this;
  }

  std::ostream & operator <<(std::ostream & os, const TimeRep & time_rep) {
    time_rep.write(os);
    return os;
  }

  st_stream::OStream & operator <<(st_stream::OStream & os, const TimeRep & time_rep) {
    time_rep.write(os);
    return os;
  }


  MetRep::MetRep(const std::string & system_name, long mjd_ref_int, double mjd_ref_frac, double met):
    m_system(&TimeSystem::getSystem(system_name)), m_mjd_ref(IntFracPair(mjd_ref_int, mjd_ref_frac), Day), m_met(met) {}

  MetRep::MetRep(const std::string & system_name, const IntFracPair & mjd_ref, double met):
    m_system(&TimeSystem::getSystem(system_name)), m_mjd_ref(mjd_ref, Day), m_met(met) {}

  MetRep & MetRep::operator =(const AbsoluteTime & abs_time) { TimeRep::operator =(abs_time); return *this; }

  void MetRep::get(std::string & system_name, Duration & origin, Duration & elapsed) const {
    system_name = m_system->getName(); origin = m_mjd_ref; elapsed = Duration(0, m_met);
  }

  void MetRep::get(const std::string & /* field_name */, double & value) const {
    // TODO Ignore field_name or throw exception?
    value = m_met;
  }

  void MetRep::get(const std::string & /* field_name */, long & value) const {
    // TODO Ignore field_name or throw exception?
    // TODO Should this conversion work?
    value = long(m_met);
  }

  void MetRep::set(const std::string & system_name, const Duration & origin, const Duration & elapsed) {
    // Convert from the given time into "this" system.
    Moment my_time = m_system->convertFrom(TimeSystem::getSystem(system_name), Moment(origin, elapsed));

    // Now compute met from my_time in this system.
    Duration met = my_time.second + m_system->computeTimeDifference(my_time.first, m_mjd_ref);
    IntFracPair met_pair = met.getValue(Sec);
    m_met = met_pair.getDouble();
  }

  void MetRep::set(const std::string & /* field_name */, const double & value) {
    // TODO Ignore field_name or throw exception?
    m_met = value;
  }

  void MetRep::set(const std::string & /* field_name */, const long & value) {
    // TODO Ignore field_name or throw exception?
    // TODO Should this conversion work?
    m_met = double(value);
  }

  std::string MetRep::getString() const {
    std::ostringstream os;
    os << std::setprecision(s_digits) << getValue() << " MET (" << *m_system << ") [MJDREF=" << m_mjd_ref.getValue(Day) << "]";
    return os.str();
  }

  void MetRep::assign(const std::string & value) {
    IntFracPair pair_value(value);
    setValue(pair_value.getDouble());
  }

  double MetRep::getValue() const { return m_met; }

  void MetRep::setValue(double met) { m_met = met; }

  MjdRep::MjdRep(const std::string & system_name, long mjd_int, double mjd_frac):
    m_system(&TimeSystem::getSystem(system_name)) {
    IntFracPair int_frac(mjd_int, mjd_frac);
    addField<long>("MJDI", int_frac.getIntegerPart());
    addField<double>("MJDF", int_frac.getFractionalPart());
  }

  MjdRep & MjdRep::operator =(const AbsoluteTime & abs_time) { TimeRep::operator =(abs_time); return *this; }

  void MjdRep::get(std::string & system_name, Duration & origin, Duration & elapsed) const {
    system_name = m_system->getName();

    long int_part = 0;
    TimeRep::get("MJDI", int_part);
    double frac_part = 0.;
    TimeRep::get("MJDF", frac_part);
    origin = Duration(IntFracPair(int_part, frac_part), Day);

    elapsed = Duration(0, 0.);
  }

  void MjdRep::set(const std::string & system_name, const Duration & origin, const Duration & elapsed) {
    // Convert from the given time into "this" system.
    Moment my_time = m_system->convertFrom(TimeSystem::getSystem(system_name), Moment(origin, elapsed));

    // Now compute mjd from my_time in this system as a Duration object.
    Duration mjd = m_system->computeMjd(my_time);

    // Extract Duration in units of days.
    IntFracPair int_frac(mjd.getValue(Day));

    // Save value in two pieces.
    TimeRep::set("MJDI", int_frac.getIntegerPart());
    TimeRep::set("MJDF", int_frac.getFractionalPart());
  }

  std::string MjdRep::getString() const {
    std::ostringstream os;
    os << std::setprecision(s_digits) << getValue() << " MJD (" << *m_system << ")";
    return os.str();
  }

  void MjdRep::assign(const std::string & value) {
    IntFracPair pair_value(value);
    setValue(pair_value.getIntegerPart(), pair_value.getFractionalPart());
  }

  IntFracPair MjdRep::getValue() const {
    long int_part = 0;
    TimeRep::get("MJDI", int_part);
    double frac_part = 0.;
    TimeRep::get("MJDF", frac_part);
    return IntFracPair(int_part, frac_part);
  }

  void MjdRep::setValue(long mjd_int, double mjd_frac) {
    IntFracPair int_frac(mjd_int, mjd_frac);
    TimeRep::set("MJDI", int_frac.getIntegerPart());
    TimeRep::set("MJDF", int_frac.getFractionalPart());
  }

  TimeFormat::TimeFormat(const std::string & format_name): m_format_name(format_name) {
    std::string format_name_uc(format_name);
    for (std::string::iterator itor = format_name_uc.begin(); itor != format_name_uc.end(); ++itor) *itor = std::toupper(*itor);
    getContainer()[format_name_uc] = this;
  }

  TimeFormat::~TimeFormat() {
  }

  const TimeFormat & TimeFormat::getFormat(const std::string & format_name) {
    MjdFormat::getMjdFormat();

    std::string format_name_uc(format_name);
    for (std::string::iterator itor = format_name_uc.begin(); itor != format_name_uc.end(); ++itor) *itor = std::toupper(*itor);
    container_type & container(getContainer());

    container_type::iterator cont_itor = container.find(format_name_uc);
    if (container.end() == cont_itor) throw std::runtime_error("TimeFormat::getFormat could not find time format " + format_name);
    return *cont_itor->second;
  }

  TimeFormat::container_type & TimeFormat::getContainer() {
    static container_type s_prototype;
    return s_prototype;
  }

  MjdFormat::MjdFormat(): TimeFormat("MJD") {}

  const MjdFormat & MjdFormat::getMjdFormat() {
    static MjdFormat s_format;
    return s_format;
  }

  std::string MjdFormat::format(const moment_type & value, std::streamsize precision) const {
    std::ostringstream os;
    long mjd_int = 0;
    double mjd_frac = 0.;
    convert(value, mjd_int, mjd_frac);
    IntFracPair int_frac(mjd_int, mjd_frac);
    os << std::setprecision(precision) << int_frac << " MJD";
    return os.str();
  }

  moment_type MjdFormat::parse(const std::string & value) const {
    IntFracPair int_frac(value);
    long mjd_int = int_frac.getIntegerPart();
    double mjd_frac = int_frac.getFractionalPart();
    moment_type moment;
    convert(mjd_int, mjd_frac, moment);
    return moment;
  }

  void MjdFormat::convert(const moment_type & moment, long & mjd_int, double & mjd_frac) const {
    if (SecPerDay() < moment.second) {
      // During an inserted leap-second.
      mjd_int = moment.first + 1;
      mjd_frac = 0.;
    } else {
      mjd_int = moment.first;
      mjd_frac = moment.second * DayPerSec();
    }
  }

  void MjdFormat::convert(const moment_type & moment, double & mjd) const {
    long int_part = 0;
    double frac_part = 0.;
    convert(moment, int_part, frac_part);
    mjd = int_part + frac_part;
  }

  void MjdFormat::convert(long mjd_int, double mjd_frac, moment_type & moment) const {
    // Split mjd_frac into integer part and fractional part.
    IntFracPair mjd_frac_split(mjd_frac);

    // Set the value to the moment_type object.
    moment.first = mjd_int + mjd_frac_split.getIntegerPart();
    moment.second = mjd_frac_split.getFractionalPart() * SecPerDay();
  }

  void MjdFormat::convert(double mjd, moment_type & moment) const {
    IntFracPair mjd_int_frac(mjd);
    convert(mjd_int_frac.getIntegerPart(), mjd_int_frac.getFractionalPart(), moment);
  }
}
