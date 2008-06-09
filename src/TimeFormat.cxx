/** \file TimeFormat.cxx
    \brief Implementation of TimeFormat and related classes.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#include "timeSystem/TimeFormat.h"

#include <cctype>
#include <stdexcept>

namespace timeSystem {

  TimeFormat::TimeFormat(const std::string & format_name) {
    std::string format_name_uc(format_name);
    for (std::string::iterator itor = format_name_uc.begin(); itor != format_name_uc.end(); ++itor) *itor = std::toupper(*itor);
    getContainer()[format_name_uc] = this;
  }

  TimeFormat::~TimeFormat() {
  }

  const TimeFormat & TimeFormat::getFormat(const std::string & format_name) {
    std::string format_name_uc(format_name);
    for (std::string::iterator itor = format_name_uc.begin(); itor != format_name_uc.end(); ++itor) *itor = std::toupper(*itor);
    container_type & container(getContainer());

    container_type::iterator cont_itor = container.find(format_name_uc);
    if (container.end() == cont_itor) throw std::runtime_error("TimeFormat::getFormat could not find time format " + format_name);
    return *cont_itor->second;
  }

  TimeFormat::container_type & TimeFormat::getContainer() {
    static container_type s_prototype;
    return s_prototype;
  }
}
