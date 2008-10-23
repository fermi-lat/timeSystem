/** \file BaryTimeComputer.cxx
    \brief Implementation of BaryTimeComputer class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/BaryTimeComputer.h"

#include "timeSystem/AbsoluteTime.h"
#include "timeSystem/Duration.h"
#include "timeSystem/ElapsedTime.h"
#include "timeSystem/MjdFormat.h"

#include <cctype>
#include <cmath>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

extern "C" {
#include "bary.h"
}

namespace {

  using namespace timeSystem;

  /** \class JplComputer
      \brief Base class to read JPL solar system ephemeris data and compute geocentric and barycentric times.
             Actual tasks to read JPL solar system ephemeris data is deligated to the C functions
             written by Arnold Rots.
  */
  class JplComputer: public BaryTimeComputer {
    public:
      /** \brief Compute a barycentric time for a given time, and update the time with a computed time.
          \param ra Right Ascension of a sky position for which a barycentric time is computed.
          \param dec Declination of a sky position for which a barycentric time is computed.
          \param sc_position Spacecraft position at the time for which a barycentric time is computed.
          \param abs_time Photon arrival time at the spacecraft. This argument is updated to a barycentric time for it.
      */
      virtual void computeBaryTime(double ra, double dec, const std::vector<double> & sc_position, AbsoluteTime & abs_time) const;

      /** \brief Compute a geocentric time for a given time, and update the time with a computed time.
          \param ra Right Ascension of a sky position for which a geocentric time is computed.
          \param dec Declination of a sky position for which a geocentric time is computed.
          \param sc_position Spacecraft position at the time for which a geocentric time is computed.
          \param abs_time Photon arrival time at the spacecraft. This argument is updated to a geocentric time for it.
      */
      virtual void computeGeoTime(double ra, double dec, const std::vector<double> & sc_position, AbsoluteTime & abs_time) const;

    protected:
      /** \brief Construct a JplComputer object.
          \param pl_ephem Name of the JPL planetary ephemeris, such as "JPL DE405".
          \param eph_num Integer number of JPL ephemeris (i.e., 405 for "JPL DE405").
      */
      JplComputer(const std::string & pl_ephem, int eph_num);

      /// \brief Initialize this JplComputer object.
      virtual void initializeComputer();

    private:
      int m_ephnum;
      double m_speed_of_light;
      double m_solar_mass;
      static const JplComputer * m_initialized_computer;

      /** \brief Helper method to compute an inner product of a pair of three-vectors.
          \param vect_x One of the three vector to compute an inner product for.
          \param vect_y The other of the three vector to compute an inner product for.
      */
      double computeInnerProduct(const std::vector<double> & vect_x, const std::vector<double> & vect_y) const;
  };

  /** \class JplDe200Computer
      \brief Class to read JPL DE200 ephemeris and compute geocentric and barycentric times.
  */
  class JplDe200Computer: public JplComputer {
    public:
      /// \brief Construct a JplDe200Computer object.
      JplDe200Computer(): JplComputer("JPL DE200", 200) {}
  };

  /** \class JplDe405Computer
      \brief Class to read JPL DE405 ephemeris and compute geocentric and barycentric times.
  */
  class JplDe405Computer: public JplComputer {
    public:
      /// \brief Construct a JplDe405Computer object.
      JplDe405Computer(): JplComputer("JPL DE405", 405) {}
  };

  const JplComputer * JplComputer::m_initialized_computer(0);

  JplComputer::JplComputer(const std::string & pl_ephem, int eph_num): BaryTimeComputer(pl_ephem), m_ephnum(eph_num),
    m_speed_of_light(0.), m_solar_mass(0.) {}

  void JplComputer::initializeComputer() {
    // Check whether initialized or not.
    if (m_initialized_computer) {
      if (this != m_initialized_computer) {
        // Alrady initialized with a different planetary ephemeris (currently JPL DE200 and DE405 cannot coexist).
        // TODO: Allow JPL DE200 and DE405 to coexist.
        std::ostringstream os;
        os << "Requested planetary ephemeris \"" << getPlanetaryEphemerisName() << "\" cannot coexist with \"" <<
          m_initialized_computer->getPlanetaryEphemerisName() << "\" that is already in use";
        throw std::runtime_error(os.str());
      }

    } else {
      // Call initephem C-function.
      int denum = 0;
      double radsol = 0.;
      int status = initephem(m_ephnum, &denum, &m_speed_of_light, &radsol, &m_solar_mass);

      // Check initialization status.
      if (status) {
        std::ostringstream os;
        os << "Error while initializing ephemeris (status = " << status << ")";
        throw std::runtime_error(os.str());
      }
        
      // Store the pointer to this object to indicate a sucessful initialization.
      m_initialized_computer = this;
    }
  }

  void JplComputer::computeBaryTime(double ra, double dec, const std::vector<double> & sc_position,
    AbsoluteTime & abs_time) const {
    // Check the size of sc_position.
    if (sc_position.size() < 3) {
      throw std::runtime_error("Space craft position was given in a wrong format");
    }

    // Set given time to a variable to pass to dpleph C-function.
    Jd jd_rep(0, 0.);
    abs_time.get("TT", jd_rep);
    double jdt[2] = { jd_rep.m_int, jd_rep.m_frac };

    // Read solar system ephemeris for the given time.
    const int iearth = 3;
    const int isun = 11;
    const double *eposn;
    eposn = dpleph(jdt, iearth, isun);
    if (NULL == eposn) {
      std::ostringstream os;
      os << "Could not find solar system ephemeris for " << abs_time.represent("TT", MjdFmt);
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
    std::vector<double> sourcedir(3);
    sourcedir[0] = std::cos(ra/RADEG) * std::cos(dec/RADEG);
    sourcedir[1] = std::sin(ra/RADEG) * std::cos(dec/RADEG);
    sourcedir[2] = std::sin(dec/RADEG);

    // Compute total propagation delay.
    double sundis = sqrt(computeInnerProduct(rsa, rsa));
    double cth = computeInnerProduct(sourcedir, rsa) / sundis;
    double delay = computeInnerProduct(sourcedir, rca) + computeInnerProduct(sc_position, vce)/m_speed_of_light
      + 2*m_solar_mass*log(1.+cth);

    // Compute a barycenteric time for the give arrival time.
    // Note: Time system used below must be TDB.  By giving "TDB" to the ElapsedTime constructor, the given absolute time
    //       (abs_time variable) is first converted to TDB, then the propagation delay, etc., (delay variable) are added to it.
    //       As a result, the time difference between time systems, TDB - TT, is computed at the given absolute time, and
    //       that is the computation procedure that is needed here.
    //       On the contrary, if "TT" is given, TT-to-TDB conversion would take place after the time difference is added.
    //       In that case, the time difference, TDB - TT, would be computed at a time different from the given absolute time,
    //       and may be significantly different from that at the given absolute time,
    abs_time += ElapsedTime("TDB", Duration(delay, "Sec"));
  }

  void JplComputer::computeGeoTime(double ra, double dec, const std::vector<double> & sc_position,
    AbsoluteTime & abs_time) const {
    // Check the size of sc_position.
    if (sc_position.size() < 3) {
      throw std::runtime_error("Space craft position was given in a wrong format");
    }

    // Convert source direction from (RA, Dec) to a three-vector.
    std::vector<double> sourcedir(3);
    sourcedir[0] = std::cos(ra/RADEG) * std::cos(dec/RADEG);
    sourcedir[1] = std::sin(ra/RADEG) * std::cos(dec/RADEG);
    sourcedir[2] = std::sin(dec/RADEG);

    // Compute total propagation delay.
    double delay = computeInnerProduct(sourcedir, sc_position)/m_speed_of_light;

    // Compute a geocenteric time for the give arrival time.
    abs_time += ElapsedTime("TT", Duration(delay, "Sec"));
  }

  double JplComputer::computeInnerProduct(const std::vector<double> & vect_x, const std::vector<double> & vect_y) const {
    return vect_x[0]*vect_y[0] + vect_x[1]*vect_y[1] + vect_x[2]*vect_y[2];
  }

}

