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
#include "timeSystem/SourcePosition.h"

#include <cctype>
#include <cmath>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

extern "C" {
// Copied from bary.h.
#define RADEG   57.2957795130823
int initephem (int, int *, double *, double *, double *) ;
const double *dpleph (double *, int, int) ;
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
          \param src_position Position of the celestial object for which a barycentric time is computed.
          \param obs_position Observatory position at the time for which a barycentric time is computed. The position must be
                 given in the form of Cartesian coordinates in meters in the equatorial coordinate system with the origin at
                 the center of the Earth.
          \param abs_time Photon arrival time at the spacecraft. This argument is updated to a barycentric time for it.
      */
      virtual void computeBaryTime(const SourcePosition & src_position, const std::vector<double> & obs_position,
        AbsoluteTime & abs_time) const;

      /** \brief Compute a geocentric time for a given time, and update the time with a computed time.
          \param src_position Position of the celestial object for which a geocentric time is computed.
          \param obs_position Observatory position at the time for which a geocentric time is computed. The position must be
                 given in the form of Cartesian coordinates in meters in the equatorial coordinate system with the origin at
                 the center of the Earth.
          \param abs_time Photon arrival time at the spacecraft. This argument is updated to a geocentric time for it.
      */
      virtual void computeGeoTime(const SourcePosition & src_position, const std::vector<double> & obs_position,
        AbsoluteTime & abs_time) const;

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

      /** \brief Helper method to compute (and return) a time delay for a geocentric or a barycentric correction.
          \param src_position Position of the celestial object for which a geo/barycentric time is computed.
          \param obs_position Observatory position at the time for which a geo/barycentric time is computed. The position must be
                 given in the form of Cartesian coordinates in meters in the equatorial coordinate system with the origin at
                 the center of the Earth.
          \param abs_time Photon arrival time at the spacecraft. This argument is updated to a geo/barycentric time for it.
          \param barycentric If true, a time delay for a barycentric correction is computed. If false, one for a geocentric
                 correction is computed.
      */
      double computeTimeDelay(const SourcePosition & src_position, const std::vector<double> & obs_position,
        AbsoluteTime & abs_time, bool barycentric) const;

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

  void JplComputer::computeBaryTime(const SourcePosition & src_position, const std::vector<double> & obs_position,
    AbsoluteTime & abs_time) const {
    // Compute a time delay for the barycentric correction.
    double delay = computeTimeDelay(src_position, obs_position, abs_time, true);

    // Compute a barycenteric time for the give arrival time.
    // Note: Time system used below must be TDB.  By giving "TDB" to the ElapsedTime constructor, the given absolute time
    //       (abs_time variable) is first converted to TDB, then the propagation delay, etc., (delay variable) are added to it.
    //       As a result, the time difference between time systems, TDB - TT, is computed at the given absolute time, and
    //       that is the computation procedure that is needed here.
    //       On the contrary, if "TT" is given, TT-to-TDB conversion would take place after the time difference is added.
    //       In that case, the time difference, TDB - TT, would be computed at a time different from the given absolute time,
    //       and may be significantly different from that at the given absolute time.
    abs_time += ElapsedTime("TDB", Duration(delay, "Sec"));
  }

  void JplComputer::computeGeoTime(const SourcePosition & src_position, const std::vector<double> & obs_position,
    AbsoluteTime & abs_time) const {
    // Compute a time delay for the geocentric correction.
    double delay = computeTimeDelay(src_position, obs_position, abs_time, false);

    // Compute a geocenteric time for the give arrival time.
    abs_time += ElapsedTime("TT", Duration(delay, "Sec"));
  }

  double JplComputer::computeTimeDelay(const SourcePosition & src_position, const std::vector<double> & obs_position,
    AbsoluteTime & abs_time, bool barycentric) const {
    // Check the size of obs_position.
    if (obs_position.size() < 3) {
      throw std::runtime_error("Space craft position was given in a wrong format");
    }

    // Prepare the return value.
    double delay = 0.;

    // Read solar system ephemeris when necessary.
    const double * rce = 0;
    const double * vce = 0;
    const double * rcs = 0;
    if (barycentric || src_position.hasDistance()) {
      // Set given time to a variable to pass to dpleph C-function.
      Jd jd_rep(0, 0.);
      abs_time.get("TT", jd_rep);
      double jdt[2] = { jd_rep.m_int, jd_rep.m_frac };

      // Read solar system ephemeris for the given time.
      const int iearth = 3;
      const int isun = 11;
      const double * eposn = 0;
      eposn = dpleph(jdt, iearth, isun);
      if (NULL == eposn) {
        std::ostringstream os;
        os << "Could not find solar system ephemeris for " << abs_time.represent("TT", MjdFmt);
        throw std::runtime_error(os.str());
      }

      // Set pointer values for convenience.
      rce = eposn;     // SSBC-to-Earth vector.
      vce = eposn + 3; // Earth velocity with respect to SSBC.
      rcs = eposn + 6; // SSBC-to-Sun vector.
    }

    // Compute the vector pointing from the geo/barycenter to the spacecraft.
    std::vector<double> origin_to_observer(3);
    for (int idx = 0; idx < 3; ++idx) origin_to_observer[idx] = obs_position[idx]/m_speed_of_light;
    if (barycentric) for (int idx = 0; idx < 3; ++idx) origin_to_observer[idx] += rce[idx];

    // Compute the Roemer delay and the direction of the line of sight.
    std::vector<double> line_of_sight(3);
    if (src_position.hasDistance()) {
      // Compute the vector pointing from the geo/barycenter to the source.
      std::vector<double> origin_to_source = src_position.getDirection();
      for (int idx = 0; idx < 3; ++idx) origin_to_source[idx] *= src_position.getDistance();
      if (!barycentric) for (int idx = 0; idx < 3; ++idx) origin_to_source[idx] -= rce[idx];

      // Compute the vector pointing from the spacecraft to the source.
      std::vector<double> observer_to_source(3);
      for (int idx = 0; idx < 3; ++idx) {
        observer_to_source[idx] = origin_to_source[idx] - origin_to_observer[idx];
      }

      // Compute the unit vector parallel to the line of sight.
      double length = std::sqrt(computeInnerProduct(observer_to_source, observer_to_source));
      for (int idx = 0; idx < 3; ++idx) line_of_sight[idx] = observer_to_source[idx] / length;

      // Compute the Roemer delay, taking into account of the curvature of spherical wavefront.
      // Note: The following computation is exact in general cases.  Letting
      //          x = origin_to_source, y = observer_to_source, and z = origin_to_observer,
      //       then one obtains the Roemer delay by
      //          delay = |x| - |y|
      //                = (x + y) * z / (|x| + |y|)
      //       where z = x - y by definition.
      double sum_length = std::sqrt(computeInnerProduct(origin_to_source, origin_to_source));
      sum_length += std::sqrt(computeInnerProduct(observer_to_source, observer_to_source));
      if (sum_length == 0.) throw std::runtime_error("Distance to the source is computed as zero (0) in the barycentric correction");
      for (int idx = 0; idx < 3; ++idx) {
        delay += (origin_to_source[idx] + observer_to_source[idx]) * origin_to_observer[idx] / sum_length;
      }

    } else {
      // Take the original source direction as the line of sight, assuming the wavefront is planar.
      line_of_sight = src_position.getDirection();

      // Compute the Roemer delay, assuming the wavefront is planar.
      delay += computeInnerProduct(line_of_sight, origin_to_observer);
    }

    // Compute additional time delays for the barycentric correction.
    if (barycentric) {
      // Compute the Einstein delay.
      std::vector<double> earth_velocity(vce, vce + 3);
      delay += computeInnerProduct(obs_position, earth_velocity)/m_speed_of_light;

      // Compute the vector pointing from the Sun to the spacecraft (to be used for the Shapiro delay).
      std::vector<double> sun_to_observer(3); // Sun-to-S/C vector.
      for (int idx = 0; idx < 3; ++idx) sun_to_observer[idx] = origin_to_observer[idx] - rcs[idx];

      // Compute the Shapiro delay.
      double sundis = std::sqrt(computeInnerProduct(sun_to_observer, sun_to_observer));
      double cth = computeInnerProduct(line_of_sight, sun_to_observer) / sundis;
      delay += 2. * m_solar_mass * std::log(1. + cth);
    }

    // Return the computed time delay.
    return delay;
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
