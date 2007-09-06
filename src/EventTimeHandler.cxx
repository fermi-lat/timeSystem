/** \file EventTimeHandler.cxx
    \brief Implementation of EventTimeHandler class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/EventTimeHandler.h"

#include "timeSystem/AbsoluteTime.h"
#include "timeSystem/Duration.h"
#include "timeSystem/ElapsedTime.h"
#include "timeSystem/GlastMetRep.h"
#include "timeSystem/IntFracPair.h"
#include "timeSystem/TimeRep.h"

#include <sstream>
#include <stdexcept>

extern "C" {
#include "bary.h"
}

namespace timeSystem {

  EventTimeHandler::EventTimeHandler(tip::Table & table, double position_tolerance): m_table(&table), m_bary_time(false),
    m_position_tolerance(position_tolerance), m_ra_nom(0.), m_dec_nom(0.), m_speed_of_light(0.), m_solar_mass(0.),
    m_sc_file(), m_mjd_tt(new MjdRep("TT", 0, 0.)) {
    // Get table header.
    const tip::Header & header(m_table->getHeader());

    // Check TELESCOP keyword.
    std::string telescope;
    header["TELESCOP"].get(telescope);
    for (std::string::iterator itor = telescope.begin(); itor != telescope.end(); ++itor) *itor = std::toupper(*itor);
    if (telescope != "GLAST") throw std::runtime_error("Only GLAST supported for now");
    // TODO: Use dicision-making chain to support multiple missions.

    // Set to the first record in the table.
    setFirstRecord();

    // Check whether times in this table are already barycentered.
    std::string time_ref;
    header["TIMEREF"].get(time_ref);
    for (std::string::iterator itor = time_ref.begin(); itor != time_ref.end(); ++itor) *itor = std::toupper(*itor);
    m_bary_time = ("SOLARSYSTEM" == time_ref);

    // Get RA_NOM and DEC_NOM header keywords, if times in this table are barycentered.
    if (m_bary_time) {
      double ra_file;
      double dec_file;
      try {
        header["RA_NOM"].get(ra_file);
        header["DEC_NOM"].get(dec_file);
      } catch (const std::exception &) {
        throw std::runtime_error("Could not find RA_NOM or DEC_NOM header keyword in a barycentered event file.");
      }
      m_ra_nom = ra_file;
      m_dec_nom = dec_file;
    }

    // Pre-compute three vector version of RA_NOM and DEC_NOM.
    computeThreeVector(m_ra_nom, m_dec_nom, m_vect_nom);

    // Get time system from TIMESYS keyword.
    std::string time_system;
    header["TIMESYS"].get(time_system);
    for (std::string::iterator itor = time_system.begin(); itor != time_system.end(); ++itor) *itor = std::toupper(*itor);

    // Create TimeRep.
    // TODO: Consider replacing TimeRep with this class.
    m_time_rep = new GlastMetRep(time_system, 0.);
  }

  EventTimeHandler::~EventTimeHandler() {
    delete m_mjd_tt;
    delete m_time_rep;
  }

  void EventTimeHandler::initialize(const std::string & pl_ephem, const std::string & sc_file) {
    // Convert pl_ephem argument of this method to ephnum argument of initephem C-function.
    int ephnum = 0;
    std::string pl_ephem_uc(pl_ephem);
    for (std::string::iterator itor = pl_ephem_uc.begin(); itor != pl_ephem_uc.end(); ++itor) *itor = std::toupper(*itor);
    if ("JPL DE200" == pl_ephem_uc) ephnum = 200;
    else if ("JPL DE405" == pl_ephem_uc) ephnum = 405;
    else if ("AUTO" == pl_ephem_uc) ephnum = 0;
    else throw std::runtime_error("Solar system ephemeris \"" + pl_ephem + "\" not supported.");

    // Call initephem C-function.
    int denum = 0;
    double radsol = 0.;
    int status = initephem(ephnum, &denum, &m_speed_of_light, &radsol, &m_solar_mass);

    // Check initialization status.
    if (status) {
      std::ostringstream os;
      os << "Error while initializing ephemeris (status = " << status << ").";
      throw std::runtime_error(os.str());
    }

    // Set space craft file to internal variable.
    m_sc_file = sc_file;
    // TODO: Any better way than below?
    m_sc_file_char = (char *)m_sc_file.c_str();

    // Initialize clock and orbit
    // TODO: Accept other missions?  What type (and name) of the argument?
    enum Observatory mission(GLAST);
    scorbitinit (mission);
    clockinit (mission);
  }

  AbsoluteTime EventTimeHandler::readHeader(const std::string & keyword_name) {
    return readHeader(keyword_name, false, 0., 0.);
  }
  
  AbsoluteTime EventTimeHandler::readHeader(const std::string & keyword_name, const double ra, const double dec) {
    if (m_bary_time) {
      // Check RA & Dec in argument list match the table header, if already barycentered.
      checkSkyPosition(ra, dec);

      // Read barycentric time and return it.
      return readHeader(keyword_name, false, ra, dec);

    } else {
      // Delegate computation of barycenteric time and return the result.
      return readHeader(keyword_name, true, ra, dec);
    }
  }

  AbsoluteTime EventTimeHandler::readHeader(const std::string & keyword_name, const bool request_bary_time,
    const double ra, const double dec) {
    // Read keyword value from header as a signle variable of double type.
    const tip::Header & header(m_table->getHeader());
    double keyword_value = 0.;
    header[keyword_name].get(keyword_value);

    // Convert keyword value to AbsoluteTime.
    m_time_rep->set("TIME", keyword_value);
    AbsoluteTime abs_time(*m_time_rep);

    if (request_bary_time) {
      // Get space craft position at the given time.
      int error = 0;
      double * sc_position = xscorbit(m_sc_file_char, keyword_value, &error);

      // Perform barycentric correction on abs_time.
      computeBaryTime(ra, dec, sc_position, abs_time);
    }

    // Return the requested time.
    return abs_time;
  }

  AbsoluteTime EventTimeHandler::readColumn(const std::string & column_name) {
    return readColumn(column_name, false, 0., 0.);
  }
  
  AbsoluteTime EventTimeHandler::readColumn(const std::string & column_name, const double ra, const double dec) {
    if (m_bary_time) {
      // Check RA & Dec in argument list match the table header, if already barycentered.
      checkSkyPosition(ra, dec);

      // Read barycentric time and return it.
      return readColumn(column_name, false, ra, dec);

    } else {
      // Delegate computation of barycenteric time and return the result.
      return readColumn(column_name, true, ra, dec);
    }
  }

  AbsoluteTime EventTimeHandler::readColumn(const std::string & column_name, const bool request_bary_time,
    const double ra, const double dec) {
    // Read column value from a given record as a signle variable of double type.
    double column_value = (*m_record_itor)[column_name].get();

    // Convert keyword value to AbsoluteTime.
    m_time_rep->set("TIME", column_value);
    AbsoluteTime abs_time(*m_time_rep);

    if (request_bary_time) {
      // Get space craft position at the given time.
      int error = 0;
      double * sc_position = xscorbit(m_sc_file_char, column_value, &error);

      // Perform barycentric correction on abs_time.
      computeBaryTime(ra, dec, sc_position, abs_time);
    }

    // Return the requested time.
    return abs_time;
  }

  void EventTimeHandler::setFirstRecord() {
    m_record_itor = m_table->begin();
  }

  void EventTimeHandler::setNextRecord() {
    if (!isEndOfTable()) ++m_record_itor;
  }

  bool EventTimeHandler::isEndOfTable() const {
    return (m_record_itor == m_table->end());
  }

  void EventTimeHandler::computeBaryTime(const double ra, const double dec, const double sc_position[], AbsoluteTime & abs_time) const {
    // Set given time to a variable to pass to dpleph C-function.
    double jdt[2];
    *m_mjd_tt = abs_time;
    long mjd_int = 0;
    double mjd_frac = 0.;
    m_mjd_tt->get("MJDI", mjd_int);
    m_mjd_tt->get("MJDF", mjd_frac);
    // TODO: Implement JdRep and use it instead of MjdRep.
    jdt[0] = mjd_int + 2400001;
    jdt[1] = mjd_frac - 0.5;

    // Read solar system ephemeris for the given time.
    const int iearth = 3;
    const int isun = 11;
    const double *eposn;
    eposn = dpleph(jdt, iearth, isun);
    if (NULL == eposn) {
      std::ostringstream os;
      os << "Could not find solar system ephemeris for " << *m_mjd_tt << ".";
      throw std::runtime_error(os.str());
    }

    // Compute vectors between related objects.
    const double *rce, *rcs, *vce;
    double rca[3], rsa[3];
    rce = eposn;
    vce = eposn + 3;
    rcs = eposn + 6;
    for (int idx = 0; idx < 3; ++idx) {
      // Compute SSBC-to-S/C vector.
      rca[idx] = rce[idx] + sc_position[idx]/m_speed_of_light;

      // Compute Sun-to-S/C vector.
      rsa[idx] = rca[idx] - rcs[idx];
    }

    // Convert source direction from (RA, Dec) to a three-vector.
    double sourcedir[3];
    computeThreeVector(ra, dec, sourcedir);

    // Compute total propagation delay.
    double sundis = sqrt(computeInnerProduct(rsa, rsa));
    double cth = computeInnerProduct(sourcedir, rsa) / sundis;
    double delay = computeInnerProduct(sourcedir, rca) + computeInnerProduct(sc_position, vce)/m_speed_of_light
      + 2*m_solar_mass*log(1.+cth);

    // Compute a barycenteric time for the give arrival time.
    abs_time += ElapsedTime("TDB", Duration(IntFracPair(delay), Sec));
  }

  double EventTimeHandler::computeInnerProduct(const double vect_x[], const double vect_y[]) const {
    return vect_x[0]*vect_y[0] + vect_x[1]*vect_y[1] + vect_x[2]*vect_y[2];
  }

  void EventTimeHandler::computeOuterProduct(const double vect_x[], const double vect_y[], double vect_z[]) const {
    vect_z[0] = vect_x[1]*vect_y[2] - vect_x[2]*vect_y[1];
    vect_z[1] = vect_x[2]*vect_y[0] - vect_x[0]*vect_y[2];
    vect_z[2] = vect_x[0]*vect_y[1] - vect_x[1]*vect_y[0];
  }

  void EventTimeHandler::computeThreeVector(const double ra, const double dec, double vect[]) const {
    vect[0] = std::cos(ra/RADEG) * std::cos(dec/RADEG);
    vect[1] = std::sin(ra/RADEG) * std::cos(dec/RADEG);
    vect[2] = std::sin(dec/RADEG);
  }

  void EventTimeHandler::checkSkyPosition(const double ra, const double dec) const {
    double source[3];
    computeThreeVector(ra, dec, source);

    double outer[3] = {0., 0., 0.};
    computeOuterProduct(source, m_vect_nom, outer);

    double separation_angle = std::asin(sqrt(computeInnerProduct(outer, outer))) * RADEG;

    if (separation_angle > m_position_tolerance) {
      std::ostringstream os;
      os << "Sky position for barycentric corrections (RA=" << ra << ", Dec=" << dec << 
        ") does not match RA_NOM (" << m_ra_nom << ") and DEC_NOM (" << m_dec_nom << ") in Event file.";
      throw std::runtime_error(os.str());
    }

  }
}