namespace timeSystem {

  BaryTimeComputer::BaryTimeComputer(const std::string & pl_ephem): m_pl_ephem(pl_ephem) {
    std::string uc_pl_ephem = pl_ephem;
    for (std::string::iterator itor = uc_pl_ephem.begin(); itor != uc_pl_ephem.end(); ++itor) *itor = std::toupper(*itor);
    getContainer()[uc_pl_ephem] = this;
  }

  BaryTimeComputer::~BaryTimeComputer() {}

  const BaryTimeComputer & BaryTimeComputer::getComputer(const std::string & pl_ephem) {
    // Create instances of BaryTimeComputer's.
    static JplDe200Computer s_jpl_de200;
    static JplDe405Computer s_jpl_de405;

    // Make the given planeraty ephemeris name case-insensitive.
    std::string pl_ephem_uc(pl_ephem);
    for (std::string::iterator itor = pl_ephem_uc.begin(); itor != pl_ephem_uc.end(); ++itor) *itor = std::toupper(*itor);

    // Find a requested BaryTimeComputer object.
    const container_type & container(getContainer());
    container_type::const_iterator cont_itor = container.find(pl_ephem_uc);
    if (container.end() == cont_itor) {
      throw std::runtime_error("BaryTimeComputer::getComputer could not find a barycentric time computer for planetary ephemeris "
        + pl_ephem);
    }
    BaryTimeComputer & computer(*cont_itor->second);

    // Check whether the chosen computer has already been initialized.
    static std::set<const BaryTimeComputer *> s_initialized;
    std::set<const BaryTimeComputer *>::iterator init_itor = s_initialized.find(&computer);
    if (s_initialized.end() == init_itor) {
      // Initialize the computer on the first request.
      computer.initializeComputer();
      s_initialized.insert(&computer);
    }

    // Return the barycentric time computer.
    return computer;
  }

  std::string BaryTimeComputer::getPlanetaryEphemerisName() const {
    return m_pl_ephem;
  }

  BaryTimeComputer::container_type & BaryTimeComputer::getContainer() {
    static container_type s_container;
    return s_container;
  }

}
