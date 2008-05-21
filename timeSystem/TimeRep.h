/** \file TimeRep.h
    \brief Declaration of TimeRep and related classes.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_TimeRep_h
#define timeSystem_TimeRep_h

#include "timeSystem/Duration.h"
#include "timeSystem/Field.h"

#include <map>
#include <string>

namespace tip {
  class Header;
}

namespace timeSystem {

  class AbsoluteTime;
  class TimeSystem;

  class MjdRefDatabase {
    public:
      IntFracPair operator ()(const tip::Header & header) const;
  };

  /** \class TimeRep
      \brief
  */
  class TimeRep {
    public:
      virtual ~TimeRep();

      virtual TimeRep & operator =(const AbsoluteTime & abs_time);

      virtual void get(std::string & system_name, Duration & origin, Duration & elapsed) const = 0;

      virtual void get(const std::string & field_name, double & value) const {
        cont_type::const_iterator itor = m_field_cont.find(field_name);
        // TODO Put in subclass-specific name instead of TimeRep.
        if (m_field_cont.end() == itor) throw std::runtime_error("Field " + field_name + " is not valid for TimeRep");
        itor->second->get(value);
      }

      virtual void get(const std::string & field_name, long & value) const {
        cont_type::const_iterator itor = m_field_cont.find(field_name);
        // TODO Put in subclass-specific name instead of TimeRep.
        if (m_field_cont.end() == itor) throw std::runtime_error("Field " + field_name + " is not valid for TimeRep");
        itor->second->get(value);
      }

      virtual void set(const std::string & system_name, const Duration & origin, const Duration & elapsed) = 0;

      virtual void set(const std::string & field_name, const double & value) {
        cont_type::iterator itor = m_field_cont.find(field_name);
        // TODO Put in subclass-specific name instead of TimeRep.
        if (m_field_cont.end() == itor) throw std::runtime_error("Field " + field_name + " is not valid for TimeRep");
        itor->second->set(value);
      }

      virtual void set(const std::string & field_name, const long & value) {
        cont_type::iterator itor = m_field_cont.find(field_name);
        // TODO Put in subclass-specific name instead of TimeRep.
        if (m_field_cont.end() == itor) throw std::runtime_error("Field " + field_name + " is not valid for TimeRep");
        itor->second->set(value);
      }

      virtual std::string getString() const = 0;

      virtual void assign(const std::string & value) = 0;

      template <typename StreamType>
      void write(StreamType & os) const;

    protected:
      typedef std::map<std::string, IField *> cont_type;

      // Warning this leaks if you call it more than once.
      template<typename ValueType>
      void addField(const std::string & field_name, const ValueType & field_value) {
        // Destroy previous instance of this field if it exists already.
        cont_type::iterator itor = m_field_cont.find(field_name);
        if (m_field_cont.end() != itor ) delete itor->second;
        // Create new field of the given type with the given value.
        m_field_cont[field_name] = new Field<ValueType>(field_name, field_value);
      }

      cont_type m_field_cont;
  };

  template <typename StreamType>
  inline void TimeRep::write(StreamType & os) const { os << getString(); }

  std::ostream & operator <<(std::ostream & os, const TimeRep & time_rep);

  st_stream::OStream & operator <<(st_stream::OStream & os, const TimeRep & time_rep);

  /** \class MetRep
      \brief
  */
  class MetRep : public TimeRep {
    public:
      MetRep(const std::string & system_name, long mjd_ref_int, double mjd_ref_frac, double met);

      MetRep(const std::string & system_name, const IntFracPair & mjd_ref, double met);

      virtual MetRep & operator =(const AbsoluteTime & abs_time);

      virtual void get(std::string & system_name, Duration & origin, Duration & elapsed) const;

      virtual void get(const std::string & field_name, double & value) const;

      virtual void get(const std::string & field_name, long & value) const;

      virtual void set(const std::string & system_name, const Duration & origin, const Duration & elapsed);

      virtual void set(const std::string & field_name, const double & value);

      virtual void set(const std::string & field_name, const long & value);

      virtual std::string getString() const;

      virtual void assign(const std::string & value);

      // TODO Remove this method after all usages are converted to get(string, double) etc.
      double getValue() const;

      // TODO Remove this method after all usages are converted to set(string, double) etc.
      void setValue(double met);

    private:
      const TimeSystem * m_system;
      Duration m_mjd_ref;
      double m_met;
  };

#if 0
  /** \class MjdRep
      \brief
  */
  class MjdRep : public TimeRep {
    public:
      MjdRep(const std::string & system_name, long mjd_int, double mjd_frac);

      virtual MjdRep & operator =(const AbsoluteTime & abs_time);

      virtual void get(std::string & system_name, Duration & origin, Duration & elapsed) const;

      virtual void set(const std::string & system_name, const Duration & origin, const Duration & elapsed);

      virtual std::string getString() const;

      virtual void assign(const std::string & value);

      // TODO Remove this method after all usages are converted to get(string, double) etc.
      IntFracPair getValue() const;

      // TODO Remove this method after all usages are converted to set(string, double) etc.
      void setValue(long mjd_int, double mjd_frac);

    private:
      const TimeSystem * m_system;
  };
#endif

  /** \class TimeFormat
      \brief Base class to represent time format, such as MJD and Calender Day of ISO 8601.
  */
  class TimeFormat {
    public:
      virtual ~TimeFormat();

      static const TimeFormat & getFormat(const std::string & format_name);

      virtual std::string format(const moment_type & value, std::streamsize precision = std::numeric_limits<double>::digits10) const = 0;

      virtual moment_type parse(const std::string & value) const = 0;

    protected:
      typedef std::map<std::string, TimeFormat *> container_type;

      static container_type & getContainer();

      TimeFormat(const std::string & format_name);

      std::string m_format_name;
  };

  /** \class MjdFormat
      \brief Class to represent MJD format of time representation.
  */
  class MjdFormat : public TimeFormat {
    public:
      static const MjdFormat & getMjdFormat();

      virtual std::string format(const moment_type & value, std::streamsize precision = std::numeric_limits<double>::digits10) const;

      virtual moment_type parse(const std::string & value) const;

      void convert(const moment_type & moment, long & mjd_int, double & mjd_frac) const;

      void convert(const moment_type & moment, double & mjd) const;

      void convert(long mjd_int, double mjd_frac, moment_type & moment) const;

      void convert(double mjd, moment_type & moment) const;

    private:
      MjdFormat();
  };

}

#endif
