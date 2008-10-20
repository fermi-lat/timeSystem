/** \file EventTimeHandler.cxx
    \brief Implementation of EventTimeHandler class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/EventTimeHandler.h"

#include "timeSystem/MjdFormat.h"
#include "timeSystem/TimeFormat.h"

#include "tip/IFileSvc.h"
#include "tip/TipException.h"

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

    // Confirm the container is not empty.
    if (factory_cont.empty()) {
      throw std::runtime_error("IEventTimeHandlerFactory::createHandler: List of event-time handling classes is empty.");
    }

    // Check existence of the file.
    if (!tip::IFileSvc::instance().fileExists(file_name)) {
      throw std::runtime_error("File not found: " + file_name);
    }

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
      throw std::runtime_error("Unsupported timing FITS extension \"" + file_name + "[EXTNAME=" + extension_name + "]\"");
    }
  }

  EventTimeHandler::EventTimeHandler(const std::string & file_name, const std::string & extension_name, bool read_only):
    m_extension(0), m_table(0) {
    // Try to open it as a tip::Table first.
    if (read_only) {
      try {
        // Note: for convenience, read-only and read-write tables are stored as non-const tip::Table pointers in the container.
        m_table = const_cast<tip::Table *>(tip::IFileSvc::instance().readTable(file_name, extension_name));
      } catch (const tip::TipException &) {
        m_table = 0;
      }

    } else {
      try {
        m_table = tip::IFileSvc::instance().editTable(file_name, extension_name);
      } catch (const tip::TipException &) {
        m_table = 0;
      } 
    }

    // Set a pointer to a tip::Extension object.
    if (m_table) {
      m_extension = m_table;
    } else if (read_only) {
      m_extension = const_cast<tip::Extension *>(tip::IFileSvc::instance().readExtension(file_name, extension_name));
    } else {
      m_extension = tip::IFileSvc::instance().editExtension(file_name, extension_name);
    }

    // Set to the first record in the table.
    setFirstRecord();
  }

  EventTimeHandler::~EventTimeHandler() {
    delete m_extension;
    // Note: No need to delete m_table because it points to the same object as m_extension does.
  }

  EventTimeHandler * EventTimeHandler::createInstance(const std::string & /*file_name*/, const std::string & /*extension_name*/,
    bool /*read_only*/) {
    return 0;
  }

  void EventTimeHandler::setFirstRecord() {
    if (m_table) m_record_itor = m_table->begin();
  }

  void EventTimeHandler::setNextRecord() {
    if (!isEndOfTable()) ++m_record_itor;
  }

  void EventTimeHandler::setLastRecord() {
    if (m_table) {
      m_record_itor = m_table->end();
      if (m_table->begin() != m_table->end()) --m_record_itor;
    }
  }

  bool EventTimeHandler::isEndOfTable() const {
    return (m_table == 0 || m_record_itor == m_table->end());
  }

  tip::Table & EventTimeHandler::getTable() const {
    if (m_table) return *m_table;
    else throw std::runtime_error("EventTimeHandler::getTable was called for a FITS extension that contains no data table.");
  }

  tip::Header & EventTimeHandler::getHeader() const {
    return m_extension->getHeader();
  }

  tip::TableRecord & EventTimeHandler::getCurrentRecord() const {
    if (m_table) return *m_record_itor;
    else throw std::runtime_error("EventTimeHandler::getCurrentRecord was called for a FITS extension that contains no data table.");
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
