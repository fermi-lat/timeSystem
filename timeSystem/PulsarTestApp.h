/** \file PulsarTestApp.h
    \brief Declaration of base class for unit test application for pulsar tool packages.
    \author Masaharu Hirayama, GSSC
            James Peachey, HEASARC/GSSC
*/
#ifndef pulsarDb_PulsarTestApp_h
#define pulsarDb_PulsarTestApp_h

#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <typeinfo>

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

      /** \brief Main method to run a unit test. This method calls runTest method after initializing this class,
                 then throws an exception if and only if at least one test failed.
      */
      virtual void run();

      /** \brief Main method to run a unit test.
      */
      virtual void runTest() = 0;

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

    protected:
      /** \brief Write text representation of a standard exception to a given output stream.
          \param os Output stream to which text representation of a standard exception is to be written.
          \param exception_object Standard exception object, whose text representation is to be written.
      */
      template <typename StreamType>
      StreamType & writeException(StreamType & os, const std::exception & exception_object) const;

      /** \brief Helper method to compare a character string with a reference string, with a tolerance for
                 numerical expressions in the character strings. For example, string "abc 0.0001 de" is considered
                 equivalent to "abc 1e-4 de" by this method. The method returns false if the character string of interest
                 is determined equivalent to the reference string, and true otherwise.
          \param string_value Character string to be checked against a reference string.
          \param string_reference Reference string for a character string of interest to be checked against.
      */
      bool compareNumericString(const std::string & string_value, const std::string & string_reference) const;

      /** \brief Helper method to compare an output FITS file with its reference file in data/outref/ directory.
          \param out_file Name of an output FITS file to be compared with its reference.
          \param column_to_compare Container of column names used in comparison of output FITS files. If the name of a column
                 in a reference file is found in this container, the contents of the column will be compared. Otherwise,
                 the contents of the column are ignored in comparison. If the container is empty, all columns in a reference
                 file will be compared.
      */
      void checkOutputFits(const std::string & out_file, const std::set<std::string> & column_to_compare = std::set<std::string>());

      /** \brief Helper method to compare an output text file with its reference file in data/outref/ directory.
          \param out_file Name of an output text file to be compared with its reference in data/outref/ directory.
      */
      void checkOutputText(const std::string & out_file);

      /** \brief Helper method to compare an output text file with a given reference file.
          \param out_file Name of an output text file to be compared with a given reference.
          \param ref_file Name of a reference file to check a given output text file against.
      */
      void checkOutputText(const std::string & out_file, const std::string & ref_file);

      /** \brief Helper method to run an application, capture text output, compare the log and an output FITS file
                 with their reference files in data/outref/ directory.
          \param app_name Name of application to test.
          \param par_group Parameters to give to the application to test.
          \param log_file Log file name. An empty string disables logging.
          \param ref_file Name of a reference file to check a log file against. If an empty string is given,
                 the method uses a reference file in data/outref that has the same name as log_file.
          \param out_fits Output FITS file name. An empty string disables comparison of the output FITS file.
          \param column_to_compare Container of column names used in comparison of output FITS files. If the name of a column
                 in a reference file is found in this container, the contents of the column will be compared. Otherwise,
                 the contents of the column are ignored in comparison. If the container is empty, all columns in a reference
                 file will be compared.
          \param ignore_exception Set true if an application is expected to throw an exception in this test.
      */
      void testApplication(const std::string & app_name, const st_app::AppParGroup & par_group, const std::string & log_file,
        const std::string & ref_file, const std::string & out_fits, const std::set<std::string> & column_to_compare,
        bool ignore_exception = false);

      /** \brief Returns a named application object. Return 0 (zero) if no such application exists.
          \param app_name Name of application to create. The value of app_name parameter of testApplication method will be
                 passed to this method as this argument in order to create an application object. 
      */
      virtual st_app::StApp * createApplication(const std::string & app_name) const = 0;

    private:
      bool m_failed;
      std::string m_method_name;
      std::string m_data_dir;
      std::string m_outref_dir;
  };

  template <typename StreamType>
  StreamType & PulsarTestApp::writeException(StreamType & os, const std::exception & exception_object) const {
    // Report the type of the exception if possible, using typeid; typeid can throw so be careful:
    const char * type_name = "std::exception";
    try {
      type_name = typeid(exception_object).name();
    } catch (...) {
      // Ignore problems with typeid.
    }
    os << "Caught " << type_name << " at the top level: " << exception_object.what() << std::endl;

    // Return the stream.
    return os;
  }

}

#endif
