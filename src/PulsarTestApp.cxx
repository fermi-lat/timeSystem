/** \file PulsarTestApp.cxx
    \brief Implementation of base class for unit test application for pulsar tool packages.
    \author Masaharu Hirayama, GSSC
            James Peachey, HEASARC/GSSC
*/
#include "timeSystem/PulsarTestApp.h"

#include <fstream>
#include <iostream>
#include <list>
#include <stdexcept>
#include <typeinfo>

#include "facilities/commonUtilities.h"

#include "hoops/hoops.h"
#include "hoops/hoops_exception.h"
#include "hoops/hoops_par.h"

#include "st_app/AppParGroup.h"

#include "st_stream/Stream.h"
#include "st_stream/st_stream.h"

#include "tip/Extension.h"
#include "tip/FileSummary.h"
#include "tip/Header.h"
#include "tip/IFileSvc.h"

namespace timeSystem {

  PulsarTestApp::PulsarTestApp(const std::string & package_name): m_failed(false), m_method_name(), m_data_dir(), m_outref_dir() {
    // Find data directory for this app.
    m_data_dir = facilities::commonUtilities::getDataPath(package_name);

    // Set the directory name for output reference files.
    m_outref_dir = facilities::commonUtilities::joinPath(m_data_dir, "outref");
  }

  PulsarTestApp::~PulsarTestApp() throw() {}

  void PulsarTestApp::run() {
    // Initialize the internal variables that need to be refreshed everytime this method is called.
    m_failed = false;
    m_method_name.clear();

    // Run the test.
    runTest();

    // Report overall test status.
    if (m_failed) throw std::runtime_error(getName() + ": unit test failed.");
  }

  std::string PulsarTestApp::getDataPath() const {
    return m_data_dir;
  }

  void PulsarTestApp::setMethod(const std::string & method_name) {
    m_method_name = method_name;
  }

  std::string PulsarTestApp::getMethod() const {
    return m_method_name;
  }

  std::streamsize PulsarTestApp::setPrecision(std::streamsize precision) {
    return std::cerr.precision(precision);
  }

  std::ostream & PulsarTestApp::err() {
    m_failed = true;
    return std::cerr << getName() << ": " << m_method_name << ": ";
  }

