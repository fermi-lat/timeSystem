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
      /// \brief Destruct this BaryTimeComputer object.
      virtual ~BaryTimeComputer();

      /** \brief Return a BaryTimeComputer object that uses a given solar system ephemeris to compute barycentric times.
          \param pl_ephem Name of solar system ephemeris to use. The name of this argument comes from a "planetary ephemeris".
      */
      static const BaryTimeComputer & getComputer(const std::string & pl_ephem);

      /// \brief Return a character string that represents the planetary ephemeris to be used for barycentric corrections.
      std::string getPlanetaryEphemerisName() const;

      /** \brief Compute a barycentric time for a given time, and update the time with a computed time.
          \param ra Right Ascension of a sky position for which a barycentric time is computed.
          \param dec Declination of a sky position for which a barycentric time is computed.
          \param sc_position Spacecraft position at the time for which a barycentric time is computed.
          \param abs_time Photon arrival time at the spacecraft. This argument is updated to a barycentric time for it.
      */
      virtual void computeBaryTime(double ra, double dec, const std::vector<double> & sc_position, AbsoluteTime & abs_time) const = 0;

      /** \brief Compute a geocentric time for a given time, and update the time with a computed time.
          \param ra Right Ascension of a sky position for which a geocentric time is computed.
          \param dec Declination of a sky position for which a geocentric time is computed.
          \param sc_position Spacecraft position at the time for which a geocentric time is computed.
          \param abs_time Photon arrival time at the spacecraft. This argument is updated to a geocentric time for it.
      */
      virtual void computeGeoTime(double ra, double dec, const std::vector<double> & sc_position, AbsoluteTime & abs_time) const = 0;

    protected:
      /** \brief Construct a BaryTimeComputer object.
          \param pl_ephem Name of solar system ephemeris to use. The name of this argument comes from a "planetary ephemeris".
      */
      BaryTimeComputer(const std::string & pl_ephem);

      /// \brief Initialize this JplComputer object.
      virtual void initializeComputer() = 0;

    private:
      typedef std::map<std::string, BaryTimeComputer *> container_type;

      std::string m_pl_ephem;

      /// \brief Return a container of registered barycentic time computers.
      static container_type & getContainer();
  };

}

#endif
