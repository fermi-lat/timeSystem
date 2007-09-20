/** \file EventTimeHandler.cxx
    \brief Implementation of EventTimeHandler class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/EventTimeHandler.h"

#include "timeSystem/AbsoluteTime.h"
#include "timeSystem/BaryTimeComputer.h"

#include "tip/IFileSvc.h"

#include <cmath>
#include <sstream>
#include <stdexcept>
#include <vector>

extern "C" {
#include "bary.h"
}

namespace timeSystem {

  IEventTimeHandlerFactory::IEventTimeHandlerFactory() {
    registerHandler();
  }

  IEventTimeHandlerFactory::~IEventTimeHandlerFactory() {
    deregisterHandler();
  }

  void IEventTimeHandlerFactory::registerHandler() {
    getFactoryContainer().push_back(this);
  }

  void IEventTimeHandlerFactory::deregisterHandler() {
    getFactoryContainer().remove(this);
  }

  IEventTimeHandlerFactory::cont_type & IEventTimeHandlerFactory::getFactoryContainer() {
    static cont_type s_factory_cont;
    return s_factory_cont;
  }

  EventTimeHandler::EventTimeHandler(const std::string & file_name, const std::string & extension_name, const double angular_tolerance):
    m_table(0), m_bary_time(false), m_ra_nom(0.), m_dec_nom(0.), m_vect_nom(3), m_max_vect_diff(0.),
    m_computer(BaryTimeComputer::getComputer()) {
    // Get the table.
    m_table = tip::IFileSvc::instance().editTable(file_name, extension_name);

    // Get table header.
    const tip::Header & header(m_table->getHeader());

    // Set to the first record in the table.
    setFirstRecord();

    // Check whether times in this table are already barycentered.
    std::string time_ref;
    header["TIMEREF"].get(time_ref);
    for (std::string::iterator itor = time_ref.begin(); itor != time_ref.end(); ++itor) *itor = std::toupper(*itor);
    m_bary_time = ("SOLARSYSTEM" == time_ref);

    // Get RA_NOM and DEC_NOM header keywords, if times in this table are barycentered.
    if (m_bary_time) {
      double ra_file;
      double dec_file;
      try {
        header["RA_NOM"].get(ra_file);
        header["DEC_NOM"].get(dec_file);
      } catch (const std::exception &) {
        throw std::runtime_error("Could not find RA_NOM or DEC_NOM header keyword in a barycentered event file.");
      }
      m_ra_nom = ra_file;
      m_dec_nom = dec_file;
    }

    // Pre-compute three vector version of RA_NOM and DEC_NOM.
    m_vect_nom = computeThreeVector(m_ra_nom, m_dec_nom);

    // Pre-compute threshold in sky position comparison.
    m_max_vect_diff = 2. * std::sin(angular_tolerance / 2. / RADEG);
    m_max_vect_diff *= m_max_vect_diff;
  }

  EventTimeHandler::~EventTimeHandler() {
    delete m_table;
  }

  EventTimeHandler * IEventTimeHandlerFactory::createHandler(const std::string & file_name, const std::string & extension_name,
    const std::string & sc_file_name, const std::string & sc_extension_name, const double angular_tolerance) {
    // Get the factory container.
    cont_type factory_cont(getFactoryContainer());

    // Look for an EventTimeHandler that can handle given files.
    EventTimeHandler * handler(0);
    for (cont_type::iterator itor = factory_cont.begin(); itor != factory_cont.end(); ++itor) {
      handler = (*itor)->createInstance(file_name, extension_name, sc_file_name, sc_extension_name, angular_tolerance);
      if (handler) break;
    }

    // Return the handler if found, or zero (0) if not.
    if (handler) {
      return handler;
    } else {
      throw std::runtime_error("Unsupported file(s) [filename=\"" + file_name + "\", extension=\"" + extension_name +
        "\", sc_file=\"" + sc_file_name + "\", sc_extension=\"" + sc_extension_name + "\"]");
    }
  }

  EventTimeHandler * EventTimeHandler::createInstance(const std::string & /*file_name*/, const std::string & /*extension_name*/,
    const std::string & /*sc_file_name*/, const std::string & /*sc_extension_name*/, const double /*angular_tolerance*/) {
    return 0;
  }

  AbsoluteTime EventTimeHandler::readHeader(const std::string & keyword_name) {
    return readTime(m_table->getHeader(), keyword_name, false, 0., 0.);
  }
  
  AbsoluteTime EventTimeHandler::readHeader(const std::string & keyword_name, const double ra, const double dec) {
    if (m_bary_time) {
      // Check RA & Dec in argument list match the table header, if already barycentered.
      checkSkyPosition(ra, dec);

      // Read barycentric time and return it.
      return readTime(m_table->getHeader(), keyword_name, false, ra, dec);

    } else {
      // Delegate computation of barycenteric time and return the result.
      return readTime(m_table->getHeader(), keyword_name, true, ra, dec);
    }
  }

  AbsoluteTime EventTimeHandler::readColumn(const std::string & column_name) {
    return readTime(*m_record_itor, column_name, false, 0., 0.);
  }
  
  AbsoluteTime EventTimeHandler::readColumn(const std::string & column_name, const double ra, const double dec) {
    if (m_bary_time) {
      // Check RA & Dec in argument list match the table header, if already barycentered.
      checkSkyPosition(ra, dec);

      // Read barycentric time and return it.
      return readTime(*m_record_itor, column_name, false, ra, dec);

    } else {
      // Delegate computation of barycenteric time and return the result.
      return readTime(*m_record_itor, column_name, true, ra, dec);
    }
  }

  void EventTimeHandler::setFirstRecord() {
    m_record_itor = m_table->begin();
  }

  void EventTimeHandler::setNextRecord() {
    if (m_record_itor != m_table->end()) ++m_record_itor;
  }

  void EventTimeHandler::setLastRecord() {
    m_record_itor = m_table->end();
    if (m_table->begin() != m_table->end()) --m_record_itor;
  }

  bool EventTimeHandler::isEndOfTable() const {
    return (m_record_itor == m_table->end());
  }

  tip::Table & EventTimeHandler::getTable() const {
    return *m_table;
  }

  tip::Header & EventTimeHandler::getHeader() const {
    return m_table->getHeader();
  }

  tip::TableRecord & EventTimeHandler::getCurrentRecord() const {
    return *m_record_itor;
  }

  void EventTimeHandler::computeBaryTime(const double ra, const double dec, const std::vector<double> & sc_position,
    AbsoluteTime & abs_time) const {
    m_computer.computeBaryTime(ra, dec, sc_position, abs_time);
  }

  double EventTimeHandler::computeInnerProduct(const std::vector<double> & vect_x, const std::vector<double> & vect_y) const {
    return vect_x[0]*vect_y[0] + vect_x[1]*vect_y[1] + vect_x[2]*vect_y[2];
  }

  std::vector<double> EventTimeHandler::computeThreeVector(const double ra, const double dec) const {
    std::vector<double> vect(3);

    vect[0] = std::cos(ra/RADEG) * std::cos(dec/RADEG);
    vect[1] = std::sin(ra/RADEG) * std::cos(dec/RADEG);
    vect[2] = std::sin(dec/RADEG);

    return vect;
  }

  void EventTimeHandler::checkSkyPosition(const double ra, const double dec) const {
    std::vector<double> source = computeThreeVector(ra, dec);
    std::vector<double> diff(3);
    diff[0] = source[0] - m_vect_nom[0];
    diff[1] = source[1] - m_vect_nom[1];
    diff[2] = source[2] - m_vect_nom[2];

    if (m_max_vect_diff < computeInnerProduct(diff, diff)) {
      std::ostringstream os;
      os << "Sky position for barycentric corrections (RA=" << ra << ", Dec=" << dec << 
        ") does not match RA_NOM (" << m_ra_nom << ") and DEC_NOM (" << m_dec_nom << ") in Event file.";
      throw std::runtime_error(os.str());
    }
  }
}
