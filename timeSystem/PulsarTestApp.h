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

  /** \brief Base class for unit-test classes of pulsar tool packages.
  */
  class PulsarTestApp : public st_app::StApp {
    public:
      /** \brief Constructor.
          \param package_name Name of pulsar tool package to be tested by this class.
      */
      PulsarTestApp(const std::string & package_name);

      /** \brief Virtual destructor.
      */
      virtual ~PulsarTestApp() throw();

      /** \brief Main method to run a unit test.
      */
      virtual void run() = 0;

      /** \brief Returns a path name of the "data/" directory of this pulsar tool package.
      */
      std::string getDataPath() const;

      /** \brief Set a method name to be used in a prefix of an error message.
      */
      void setMethod(const std::string & method_name);

      /** \brief Get a method name to be used in a prefix of an error message.
      */
      std::string getMethod() const;

      /** \brief Set precision to be used for test outputs. Returns the previous value of precision.
          \param precision Precision to set.
      */
      std::streamsize setPrecision(std::streamsize precision);

      /** \brief Returns an output stream to which error messages should be shifted.
      */
      std::ostream & err();

      /** \brief Reports overall test status. If at least one test failed, throws an exception.
                 Otherwise, does nothing.
      */
      void reportStatus() const;

    protected:
      /** \brief Helper method to compare an output FITS file with its reference file in data/outref/ directory.
          \param file_name Name of a FITS file to be compared with its reference.
          \param compare_comment Set true if COMMENT keyword values should be compared.
      */
      void checkOutputFits(const std::string & file_name, bool compare_comment = false);

      /** \brief Helper method to compare an output text file with its reference file in data/outref/ directory.
          \param file_name Name of a text file to be compared with its reference.
      */
      void checkOutputText(const std::string & file_name);

      /** \brief Helper method to run an application, capture text output, compare the log and an output FITS file
                 with their reference files in data/outref/ directory.
          \param application Application object to test. Set parameters before calling this method.
          \param log_file Log file name. An empty string disables logging.
          \param out_fits Output FITS file name. An empty string disables comparison of the output FITS file.
          \param ignore_exception Set true if an application is expected to throw an exception in this test.
      */
      void testApplication(st_app::StApp & application, const std::string & log_file, const std::string & out_fits,
        bool ignore_exception = false);

    private:
      bool m_failed;
      std::string m_method_name;
      std::string m_data_dir;
      std::string m_outref_dir;
  };

}

#endif