  void PulsarTestApp::checkOutputFits(const std::string & file_name, bool compare_comment) {
    // Set reference file name.
    const std::string out_file(file_name);
    const std::string ref_file = facilities::commonUtilities::joinPath(m_outref_dir, out_file);

    // Check file existence.
    if (!tip::IFileSvc::instance().fileExists(out_file)) {
      err() << "File to check does not exist: " << out_file << std::endl;
      return;
    }
    if (!tip::IFileSvc::instance().fileExists(ref_file)) {
      err() << "Reference file for " << out_file << " does not exist: " << ref_file << std::endl;
      return;
    }

    // Get fille summaries for FITS files to compare.
    tip::FileSummary out_summary;
    tip::IFileSvc::instance().getFileSummary(out_file, out_summary);
    tip::FileSummary ref_summary;
    tip::IFileSvc::instance().getFileSummary(ref_file, ref_summary);

    // Compare the number of extensions.
    tip::FileSummary::size_type out_size = out_summary.size();
    tip::FileSummary::size_type ref_size = ref_summary.size();
    if (out_size != ref_summary.size()) {
      err() << "File " << out_file << " has " << out_size << " HDU('s), not " << ref_size << " as in reference file " <<
        ref_file << std::endl;
    } else {
      int num_extension = ref_size;

      // Compare each extension.
      for (int ext_number = 0; ext_number < num_extension; ++ext_number) {
        // Open extensions by extension number.
        std::ostringstream os;
        os << ext_number;
        std::string ext_name = os.str();
        std::auto_ptr<tip::Extension> out_ext(tip::IFileSvc::instance().editExtension(out_file, ext_name));
        tip::Header & out_header(out_ext->getHeader());
        std::auto_ptr<tip::Extension> ref_ext(tip::IFileSvc::instance().editExtension(ref_file, ext_name));
        tip::Header & ref_header(ref_ext->getHeader());

        // Compare the sizes of header.
        tip::Header::KeySeq_t::size_type out_num_key = out_header.getNumKeywords();
        tip::Header::KeySeq_t::size_type ref_num_key = ref_header.getNumKeywords();
        if (out_num_key != ref_num_key) {
          err() << "HDU " << ext_name << " of file " << out_file << " has " << out_num_key <<
            " header keyword(s), not " << ref_num_key << " as in reference file " << ref_file << std::endl;

        } else {
          // Compare each header keyword.
          int card_number = 1;
          tip::Header::Iterator out_itor = out_header.begin();
          tip::Header::Iterator ref_itor = ref_header.begin();
          for (; out_itor != out_header.end() && ref_itor != ref_header.end(); ++out_itor, ++ref_itor, ++card_number) {
            // Compare keyword name.
            std::string out_name = out_itor->getName();
            std::string ref_name = ref_itor->getName();
            if (out_name != ref_name) {
              err() << "Card " << card_number << " of HDU " << ext_name << " in file " << out_file <<
                " is header keyword " << out_name << ", not " << ref_name << " as in reference file " << ref_file << std::endl;
            }

            // Compare keyword value, except for CHECKSUM, CREATOR, DATE, HISTORY, COMMENT, and a blank name.
            std::string out_value;
            std::string ref_value;
            if (!ref_name.empty() && "CHECKSUM" != ref_name && "CREATOR" != ref_name && "DATE" != ref_name && "HISTORY" != ref_name &&
                "COMMENT" != ref_name) {
              std::string out_value = out_itor->getValue();
              std::string ref_value = ref_itor->getValue();
              if (out_value != ref_value) {
                err() << "Header keyword " << out_name << " on card " << card_number << " of HDU " << ext_name <<
                  " in file " << out_file << " has value " << out_value << ", not " << ref_value << " as in reference file " <<
                  ref_file << std::endl;
              }
            }

            // Compare COMMENT if requested.
            if (compare_comment && "COMMENT" == ref_name) {
              std::string out_value = out_itor->getComment();
              std::string ref_value = ref_itor->getComment();
              if (out_value != ref_value) {
                err() << "COMMENT keyword on card " << card_number << " of HDU " << ext_name << " in file " << out_file <<
                  " has comment \"" << out_value << "\", not \"" << ref_value << "\" as in reference file " << ref_file << std::endl;
              }
            }
          }
        }
      }
    }
  }

  void PulsarTestApp::checkOutputText(const std::string & file_name) {
    // Set reference file name.
    const std::string out_file(file_name);
    const std::string ref_file = facilities::commonUtilities::joinPath(m_outref_dir, out_file);

    // Check file existence.
    if (!tip::IFileSvc::instance().fileExists(out_file)) {
      err() << "File to check does not exist: " << out_file << std::endl;
      return;
    }
    if (!tip::IFileSvc::instance().fileExists(ref_file)) {
      err() << "Reference file for " << out_file << " does not exist: " << ref_file << std::endl;
      return;
    }

    // Open the files to compare.
    std::ifstream ifs_out(out_file.c_str());
    if (!ifs_out) err() << "Could not open file to check: " << out_file << std::endl;
    std::ifstream ifs_ref(ref_file.c_str());
    if (!ifs_ref) err() << "Could not open reference file for " << out_file << ": " << ref_file << std::endl;

    // Read the output file.
    static const size_t s_line_size = 2048;
    char out_buf[s_line_size];
    std::list<std::string> out_line_list;
    while (ifs_out) {
      ifs_out.getline(out_buf, s_line_size);
      out_line_list.push_back(out_buf);
    }

    // Read the reference file.
    char ref_buf[s_line_size];
    std::list<std::string> ref_line_list;
    while (ifs_ref) {
      ifs_ref.getline(ref_buf, s_line_size);
      ref_line_list.push_back(ref_buf);
    }

    // Compare the numbers of lines in the files.
    if (out_line_list.size() != ref_line_list.size()) {
      err() << "File " << out_file << " has " << out_line_list.size() << " line(s), not " << ref_line_list.size() <<
        " as in reference file " << ref_file << std::endl;

    } else {
      // Compare the file contents.
      std::list<std::string>::const_iterator out_itor = out_line_list.begin();
      std::list<std::string>::const_iterator ref_itor = ref_line_list.begin();
      bool files_differ = false;
      for (; out_itor != out_line_list.end() && ref_itor != ref_line_list.end(); ++out_itor, ++ref_itor) {
        if (*out_itor != *ref_itor) {
          files_differ = true;
          break;
        }
      }
      if (files_differ) {
        err() << "File " << out_file << " differs from reference file " << ref_file << std::endl;
      }
    }
  }

