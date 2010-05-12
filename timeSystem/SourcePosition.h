/** \file SourcePosition.h
    \brief Declaration of SourcePosition.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_SourcePosition_h
#define timeSystem_SourcePosition_h

#include <vector>

namespace timeSystem {

  /** \class SourcePosition
      \brief Class representing the position of a celestial object in the equatorial coordinate system with the origin
             at the solar system barycenter, including the distance to the celestial object if available.
  */
  class SourcePosition {
    public:
      static const double s_one_pi;
      static const double s_rad_per_deg;

      /** \brief Construct a SourcePosition object. The distance to the celestial object is marked unknown.
          \param ra Right Ascension of the celestial object in degrees.
          \param dec Declination of the celestial object in degrees.
      */
      SourcePosition(double ra, double dec);

      /** \brief Construct a SourcePosition object, with the distance to the celestial object.
          \param ra Right Ascension of the celestial object in degrees.
          \param dec Declination of the celestial object in degrees.
          \param distance Distance to the celestial object in light-seconds.
      */
      SourcePosition(double ra, double dec, double distance);

      /** \brief Construct a SourcePosition object. The distance to the celestial object is marked unknown.
          \param direction Three vector representing a direction cosine pointing to the celestial object,
                 with the x coordinate in direction[0], y in direction[1], and z in direction[2]. If the size
                 of this argument is smaller than 3, then missing coordinate(s) are assumed to be zero (0.).
                 The fourth element and later are ignored.  After copied into an internal variable, the
                 direction cosine is normalized if the vector length is positive, or the method throws an
                 exception if otherwise.
      */
      SourcePosition(std::vector<double> direction);

      /** \brief Construct a SourcePosition object, with the distance to the celestial object.
          \param direction Three vector representing a direction cosine pointing to the celestial object from the
                 solar system barycenter, with the x coordinate in direction[0], y in direction[1], and z in direction[2].
                 If the size of this argument is smaller than 3, then missing coordinate(s) are assumed to be zero (0.).
                 The fourth element and later are ignored.  After copied into an internal variable, the
                 direction cosine is normalized if the vector length is positive, or the method throws an
                 exception if otherwise.
          \param distance Distance to the celestial object in light-seconds.
      */
      SourcePosition(std::vector<double> direction, double distance);

      /** \brief Return direction cosines of the celestial object (dimensionless) in the equatorial coordinate system
                 with the origin at the solar system barycenter.
      */
      const std::vector<double> & getDirection() const;

      /** \brief Return the distance to the celestial object from the solar system barycenter in light-seconds.
                 An exception is thrown if the distance is not known.
      */
      double getDistance() const;

      /// \brief Return true if the distance to the celestial object is available, and false if otherwise.
      bool hasDistance() const;

    private:
      std::vector<double> m_direction;
      double m_distance;
      bool m_has_distance;

      /** \brief Convert RA and Dec to the Cartesian coordinates and set them to the data member.
          \param ra Right Ascension of the celestial object in degrees.
          \param dec Declination of the celestial object in degrees.
      */
      void setDirection(double ra, double dec);

      /** \brief Set the given direction to the data member.
          \param direction Three vector representing a direction cosine pointing to the celestial object,
                 with the x coordinate in direction[0], y in direction[1], and z in direction[2]. If the size
                 of this argument is smaller than 3, then missing coordinate(s) are assumed to be zero (0.).
                 The fourth element and later are ignored.  After copied into an internal variable, the
                 direction cosine is normalized if the vector length is positive, or the method throws an
                 exception if otherwise.
      */
      void setDirection(const std::vector<double> & direction);
  };

}

#endif
