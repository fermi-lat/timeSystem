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

      /// \brief Construct an IEventTimeHandlerFactory object.
      IEventTimeHandlerFactory();

      /// \brief Denstruct this IEventTimeHandlerFactory object.
      virtual ~IEventTimeHandlerFactory();

      /// \brief Register this IEventTimeHandlerFactory object.
      void registerHandler();

      /// \brief Deregister this IEventTimeHandlerFactory object.
      void deregisterHandler();

      /** \brief Create an EventTimeHandler object that can appropriately handle a given FITS extension.
          \param file_name Name of FITS file to open.
          \param extension_name Name of FITS extension to open.
          \param read_only Set to true to open the file in a read-only mode. Set to false to open it in a read-write mode.
      */
      static EventTimeHandler * createHandler(const std::string & file_name, const std::string & extension_name, bool read_only = true);

    private:
      /// \brief Return a container of registered IEventTimeHandlerFactory objects.
      static cont_type & getFactoryContainer();

      /** \brief Create an instance of a concrete EventTimeHandler subclass, and return the pointer to the instance.
                 If failed to open a given file, this method should return 0 (null pointer).
          \param file_name Name of FITS file to open.
          \param extension_name Name of FITS extension to open.
          \param read_only Set to true to open the file in a read-only mode. Set to false to open it in a read-write mode.
      */
      virtual EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name,
        bool read_only = true) const = 0;
  };

  /** \class IEventTimeHandlerFactory
      \brief Class which registers and creates an EventTimeHandler sub-class given as a template argument.
  */
  template <typename HandlerType>
  class EventTimeHandlerFactory: public IEventTimeHandlerFactory {
    private:
      /** \brief Create an instance of a concrete EventTimeHandler subclass, and return the pointer to the instance.
                 If failed to open a given file, this method should return 0 (null pointer).
          \param file_name Name of FITS file to open.
          \param extension_name Name of FITS extension to open.
          \param read_only Set to true to open the file in a read-only mode. Set to false to open it in a read-write mode.
      */
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
      /// \brief Denstruct this EventTimeHandler object.
      virtual ~EventTimeHandler();

      /** \brief Fall-back method just in case a derived class does not implement createInstance method.
                 This method always returns 0 (null pointer).
          \param file_name Name of FITS file to open (not used).
          \param extension_name Name of FITS extension to open (not used).
          \param read_only Set to true to open the file in a read-only mode. Set to false to open it in a read-write mode (not used).
      */
      static EventTimeHandler * createInstance(const std::string & file_name, const std::string & extension_name, bool read_only = true);

      /** \brief Initialize arrival time corrections.
          \param sc_file_name Name of spacecraft file to be used for arrival time corrections.
          \param sc_extension_name Name of FITS table that contains spacecraft data in the above file.
          \param solar_eph Name of solar system ephemeris to use for arrival time corrections.
          \param match_solar_eph Set to true if the above solar system ephemeris must match the one written in the opened file.
          \param angular_tolerance Maximum angular difference in degrees allowed in comparison of sky position between
                 the one given to setSourcePosition method and the one in the opened file.
      */
      virtual void initTimeCorrection(const std::string & sc_file_name, const std::string & sc_extension_name, 
        const std::string & solar_eph, bool match_solar_eph, double angular_tolerance) = 0;

      /** \brief Set the source position for arrival time corrections.
          \param ra Right Ascension of the source position.
          \param dec Declination of the source position.
      */
      virtual void setSourcePosition(double ra, double dec) = 0;

      /** \brief Read a given field of the opened FITS header or the current record of the opened FITS table,
                 compute an absolute time that it represents, and return it.
          \param field_name Name of field from which a time is to be read.
          \param from_header Set to true to read a time from the header. Set to false to read a time from the current record
                 of the table.
      */
      virtual AbsoluteTime readTime(const std::string & field_name, bool from_header = false) const = 0;

      /** \brief Write a given absolute time to a specified field of the opened FITS header or the current record of
                 the opened FITS tale.
          \param field_name Name of field to which a given time is written.
          \param abs_time Absolute time to write to the above field.
          \param to_header Set to true to write the time to the header. Set to false to write it to the current record of the table.
      */
      virtual void writeTime(const std::string & field_name, const AbsoluteTime & abs_time, bool to_header = false) = 0;

      /** \brief Read a given field of the opened FITS header or the current record of the opened FITS table,
                 compute an absolute time that it represents, compute a geocentric time for it, and return it.
          \param field_name Name of field from which a time is to be read.
          \param from_header Set to true to read a time from the header. Set to false to read a time from the current record
                 of the table.
      */
      virtual AbsoluteTime getGeoTime(const std::string & field_name, bool from_header = false) const = 0;

      /** \brief Read a given field of the opened FITS header or the current record of the opened FITS table,
                 compute an absolute time that it represents, compute a barycentric time for it, and return it.
          \param field_name Name of field from which a time is to be read.
          \param from_header Set to true to read a time from the header. Set to false to read a time from the current record
                 of the table.
      */
      virtual AbsoluteTime getBaryTime(const std::string & field_name, bool from_header = false) const = 0;

      /** \brief Interpret a given character string as a time value in the same format as in the opened file,
                 compute an absolute time that it represents, and return it.
          \param time_string Character string to interpret as a time value.
          \param time_system Time system name to use to interpret the time value. This method uses TIMESYS header keyword value
                             if this argument is set to "FILE".
      */
      virtual AbsoluteTime parseTimeString(const std::string & time_string, const std::string & time_system = "FILE") const = 0;

      /// \brief Set the internal record iterator to point to the first record in the opened FITS file.
      void setFirstRecord();

      /// \brief Increment the internal record iterator to point to the next record in the opened FITS file.
      void setNextRecord();

      /// \brief Set the internal record iterator to point to the last record in the opened FITS file.
      void setLastRecord();

      /// \brief Return if the internal record iterator points to one past the last record in the opened FITS file.
      bool isEndOfTable() const;

      /// \brief Return the opend FITS table.
      tip::Table & getTable() const;

      /// \brief Return the opend FITS header.
      tip::Header & getHeader() const;

      /// \brief Return the current record of the opend FITS table.
      tip::TableRecord & getCurrentRecord() const;

    protected:
      /** \brief Construct an EventTimeHandler object.
          \param file_name Name of FITS file to open.
          \param extension_name Name of FITS extension to open.
          \param read_only Set to true to open the file in a read-only mode. Set to false to open it in a read-write mode.
      */
      EventTimeHandler(const std::string & file_name, const std::string & extension_name, bool read_only = true);

      /** \brief Read a pair of MJDREFI and MJDREFF keywords or a single MJDREF header keyword from the opened FITS header.
                 If none of the two is found, an exception is thrown.
          \param header FITS header in which the aimed header keywords are searched for.
      */
      Mjd readMjdRef(const tip::Header & header) const;

    private:
      tip::Extension * m_extension;
      tip::Table * m_table;
      tip::Table::Iterator m_record_itor;
  };
}

#endif
