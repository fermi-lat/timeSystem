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

  /** \class TimeCorrectorApp
      \brief Main application class for photon arrival time corrections.
  */
  class TimeCorrectorApp : public st_app::StApp {
    public:
      /// \brief Construct a TimeCorrectorApp object.
      TimeCorrectorApp();

      /// \brief Destruct this TimeCorrectorApp object.
      virtual ~TimeCorrectorApp() throw();

      /// \brief Run the application.
      virtual void run();

    private:
      /** \brief Create a temporary file name.
          \param file_name Name of file based on which a temorary file name is created.
      */
      std::string tmpFileName(const std::string & file_name) const;
  };

}
#endif
