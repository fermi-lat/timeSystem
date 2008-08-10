/** \file EventTimeHandler.h
    \brief Declaration of EventTimeHandler class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_EventTimeHandler_h
#define timeSystem_EventTimeHandler_h

#include "tip/Table.h"

#include <string>
#include <list>

namespace tip {
  class Header;
}

namespace timeSystem {

  class AbsoluteTime;
  class BaryTimeComputer;
  class EventTimeHandler;
  class Mjd;

  /** \class IEventTimeHandlerFactory
      \brief Abstract base class for EventTimeHandlerFactory.
  */
  class IEventTimeHandlerFactory {
    public:
      typedef std::list<IEventTimeHandlerFactory *> cont_type;

      IEventTimeHandlerFactory();

      virtual ~IEventTimeHandlerFactory();

      void registerHandler();

      void deregisterHandler();

      static EventTimeHandler * createHandler(const std::string & file_name, const std::string & extension_name, bool read_only = true);

    private:
      static cont_type & getFactoryContainer();

      virtual EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name,
        bool read_only = true) const = 0;
  };

  /** \class IEventTimeHandlerFactory
      \brief Class which registers and creates an EventTimeHandler sub-class given as a template argument.
  */
  template <typename HandlerType>
  class EventTimeHandlerFactory: public IEventTimeHandlerFactory {
    private:
      virtual EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name,
        bool read_only = true) const {
        return HandlerType::createInstance(file_name, extension_name, read_only);
      }
  };

  /** \class EventTimeHandler
      \brief Class which reads out event times from an event file, creates AbsoluteTime objects for event times,
             and performs barycentric correction on event times, typically recorded at a space craft.
  */
  class EventTimeHandler {
    public:
      virtual ~EventTimeHandler();

      static EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name, bool read_only = true);

      virtual void initTimeCorrection(const std::string & sc_file_name, const std::string & sc_extension_name, 
        const std::string & solar_eph, bool match_solar_eph, double angular_tolerance) = 0;

      virtual void setSourcePosition(double ra, double dec) = 0;

      virtual AbsoluteTime readTime(const std::string & field_name, bool from_header = false) const = 0;

      virtual AbsoluteTime getBaryTime(const std::string & field_name, bool from_header = false) const = 0;

      virtual AbsoluteTime parseTimeString(const std::string & time_string, const std::string & time_system = "FILE") const = 0;

      void setFirstRecord();

      void setNextRecord();

      void setLastRecord();

      bool isEndOfTable() const;

      tip::Table & getTable() const;

      tip::Header & getHeader() const;

      tip::TableRecord & getCurrentRecord() const;

    protected:
      EventTimeHandler(const std::string & file_name, const std::string & extension_name, bool read_only = true);

      Mjd readMjdRef(const tip::Header & header) const;

    private:
      tip::Table * m_table;
      tip::Table::Iterator m_record_itor;
  };
}

#endif
