/** \file BaryTimeComputer.h
    \brief Declaration of BaryTimeComputer class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_BaryTimeComputer_h
#define timeSystem_BaryTimeComputer_h

#include <map>
#include <string>
#include <vector>

namespace timeSystem {

  class AbsoluteTime;

  /** \class BaryTimeComputer
      \brief Class which performs barycentric correction on photon arrival times, typically recorded at a space craft.
  */
  class BaryTimeComputer {
    public:
      virtual ~BaryTimeComputer();

      static const BaryTimeComputer & getComputer(const std::string & pl_ephem);

      std::string getPlanetaryEphemerisName() const;

      virtual void computeBaryTime(double ra, double dec, const std::vector<double> & sc_position, AbsoluteTime & abs_time) const = 0;

      virtual void computeGeoTime(double ra, double dec, const std::vector<double> & sc_position, AbsoluteTime & abs_time) const = 0;

    protected:
      BaryTimeComputer(const std::string & pl_ephem);

      virtual void initializeComputer() = 0;

    private:
      typedef std::map<std::string, BaryTimeComputer *> container_type;

      std::string m_pl_ephem;

      static container_type & getContainer();
  };

}

#endif