  void PulsarTestApp::testApplication(const std::string & app_name, const st_app::AppParGroup & par_group, const std::string & log_file,
    const std::string & out_fits, bool ignore_exception) {
    // Fake the application name for logging.
    const std::string app_name_save(st_stream::GetExecName());
    st_stream::SetExecName(app_name);

    // Set chatter.
    const int chat_save = st_stream::GetMaximumChatter();
    int chat = par_group["chatter"];
    st_stream::SetMaximumChatter(chat);

    // Set debug mode.
    const bool debug_mode_save = st_stream::GetDebugMode();
    bool debug_mode = par_group["debug"];
    st_stream::SetDebugMode(debug_mode);

    // Create and setup an application object.
    std::auto_ptr<st_app::StApp> app_ptr(createApplication(app_name));
    if (0 == app_ptr.get()) {
      err() << "Cannot create an application object: \"" << app_name << "\"" << std::endl;
      return;
    }
    app_ptr->setName(app_name);
    st_app::AppParGroup & pars(app_ptr->getParGroup());
    pars.setPromptMode(false);

    // Copy parameter values.
    for (hoops::GenParItor par_itor = pars.begin(); par_itor != pars.end(); ++par_itor) {
      const std::string & par_name((*par_itor)->Name());
      try {
        pars[par_name] = *(par_group[par_name].PrimValue());
      } catch (hoops::Hexception &) {}
    }

    // Determine logging and checking output.
    bool record_log(!log_file.empty());
    bool check_output(!out_fits.empty());

    // Capture output in a log file.
    std::ofstream ofs_log;
    if (record_log) {
      remove(log_file.c_str());
      ofs_log.open(log_file.c_str());
      st_stream::sterr.disconnect(std::cerr);
      st_stream::stlog.disconnect(std::clog);
      st_stream::stout.disconnect(std::cout);
      st_stream::sterr.connect(ofs_log);
      st_stream::stlog.connect(ofs_log);
      st_stream::stout.connect(ofs_log);
    }

    // Run the application.
    bool exception_caught = false;
    try {
      app_ptr->run();

    } catch (const std::exception & x) {
      // Simulate the behavior of balistic_main.cxx in st_app package.
      exception_caught = true;

      // Report the type of the exception if possible, using typeid; typeid can throw so be careful:
      const char * type_name = "std::exception";
      try {
        type_name = typeid(x).name();
      } catch (...) {
        // Ignore problems with typeid.
      }
      st_stream::sterr << "Caught " << type_name << " at the top level: " << x.what() << std::endl;

    } catch (...) {
      // Catch everything else and report it.
      exception_caught = true;
      err() << "Unknown exception thrown by application \"" << app_ptr->getName() << "\"" << std::endl;
    }

    if (record_log) {
      // Close the log file, and reconnect output streams to the standard ones.
      st_stream::sterr.disconnect(ofs_log);
      st_stream::stlog.disconnect(ofs_log);
      st_stream::stout.disconnect(ofs_log);
      ofs_log.close();
      st_stream::sterr.connect(std::cerr);
      st_stream::stlog.connect(std::clog);
      st_stream::stout.connect(std::cout);
    }

    // Output an error message if the application threw an exception, or check the output file against a reference file.
    if (exception_caught && !ignore_exception) {
      err() << "Application \"" << app_ptr->getName() << "\" threw an exception for the following parameter values:" << std::endl;
      const st_app::AppParGroup & pars(app_ptr->getParGroup());
      for (hoops::ConstGenParItor par_itor = pars.begin(); par_itor != pars.end(); ++par_itor) {
        err() << (*par_itor)->Name() << " = " << (*par_itor)->Value() << std::endl;
      }
    } else {
      // Compare the log with its reference.
      if (record_log) checkOutputText(log_file);

      // Compare the output FITS file with its reference.
      if (check_output) checkOutputFits(out_fits);
    }

    // Restore the application name, chatter, and debug mode.
    st_stream::SetExecName(app_name_save);
    st_stream::SetMaximumChatter(chat_save);
    st_stream::SetDebugMode(debug_mode_save);
  }

}
