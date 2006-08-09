/** \file GlastTimeRep.h
    \brief Declaration of GlastTimeRep class.
    \authors Masaharu Hirayama, GSSC
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_GlastTimeRep_h
#define timeSystem_GlastTimeRep_h

#include "timeSystem/TimeRep.h"

namespace timeSystem {

  class GlastMetRep : public MetRep {
    public:
      GlastMetRep(const std::string & system_name, double met): MetRep(system_name, 51910, 7.428703703703703e-4, met) {}
  };

}

#endif
