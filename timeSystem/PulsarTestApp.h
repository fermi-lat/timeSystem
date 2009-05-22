/** \file PulsarTestApp.h
    \brief Declaration of base class for unit test application for pulsar tool packages.
    \author Masaharu Hirayama, GSSC
            James Peachey, HEASARC/GSSC
*/
#ifndef pulsarDb_PulsarTestApp_h
#define pulsarDb_PulsarTestApp_h

#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <typeinfo>

#include "st_app/StApp.h"

#include "st_stream/StreamFormatter.h"

namespace tip {
  class KeyRecord;
  class TableCell;
}

namespace timeSystem {

  /** \brief Base class for unit-test classes of pulsar tool packages.
  */
  class PulsarTestApp : public st_app::StApp {
    public:
      /** \brief Constructor.
          \param package_name Name of pulsar tool package to be tested by this class.
      */
      PulsarTestApp(const std::string & package_name);

      /// \brief Virtual destructor.
      virtual ~PulsarTestApp() throw();

      /** \brief Main method to run a unit test. This method calls runTest method after initializing this class,
                 then throws an exception if and only if at least one test failed.
      */
      virtual void run();

      /// \brief Main method to run a unit test.
      virtual void runTest() = 0;

      /** \brief Returns a full-path name of a file under the "data/" directory of this pulsar tool package.
          \param base_name Name of the file without a leading path name.
      */
      std::string prependDataPath(const std::string & base_name) const;

      /** \brief Returns a full-path name of a file under the "data/outref/" directory of this pulsar tool package.
          \param base_name Name of the file without a leading path name.
      */
      std::string prependOutrefPath(const std::string & base_name) const;

      /// \brief Set a method name to be used in a prefix of an error message.
      void setMethod(const std::string & method_name);

      /// \brief Get a method name to be used in a prefix of an error message.
      std::string getMethod() const;

      /** \brief Set precision to be used for test outputs. Returns the previous value of precision.
          \param precision Precision to set.
      */
      std::streamsize setPrecision(std::streamsize precision);

      /// \brief Returns an output stream to which error messages should be shifted.
      std::ostream & err();

    private:
      bool m_failed;
      std::string m_method_name;
      std::string m_data_dir;
      std::string m_outref_dir;
  };

  /** \brief Base class for classes to test an application in a pulsar tool package.
  */
  class PulsarApplicationTester {
    public:
      /** \brief Constructor.
          \param app_name Name of application to test.
          \param test_app Unit test appliction of pulsar tool package, under which this application tester is to run.
      */
      PulsarApplicationTester(const std::string & app_name, PulsarTestApp & test_app);

      /** \brief Virtual destructor.
      */
      virtual ~PulsarApplicationTester() throw();

      /// \brief Returns an application object to be tested.
      virtual st_app::StApp * createApplication() const = 0;

      /// \brief Get the application name to be tested.
      std::string getName() const;

      /** \brief Returns an output stream to which error messages should be shifted.
      */
      std::ostream & err();

      /** \brief Compare an output FITS file with its reference file in data/outref/ directory.
          \param out_file Name of an output FITS file to be compared with its reference.
          \param ref_file Name of a reference file to check a given output FITS file against.
      */
      void checkOutputFits(const std::string & out_file, const std::string & ref_file);

      /** \brief Compare an output text file with a given reference file.
          \param out_file Name of an output text file to be compared with a given reference.
          \param ref_file Name of a reference file to check a given output text file against.
      */
      void checkOutputText(const std::string & out_file, const std::string & ref_file);

      /** \brief Write text representation of a standard exception to a given output stream.
          \param os Output stream to which text representation of a standard exception is to be written.
          \param exception_object Standard exception object, whose text representation is to be written.
      */
      template <typename StreamType>
      StreamType & writeException(StreamType & os, const std::exception & exception_object) const;

      /** \brief Run an application, capture text output (if any), compare the text output and an output FITS file (if any)
                 with their reference files in data/outref/ directory.
          \param par_group Parameters to give to the application to test.
          \param log_file Log file name. An empty string disables logging.
          \param log_file_ref Name of a reference file to check a log file against. If an empty string is given,
                 the method uses a reference file in data/outref that has the same name as log_file.
          \param out_file Output FITS file name. An empty string disables comparison of the output FITS file.
          \param out_file_ref Name of a reference file to check an output FITS file against. If an empty string is given,
                 the method uses a reference file in data/outref that has the same name as out_file.
          \param ignore_exception Set true if an application is expected to throw an exception in this test.
      */
      void test(const st_app::AppParGroup & par_group, const std::string & log_file, const std::string & log_file_ref,
        const std::string & out_file, const std::string & out_file_ref, bool ignore_exception = false);

    protected:
      /** \brief Return a logical true if the two character strings are determined equivalent to each other,
                 and a logical false otherwise. This method compares a character string with a reference string,
                 with a tolerance for numerical expressions in the character strings. For example, string
                 "abc 0.0001 de" is considered equivalent to "abc 1e-4 de" by this method. The two floating-point
                 numbers are considered equivalent unless the difference between the two exceeds:
                   tolerance_abs + tolerance_rel * reference
                 where reference is the absolute value of the floating-point number taken from string_reference.
          \param string_value Character string to be checked against a reference string.
          \param string_reference Reference string for a character string of interest to be checked against.
          \param tolerance_abs Absolute tolerance in comparison of floating-point numbers.
          \param tolerance_rel Relative tolerance in comparison of floating-point numbers.
      */
      bool equivalent(const std::string & string_value, const std::string & string_reference, double tolerance_abs = 0.,
        double tolerance_rel = std::numeric_limits<double>::epsilon() * 1000.) const;

      /** \brief Return a logical true if the given header keyword is determined correct, and a logical false otherwise.
          \param keyword_name Name of the header keyword to be verified.
          \param out_keyword Header keyword taken from the output file to be verified.
          \param ref_keyword Header keyword taken from the reference file which out_keyword is checked against.
          \param error_stream Output stream for this method to put an error messages when verification fails.
      */
      virtual bool verify(const std::string & keyword_name, const tip::KeyRecord & out_keyword,
        const tip::KeyRecord & ref_keyword, std::ostream & error_stream) const;

      /** \brief Return a logical true if the given table cell is considered correct, and a logical false otherwise.
          \param column_name Name of the FITS column that the given table cell belongs to.
          \param out_cell Table cell taken from the output file to be verified.
          \param ref_cell Table cell taken from the reference file which out_cell is checked against.
          \param error_stream Output stream for this method to put an error message when verification fails.
      */
      virtual bool verify(const std::string & column_name, const tip::TableCell & out_cell, const tip::TableCell & ref_cell,
        std::ostream & error_stream) const;

      /** \brief Return a logical true if the given character string is considered correct, and a logical false otherwise.
          \param out_string Character string taken from the output file to be verified.
          \param ref_string Character string taken from the reference file which out_string is checked against.
          \param error_stream Output stream for this method to put an error message when verification fails.
      */
      virtual bool verify(const std::string & out_string, const std::string & ref_string, std::ostream & error_stream) const;

    private:
      std::string m_app_name;
      PulsarTestApp * m_test_app;
  };

  template <typename StreamType>
  StreamType & PulsarApplicationTester::writeException(StreamType & os, const std::exception & exception_object) const {
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
