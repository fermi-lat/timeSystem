/** \file BaryTimeComputer.h
    \brief Declaration of BaryTimeComputer class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_BaryTimeComputer_h
#define timeSystem_BaryTimeComputer_h

#include <string>

namespace timeSystem {

  class AbsoluteTime;
  class TimeRep;

  /** \class BaryTimeComputer
      \brief Class which performs barycentric correction on photon arrival times, typically recorded at a space craft.
  */
  class BaryTimeComputer {
    public:
      ~BaryTimeComputer();

      static BaryTimeComputer & getComputer();

      std::string getPlanetaryEphemerisName() const;

      void initialize(const std::string & pl_ephem);

      void computeBaryTime(const double ra, const double dec, const double sc_position[], AbsoluteTime & abs_time) const;

    private:
      std::string m_pl_ephem;
      double m_speed_of_light;
      double m_solar_mass;
      TimeRep * m_mjd_tt;

      BaryTimeComputer();

      double computeInnerProduct(const double vect_x[], const double vect_y[]) const;

      void computeThreeVector(const double ra, const double dec, double vect[]) const;
  };

}

#endif
