/** \file BaryTimeComputer.cxx
    \brief Implementation of BaryTimeComputer class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/BaryTimeComputer.h"

#include "timeSystem/AbsoluteTime.h"
#include "timeSystem/Duration.h"
#include "timeSystem/ElapsedTime.h"
#include "timeSystem/IntFracPair.h"
#include "timeSystem/MjdFormat.h"

#include <cmath>
#include <sstream>
#include <stdexcept>
#include <vector>

extern "C" {
#include "bary.h"
}

namespace timeSystem {

  BaryTimeComputer::BaryTimeComputer(): m_pl_ephem(), m_speed_of_light(0.), m_solar_mass(0.) {}

  BaryTimeComputer::~BaryTimeComputer() {}

  BaryTimeComputer & BaryTimeComputer::getComputer() {
    static BaryTimeComputer s_computer;
    return s_computer;
  }

  std::string BaryTimeComputer::getPlanetaryEphemerisName() const {
    return m_pl_ephem;
  }

  void BaryTimeComputer::initialize(const std::string & pl_ephem) {
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
    std::ostringstream os;
    if (status) {
      os << "Error while initializing ephemeris (status = " << status << ").";
      throw std::runtime_error(os.str());
    } else {
      // Store solar system ephemeris name.
      os << "JPL DE" << denum;
      m_pl_ephem = os.str();
    }

  }

  void BaryTimeComputer::computeBaryTime(const double ra, const double dec, const std::vector<double> & sc_position,
    AbsoluteTime & abs_time) const {
    // Check whether initialized or not.
    if (m_pl_ephem.empty()) {
      throw std::runtime_error("BaryTimeComputer::computeBaryTime was called before initialized.");
    }

    // Check the size of sc_position.
    if (sc_position.size() < 3) {
      throw std::runtime_error("Space craft position was given in a wrong format.");
    }

    // Set given time to a variable to pass to dpleph C-function.
    double jdt[2];
    Mjd mjd(0, 0.);
    abs_time.get("TT", mjd);
    jdt[0] = mjd.m_int + 2400001;
    jdt[1] = mjd.m_frac - 0.5;

    // Read solar system ephemeris for the given time.
    const int iearth = 3;
    const int isun = 11;
    const double *eposn;
    eposn = dpleph(jdt, iearth, isun);
    if (NULL == eposn) {
      std::ostringstream os;
      os << "Could not find solar system ephemeris for " << abs_time.represent("TT", "MJD") << ".";
      throw std::runtime_error(os.str());
    }

    // Compute vectors between related objects.
    std::vector<double> rce(eposn,     eposn + 3);
    std::vector<double> vce(eposn + 3, eposn + 6);
    std::vector<double> rcs(eposn + 6, eposn + 9);
    std::vector<double> rca(3);
    std::vector<double> rsa(3);
    for (int idx = 0; idx < 3; ++idx) {
      // Compute SSBC-to-S/C vector.
      rca[idx] = rce[idx] + sc_position[idx]/m_speed_of_light;

      // Compute Sun-to-S/C vector.
      rsa[idx] = rca[idx] - rcs[idx];
    }

    // Convert source direction from (RA, Dec) to a three-vector.
    std::vector<double> sourcedir = computeThreeVector(ra, dec);

    // Compute total propagation delay.
    double sundis = sqrt(computeInnerProduct(rsa, rsa));
    double cth = computeInnerProduct(sourcedir, rsa) / sundis;
    double delay = computeInnerProduct(sourcedir, rca) + computeInnerProduct(sc_position, vce)/m_speed_of_light
      + 2*m_solar_mass*log(1.+cth);

    // Compute a barycenteric time for the give arrival time.
    abs_time += ElapsedTime("TDB", Duration(delay, "Sec"));
  }

  double BaryTimeComputer::computeInnerProduct(const std::vector<double> & vect_x, const std::vector<double> & vect_y) const {
    return vect_x[0]*vect_y[0] + vect_x[1]*vect_y[1] + vect_x[2]*vect_y[2];
  }

  std::vector<double> BaryTimeComputer::computeThreeVector(const double ra, const double dec) const {
    std::vector<double> vect(3);

    vect[0] = std::cos(ra/RADEG) * std::cos(dec/RADEG);
    vect[1] = std::sin(ra/RADEG) * std::cos(dec/RADEG);
    vect[2] = std::sin(dec/RADEG);

    return vect;
  }

}
