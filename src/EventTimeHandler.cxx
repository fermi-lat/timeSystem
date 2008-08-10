/** \file EventTimeHandler.cxx
    \brief Implementation of EventTimeHandler class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/EventTimeHandler.h"

#include "timeSystem/MjdFormat.h"
#include "timeSystem/TimeFormat.h"

#include "tip/IFileSvc.h"

#include <stdexcept>

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

  EventTimeHandler * IEventTimeHandlerFactory::createHandler(const std::string & file_name, const std::string & extension_name,
    bool read_only) {
    // Get the factory container.
    cont_type factory_cont(getFactoryContainer());

    // Look for an EventTimeHandler that can handle given files.
    EventTimeHandler * handler(0);
    for (cont_type::iterator itor = factory_cont.begin(); itor != factory_cont.end(); ++itor) {
      handler = (*itor)->createInstance(file_name, extension_name, read_only);
      if (handler) break;
    }

    // Return the handler if found, or throw an exception.
    if (handler) {
      return handler;
    } else {
      throw std::runtime_error("Unsupported event file \"" + file_name + "[EXTNAME=" + extension_name + "]\"");
    }
  }

  EventTimeHandler::EventTimeHandler(const std::string & file_name, const std::string & extension_name, bool read_only): m_table(0) {
    // Get the table.
    // Note: for convenience, read-only and read-write tables are stored as non-const tip::Table pointers in the container.
    if (read_only) {
      const tip::Table * const_table = tip::IFileSvc::instance().readTable(file_name, extension_name);
      m_table = const_cast<tip::Table *>(const_table);
    } else {
      m_table = tip::IFileSvc::instance().editTable(file_name, extension_name);
    }

    // Set to the first record in the table.
    setFirstRecord();
  }

  EventTimeHandler::~EventTimeHandler() {
    delete m_table;
  }

  EventTimeHandler * EventTimeHandler::createInstance(const std::string & /*file_name*/, const std::string & /*extension_name*/,
    bool /*read_only*/) {
    return 0;
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

  Mjd EventTimeHandler::readMjdRef(const tip::Header & header) const {
    Mjd mjd_ref(0, 0.);
    bool found_mjd_ref = false;

    // Look for MJDREFI and MJDREFF keywords first.
    if (!found_mjd_ref) {
      try {
        header["MJDREFI"].get(mjd_ref.m_int);
        header["MJDREFF"].get(mjd_ref.m_frac);
        found_mjd_ref = true;
      } catch (const std::exception &) {}
    }

    // Look for MJDREF keyword next.
    if (!found_mjd_ref) {
      // Get the keyword value as a string to preserve its precision.
      std::string mjd_ref_string;
      try {
        header["MJDREF"].get(mjd_ref_string);
        found_mjd_ref = true;
      } catch (const std::exception &) {}

      // Parse the keyword value when found.
      if (found_mjd_ref) {
        const TimeFormat<Mjd> & mjd_format(TimeFormatFactory<Mjd>::getFormat());
        mjd_ref = mjd_format.parse(mjd_ref_string);
      }
    }

    // Throw an exception if none of above succeeds.
    if (!found_mjd_ref) {
      throw std::runtime_error("EventTimeHandler::readMjdRef could not find MJDREFI/MJDREFF or MJDREF.");
    }

    return mjd_ref;
  }
}
