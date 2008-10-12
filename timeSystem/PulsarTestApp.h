/** \file PulsarTestApp.h
    \brief Declaration of base class for unit test application for pulsar tool packages.
    \author Masaharu Hirayama, GSSC
            James Peachey, HEASARC/GSSC
*/
#ifndef pulsarDb_PulsarTestApp_h
#define pulsarDb_PulsarTestApp_h

#include <iostream>
#include <string>

#include "st_app/StApp.h"

#include "st_stream/StreamFormatter.h"

namespace timeSystem {

  class PulsarTestApp : public st_app::StApp {
    public:
      PulsarTestApp(const std::string & package_name);
      virtual ~PulsarTestApp() throw();
      virtual void run() = 0;

      std::string getDataPath() const;

      void setMethod(const std::string & method_name);

      std::string getMethod() const;

      std::streamsize setPrecision(std::streamsize precision);

      std::ostream & err();

      void reportStatus() const;

    protected:
      /// Helper method to compare an output FITS file with its reference file in data/outref/ directory.
      void checkOutputFits(const std::string & file_name, bool compare_comment = false);

    private:
      bool m_failed;
      std::string m_method_name;
      std::string m_data_dir;
      std::string m_outref_dir;
  };

}

#endif
