/** \file BaryTimeComputer.cxx
    \brief Implementation of BaryTimeComputer class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/BaryTimeComputer.h"

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

  BaryTimeComputer::BaryTimeComputer(): m_speed_of_light(0.), m_solar_mass(0.), m_sc_file(), m_mjd_tt(new MjdRep("TT", 0, 0.)),
    m_glast_tt(new GlastMetRep("TT", 0.)) {}

  BaryTimeComputer::~BaryTimeComputer() {
    delete m_glast_tt;
    delete m_mjd_tt;
  }

  void BaryTimeComputer::initialize(const std::string & pl_ephem, const std::string & sc_file) {
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

  void BaryTimeComputer::correct(double ra, double dec, AbsoluteTime & abs_time) {
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

    // Compute Mission Elapsed Time from abs_time.
    // TODO: Re-consider this design; abs_time is computed from MET outside of this method, in most cases,
    // and here is needed its reverse operation, suggesting this is a design flaw.
    *m_glast_tt = abs_time;
    double met = 0.;
    m_glast_tt->get("TIME", met);

    // Get space craft position at the given time.
    int error = 0;
    double * scposn = xscorbit(m_sc_file_char, met, &error);

    // Compute vectors between related objects.
    const double *rce, *rcs, *vce;
    double rca[3], rsa[3];
    rce = eposn;
    vce = eposn + 3;
    rcs = eposn + 6;
    for (int idx = 0; idx < 3; ++idx) {
      // Compute SSBC-to-S/C vector.
      rca[idx] = rce[idx] + scposn[idx]/m_speed_of_light;

      // Compute Sun-to-S/C vector.
      rsa[idx] = rca[idx] - rcs[idx];
    }

    // Convert source direction from (RA, Dec) to a three-vector.
    double sourcedir[3];
    sourcedir[0] = cos(ra/RADEG) * cos(dec/RADEG);
    sourcedir[1] = sin(ra/RADEG) * cos(dec/RADEG);
    sourcedir[2] = sin(dec/RADEG);

    // Compute total propagation delay.
    double sundis = sqrt(inner_product(rsa, rsa));
    double cth = inner_product(sourcedir, rsa) / sundis;
    double delay = inner_product(sourcedir, rca) + inner_product(scposn, vce)/m_speed_of_light + 2*m_solar_mass*log(1.+cth);

    // Compute a barycenteric time for the give arrival time.
    abs_time += ElapsedTime("TDB", Duration(IntFracPair(delay), Sec));
  }

  double BaryTimeComputer::inner_product(const double vect_x[], const double vect_y[]) const {
    return vect_x[0]*vect_y[0] + vect_x[1]*vect_y[1] + vect_x[2]*vect_y[2];
  }

}
