/** \file SourcePosition.cxx
    \brief Implementation of SourcePosition.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/SourcePosition.h"

#include <cmath>
#include <stdexcept>

namespace timeSystem {

  const double SourcePosition::s_one_pi = M_PI;
  const double SourcePosition::s_rad_per_deg  = SourcePosition::s_one_pi / 180.;

  SourcePosition::SourcePosition(double ra, double dec): m_direction(3), m_distance(-1.), m_has_distance(false) {
    setDirection(ra, dec);
  }

  SourcePosition::SourcePosition(double ra, double dec, double distance): m_direction(3), m_distance(distance),
    m_has_distance(true) {
    setDirection(ra, dec);
  }

  SourcePosition::SourcePosition(std::vector<double> direction): m_direction(3), m_distance(-1.), m_has_distance(false) {
    setDirection(direction);
  }

  SourcePosition::SourcePosition(std::vector<double> direction, double distance): m_direction(3), m_distance(distance),
    m_has_distance(true) {
    setDirection(direction);
  }

  const std::vector<double> & SourcePosition::getDirection() const {
    return m_direction;
  }

  double SourcePosition::getDistance() const {
    if (m_has_distance) return m_distance;
    else throw std::runtime_error("Distance not available for this celestial object");
  }

  bool SourcePosition::hasDistance() const {
    return m_has_distance;
  }

  void SourcePosition::setDirection(double ra, double dec) {
    // Convert RA & Dec to Cartesian coordinates.
    double ra_rad = ra * s_rad_per_deg;
    double dec_rad = dec * s_rad_per_deg;
    m_direction[0] = std::cos(ra_rad) * std::cos(dec_rad);
    m_direction[1] = std::sin(ra_rad) * std::cos(dec_rad);
    m_direction[2] = std::sin(dec_rad);
  }

  void SourcePosition::setDirection(const std::vector<double> & direction) {
    // Copy the direction cosine, in a restricted manner.
    for (std::vector<double>::size_type ii = 0; ii < 3; ++ii) {
      m_direction[ii] = (ii < direction.size() ? direction[ii] : 0.);
    }

    // Normalize the direction cosine.
    double length_squared = m_direction[0]*m_direction[0] + m_direction[1]*m_direction[1] + m_direction[2]*m_direction[2];
    if (length_squared > 0.) {
      double length = std::sqrt(length_squared);
      for (std::vector<double>::iterator itor = m_direction.begin(); itor != m_direction.end(); ++itor) (*itor) /= length;
    } else {
      throw std::runtime_error("Direction cosine of non-positive length given for this celestial object");
    }
  }

}
