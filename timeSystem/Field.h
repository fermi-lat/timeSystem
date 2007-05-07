/** \file Field.h
    \brief Declaration of Field class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_Field_h
#define timeSystem_Field_h

#include <string>

namespace timeSystem {

  class IField {
    public:
      virtual ~IField() {}

      virtual void get(double & value) const = 0;
      virtual void get(long & value) const = 0;

      virtual void set(const double & value) = 0;
      virtual void set(const long & value) = 0;

      virtual std::string getName() const = 0;
  };

  template <typename NumericType>
  class Field : public IField {
    public:
      explicit Field(const std::string & name, const NumericType & value): m_name(name), m_value(value) {}

      virtual void get(double & value) const { value = double(m_value); }
      virtual void get(long & value) const { value = long(m_value); }

      virtual void set(const double & value) { m_value = NumericType(value); }
      virtual void set(const long & value) { m_value = NumericType(value); }

      virtual std::string getName() const { return m_name; }

    private:
      std::string m_name;
      NumericType m_value;
  };

}

#endif
