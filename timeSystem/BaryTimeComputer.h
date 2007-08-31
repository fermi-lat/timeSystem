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
      BaryTimeComputer();

      ~BaryTimeComputer();

      void initialize(const std::string & pl_ephem, const std::string & sc_file);

      void correct(double ra, double dec, AbsoluteTime & abs_time);

    private:
      double m_speed_of_light;
      double m_solar_mass;
      std::string m_sc_file;
      char * m_sc_file_char;
      TimeRep * m_mjd_tt;
      TimeRep * m_glast_tt;

      double inner_product(const double vect_x[], const double vect_y[]) const;
  };

}

#endif
