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
  class TimeRep;

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

      static EventTimeHandler * createHandler(const std::string & file_name, const std::string & extension_name,
        const double angular_tolerance, const bool read_only = true);

    private:
      static cont_type & getFactoryContainer();

      virtual EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name,
        const double angular_tolerance, const bool read_only = true) const = 0;
  };

  /** \class IEventTimeHandlerFactory
      \brief Class which registers and creates an EventTimeHandler sub-class given as a template argument.
  */
  template <typename HandlerType>
  class EventTimeHandlerFactory: public IEventTimeHandlerFactory {
    private:
      virtual EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name,
        const double angular_tolerance, const bool read_only = true) const {
        return HandlerType::createInstance(file_name, extension_name, angular_tolerance, read_only);
      }
  };

  /** \class EventTimeHandler
      \brief Class which reads out event times from an event file, creates AbsoluteTime objects for event times,
             and performs barycentric correction on event times, typically recorded at a space craft.
  */
  // TODO: Rename EventTimeHandler to MetHandler? Yes!
  class EventTimeHandler {
    public:
      virtual ~EventTimeHandler();

      static EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name,
        const double angular_tolerance, const bool read_only = true);

      virtual void setSpacecraftFile(const std::string & sc_file_name, const std::string & sc_extension_name) = 0;

      // TODO: Consider adding the following methods to replace TimeRep objects for MET's.
      //virtual AbsoluteTime readString(const std::string & time_string, const std::string & time_system = "FILE") = 0;
      //virtual TimeRep * createTimeRep(const std::string & time_system = "FILE") const = 0;

      AbsoluteTime readHeader(const std::string & keyword_name);

      AbsoluteTime readHeader(const std::string & keyword_name, const double ra, const double dec);

      AbsoluteTime readColumn(const std::string & column_name);

      AbsoluteTime readColumn(const std::string & column_name, const double ra, const double dec);

      void setFirstRecord();

      void setNextRecord();

      void setLastRecord();

      bool isEndOfTable() const;

      tip::Table & getTable() const;

      tip::Header & getHeader() const;

      tip::TableRecord & getCurrentRecord() const;

      void checkSkyPosition(const double ra, const double dec) const;

      void checkSolarEph(const std::string & solar_eph) const;

    protected:
      EventTimeHandler(const std::string & file_name, const std::string & extension_name, const double angular_tolerance,
        const bool read_only = true);

      virtual AbsoluteTime readTime(const tip::Header & header, const std::string & keyword_name, const bool request_bary_time,
        const double ra, const double dec) = 0;

      virtual AbsoluteTime readTime(const tip::TableRecord & record, const std::string & column_name, const bool request_bary_time,
        const double ra, const double dec) = 0;

      void computeBaryTime(const double ra, const double dec, const std::vector<double> & sc_position, AbsoluteTime & abs_time)
        const;

    private:
      // Variables for event table handling.
      std::string m_file_name;
      std::string m_ext_name;
      tip::Table * m_table;
      tip::Table::Iterator m_record_itor;
      bool m_bary_time;
      double m_ra_nom;
      double m_dec_nom;
      std::vector<double> m_vect_nom; // Three vector representation of m_ra_nom and m_dec_nom.
      double m_max_vect_diff;
      std::string m_pl_ephem;

      // Variables for barycentering.
      BaryTimeComputer & m_computer;

      // TODO: Should those (computeInnerProduct/ThreeVector) be replaced by the ones in BaryTimeComputer?
      double computeInnerProduct(const std::vector<double> & vect_x, const std::vector<double> & vect_y) const;

      std::vector<double> computeThreeVector(const double ra, const double dec) const;
  };
}

#endif
