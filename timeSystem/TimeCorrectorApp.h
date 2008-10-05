/** \file TimeCorrectorApp.h
    \brief Interface for TimeCorrectorApp class.
    \authors Masaharu Hirayama, GSSC,
             James Peachey, HEASARC/GSSC
*/
#ifndef timeSystem_TimeCorrectorApp_h
#define timeSystem_TimeCorrectorApp_h

#include <string>

#include "st_app/StApp.h"

namespace timeSystem {

  class TimeCorrectorApp : public st_app::StApp {
    public:
      TimeCorrectorApp();
      virtual ~TimeCorrectorApp() throw();
      virtual void run();

    private:
      std::string tmpFileName(const std::string & file_name) const;
  };

}
#endif
