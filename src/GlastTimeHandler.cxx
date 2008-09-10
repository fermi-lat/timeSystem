/** \file GlastTimeHandler.cxx
    \brief Implementation of GlastTimeHandler class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/GlastTimeHandler.h"

#include "timeSystem/AbsoluteTime.h"
#include "timeSystem/BaryTimeComputer.h"
#include "timeSystem/ElapsedTime.h"

#include "tip/IFileSvc.h"

#include <cctype>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <vector>

extern "C" {
#include "bary.h"
}

namespace timeSystem {

  GlastTimeHandler::GlastTimeHandler(const std::string & file_name, const std::string & extension_name, bool read_only):
    EventTimeHandler(file_name, extension_name, read_only), m_time_system(0), m_mjd_ref(0, 0.) {
    // Get time system name from TIMESYS keyword.
    const tip::Header & header(getHeader());
    std::string time_system_name;
    header["TIMESYS"].get(time_system_name);

    // Get a TimeSystem object.
    m_time_system = &TimeSystem::getSystem(time_system_name);

    // Get MJDREF value.
    m_mjd_ref = readMjdRef(header);
  }

  GlastTimeHandler::~GlastTimeHandler() {}

  EventTimeHandler * GlastTimeHandler::createInstance(const std::string & file_name, const std::string & extension_name,
    bool read_only) {
    // Try GlastScTimeHandler first.
    EventTimeHandler * handler = GlastScTimeHandler::createInstance(file_name, extension_name, read_only);

    // Try GlastBaryTimeHandler next.
    if (0 == handler) handler = GlastBaryTimeHandler::createInstance(file_name, extension_name, read_only);

    // Return the handler (or zero if those classes above cannot handle it).
    return handler;
  }

  AbsoluteTime GlastTimeHandler::readTime(const std::string & field_name, bool from_header) const {
    // Read the field value as a GLAST time.
    double glast_time = readGlastTime(field_name, from_header);

    // Convert GLAST time to AbsoluteTime, and return it.
    return computeAbsoluteTime(glast_time);
  }

  AbsoluteTime GlastTimeHandler::parseTimeString(const std::string & time_string, const std::string & time_system) const {
    // Rationalize time system name.
    std::string time_system_rat(time_system);
    for (std::string::iterator itor = time_system_rat.begin(); itor != time_system_rat.end(); ++itor) *itor = std::toupper(*itor);
    if ("FILE" == time_system_rat) time_system_rat = m_time_system->getName();

    // Parse time string into an absolute time, and return it.
    std::istringstream iss(time_string);
    double time_double = 0.;
    iss >> time_double;
    if (iss.fail() || !iss.eof()) throw std::runtime_error("Cannot interpret \"" + time_string + "\" as a GLAST event time");

    // Create and return the time.
    return computeAbsoluteTime(time_double, time_system_rat);
  }

  bool GlastTimeHandler::checkHeaderKeyword(const std::string & file_name, const std::string & extension_name,
    const std::string & time_ref_value, const std::string & time_sys_value) {
    // Get the table and the header.
    std::auto_ptr<const tip::Table> table(tip::IFileSvc::instance().readTable(file_name, extension_name));
    const tip::Header & header(table->getHeader());

    // Get TELESCOP keyword value.
    std::string telescope;
    header["TELESCOP"].get(telescope);
    for (std::string::iterator itor = telescope.begin(); itor != telescope.end(); ++itor) *itor = std::toupper(*itor);

    // Get INSTRUME keyword value.
    std::string instrument;
    header["INSTRUME"].get(instrument);
    for (std::string::iterator itor = instrument.begin(); itor != instrument.end(); ++itor) *itor = std::toupper(*itor);

    // Get TIMEREF keyword value to check whether times in this table are already barycentered.
    std::string time_ref;
    header["TIMEREF"].get(time_ref);
    for (std::string::iterator itor = time_ref.begin(); itor != time_ref.end(); ++itor) *itor = std::toupper(*itor);
    std::string time_ref_arg(time_ref_value);
    for (std::string::iterator itor = time_ref_arg.begin(); itor != time_ref_arg.end(); ++itor) *itor = std::toupper(*itor);

    // Get TIMESYS keyword value to check whether times in this table are already barycentered.
    std::string time_sys;
    header["TIMESYS"].get(time_sys);
    for (std::string::iterator itor = time_sys.begin(); itor != time_sys.end(); ++itor) *itor = std::toupper(*itor);
    std::string time_sys_arg(time_sys_value);
    for (std::string::iterator itor = time_sys_arg.begin(); itor != time_sys_arg.end(); ++itor) *itor = std::toupper(*itor);

    // Return whether this class can handle the file or not.
    return (telescope == "GLAST" && instrument == "LAT" && time_ref == time_ref_arg && time_sys == time_sys_arg);
  }

  double GlastTimeHandler::readGlastTime(const std::string & field_name, bool from_header) const {
    double field_value = 0.;
    if (from_header) {
      const tip::Header & header(getHeader());
      header[field_name].get(field_value);
    } else {
      const tip::TableRecord & record(getCurrentRecord());
      record[field_name].get(field_value);
    }
    return field_value;
  }

  AbsoluteTime GlastTimeHandler::computeAbsoluteTime(double glast_time) const {
    // Convert GLAST time to AbsoluteTime, and return it.
    return computeAbsoluteTime(glast_time, m_time_system->getName());
  }

  AbsoluteTime GlastTimeHandler::computeAbsoluteTime(double glast_time, const std::string & time_system_name) const {
    // Convert GLAST time to AbsoluteTime, and return it.
    return AbsoluteTime(time_system_name, m_mjd_ref) + ElapsedTime(time_system_name, Duration(glast_time, "Sec"));
  }

  GlastScTimeHandler::GlastScTimeHandler(const std::string & file_name, const std::string & extension_name, bool read_only):
    GlastTimeHandler(file_name, extension_name, read_only), m_sc_file(), m_sc_file_char(0), m_ra_bary(0.), m_dec_bary(0.),
    m_computer(0) {}

  GlastScTimeHandler::~GlastScTimeHandler() {}

  EventTimeHandler * GlastScTimeHandler::createInstance(const std::string & file_name, const std::string & extension_name,
    bool read_only) {
    // Create an object to hold a return value and set a default return value.
    EventTimeHandler * handler(0);

    // Check header keywords to identify an event file with barycentric corrections NOT applied.
    if (checkHeaderKeyword(file_name, extension_name, "LOCAL", "TT")) {
      handler = new GlastScTimeHandler(file_name, extension_name, read_only);
    }

    // Return the handler (or zero if this class cannot handle it).
    return handler;
  }

  void GlastScTimeHandler::initTimeCorrection(const std::string & sc_file_name, const std::string & sc_extension_name,
     const std::string & solar_eph, bool /*match_solar_eph*/, double /*angular_tolerance*/) {
    // Check header keywords.
    if (!checkHeaderKeyword(sc_file_name, sc_extension_name, "LOCAL", "TT")) {
      throw std::runtime_error("Unsupported spacecraft file \"" + sc_file_name + "[" + sc_extension_name + "]\"");
    }

    // Set spacecraft file to internal variable.
    m_sc_file = sc_file_name;
    m_sc_file_char = const_cast<char *>(m_sc_file.c_str());

    // Initializing clock and orbit are not necessary for GLAST.
    // Note: Leave these here as a reminder of an official way to call them.
    //enum Observatory mission(GLAST);
    //scorbitinit(mission);
    //clockinit(mission);

    // Get a barycentric time computer for the given solar system ephemeris.
    m_computer = &BaryTimeComputer::getComputer(solar_eph);
  }

  void GlastScTimeHandler::setSourcePosition(double ra, double dec) {
    // Set RA & Dec to internal variables.
    m_ra_bary = ra;
    m_dec_bary = dec;
  }

  AbsoluteTime GlastScTimeHandler::getBaryTime(const std::string & field_name, bool from_header) const {
    // Check initialization status.
    if (!m_computer) throw std::runtime_error("Arrival time corrections not initialized.");

    // Read the field value as a GLAST time.
    double glast_time = readGlastTime(field_name, from_header);

    // Convert GLAST time to AbsoluteTime.
    AbsoluteTime abs_time = computeAbsoluteTime(glast_time);

    // Get space craft position at the given time.
    int error = 0;
    double * sc_position_array = glastscorbit(m_sc_file_char, glast_time, &error);
    if (error) {
      std::ostringstream os;
      os << "Error in getting GLAST spacecraft position for " << glast_time << " GLAST MET (TT).";
      throw std::runtime_error(os.str());
    }
    std::vector<double> sc_position(sc_position_array, sc_position_array + 3);

    // Perform barycentric correction on abs_time.
    m_computer->computeBaryTime(m_ra_bary, m_dec_bary, sc_position, abs_time);

    // Return the requested time.
    return abs_time;
  }

  GlastBaryTimeHandler::GlastBaryTimeHandler(const std::string & file_name, const std::string & extension_name, bool read_only):
    GlastTimeHandler(file_name, extension_name, read_only), m_file_name(file_name), m_ext_name(extension_name), m_ra_nom(0.),
    m_dec_nom(0.), m_vect_nom(3), m_max_vect_diff(0.), m_pl_ephem() {}

  GlastBaryTimeHandler::~GlastBaryTimeHandler() {}

  EventTimeHandler * GlastBaryTimeHandler::createInstance(const std::string & file_name, const std::string & extension_name,
    bool read_only) {
    // Create an object to hold a return value and set a default return value.
    EventTimeHandler * handler(0);

    // Check header keywords to identify an event file with barycentric corrections applied.
    if (checkHeaderKeyword(file_name, extension_name, "SOLARSYSTEM", "TDB")) {
      handler = new GlastBaryTimeHandler(file_name, extension_name, read_only);
    }

    // Return the handler (or zero if this class cannot handle it).
    return handler;
  }

  void GlastBaryTimeHandler::initTimeCorrection(const std::string & /*sc_file_name*/, const std::string & /*sc_extension_name*/,
     const std::string & solar_eph, bool match_solar_eph, double angular_tolerance) {
    // Get table header.
    const tip::Header & header(getHeader());

    // Get RA_NOM and DEC_NOM header keywords, if times in this table are barycentered.
    double ra_file = 0.;
    double dec_file = 0.;
    try {
      header["RA_NOM"].get(ra_file);
      header["DEC_NOM"].get(dec_file);
    } catch (const std::exception &) {
      throw std::runtime_error("Could not find RA_NOM or DEC_NOM header keyword in a barycentered event file.");
    }
    m_ra_nom = ra_file;
    m_dec_nom = dec_file;

    // Pre-compute three vector version of RA_NOM and DEC_NOM.
    m_vect_nom = computeThreeVector(m_ra_nom, m_dec_nom);

    // Pre-compute threshold in sky position comparison.
    m_max_vect_diff = 2. * std::sin(angular_tolerance / 2. / RADEG);
    m_max_vect_diff *= m_max_vect_diff;

    // Get PLEPHEM header keywords, if times in this table are barycentered.
    std::string pl_ephem;
    try {
      header["PLEPHEM"].get(pl_ephem);
    } catch (const std::exception &) {
      throw std::runtime_error("Could not find PLEPHEM header keyword in a barycentered event file.");
    }
    m_pl_ephem = pl_ephem;

    // Check solar system ephemeris if requested to match.
    if (match_solar_eph) {
      // Make the names of solar system ephemeris case insensitive.
      std::string solar_eph_uc(solar_eph);
      for (std::string::iterator itor = solar_eph_uc.begin(); itor != solar_eph_uc.end(); ++itor) *itor = std::toupper(*itor);
      std::string pl_ephem_uc(m_pl_ephem);
      for (std::string::iterator itor = pl_ephem_uc.begin(); itor != pl_ephem_uc.end(); ++itor) *itor = std::toupper(*itor);

      // Check whether the names match each other, with a little artificial tolerance.
      bool solar_eph_match = ((pl_ephem_uc == solar_eph_uc) 
                              || (pl_ephem_uc == "JPL-DE200" && solar_eph_uc == "JPL DE200")
                              || (pl_ephem_uc == "JPL-DE405" && solar_eph_uc == "JPL DE405"));

      // Throw an exception the names do not match.
      if (!solar_eph_match) {
        throw std::runtime_error("Solar system ephemeris in extension \"" + m_ext_name + "\" of file \"" + m_file_name +
          "\" (PLEPHEM=\"" + m_pl_ephem + "\") does not match the requested \"" + solar_eph + "\".");
      }
    }
  }

  void GlastBaryTimeHandler::setSourcePosition(double ra, double dec) {
    // Check RA & Dec in argument list match the table header, if already barycentered.
    std::vector<double> source = computeThreeVector(ra, dec);
    double x_diff = source[0] - m_vect_nom[0];
    double y_diff = source[1] - m_vect_nom[1];
    double z_diff = source[2] - m_vect_nom[2];
    double r_diff = x_diff*x_diff + y_diff*y_diff + z_diff*z_diff;

    if (m_max_vect_diff < r_diff) {
      std::ostringstream os;
      os << "Sky position for barycentric corrections (RA=" << ra << ", Dec=" << dec << 
        ") does not match RA_NOM (" << m_ra_nom << ") and DEC_NOM (" << m_dec_nom << ") in Event file.";
      throw std::runtime_error(os.str());
    }
  }

  AbsoluteTime GlastBaryTimeHandler::getBaryTime(const std::string & field_name, bool from_header) const {
    return readTime(field_name, from_header);
  }

  std::vector<double> GlastBaryTimeHandler::computeThreeVector(double ra, double dec) const {
    std::vector<double> vect(3);

    vect[0] = std::cos(ra/RADEG) * std::cos(dec/RADEG);
    vect[1] = std::sin(ra/RADEG) * std::cos(dec/RADEG);
    vect[2] = std::sin(dec/RADEG);

    return vect;
  }
}
