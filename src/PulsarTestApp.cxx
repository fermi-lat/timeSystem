/** \file PulsarTestApp.cxx
    \brief Implementation of base class for unit test application for pulsar tool packages.
    \author Masaharu Hirayama, GSSC
            James Peachey, HEASARC/GSSC
*/
#include "timeSystem/PulsarTestApp.h"

#include <cerrno>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
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

  bool PulsarTestApp::compareNumericString(const std::string & string_value, const std::string & string_reference) {
    // Try string comparison first.
    if (string_value == string_reference) return false;

    // Prepare variables for comparison.
    const char * ptr_val_cur(string_value.c_str());
    char * ptr_val_next(0);
    const char * ptr_ref_cur(string_reference.c_str());
    char * ptr_ref_next(0);
    double tolerance = std::numeric_limits<double>::epsilon() * 1000.;

    // Loop over reference string.
    bool mismatch_found = false;
    while (!mismatch_found && *ptr_ref_cur != '\0') {

      // Try to read it as a number.
      errno = 0;
      double double_ref = std::strtod(ptr_ref_cur, &ptr_ref_next);

      // Handle each case.
      if (errno) {
        // Compare the same number of characters if failed to read a number.
        std::size_t num_char(ptr_ref_next - ptr_ref_cur);
        if (std::string(ptr_val_cur, num_char) != std::string(ptr_ref_cur, num_char)) mismatch_found = true;
        ptr_ref_cur += num_char;
        ptr_val_cur += num_char;

      } else if (ptr_ref_next == ptr_ref_cur) {
        // Compare one character if it is not a number.
        if (*ptr_val_cur != *ptr_ref_cur) mismatch_found = true;
        ++ptr_ref_cur;
        ++ptr_val_cur;

      } else {
        // Compare as a double value if it is a number.
        errno = 0;
        double double_val = std::strtod(ptr_val_cur, &ptr_val_next);
        if (errno) mismatch_found = true;
        else if (double_val != double_ref) {
          if (double_ref == 0.0) mismatch_found = true;
          else if (std::fabs(double_val/double_ref - 1.) > tolerance) mismatch_found = true;
        }
        ptr_ref_cur = ptr_ref_next;
        ptr_val_cur = ptr_val_next;
      }
    }

    // Report the result.
    return mismatch_found;
  }


  void PulsarTestApp::checkOutputFits(const std::string & out_file) {
    // Set reference file name.
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
        std::auto_ptr<const tip::Extension> out_ext(tip::IFileSvc::instance().readExtension(out_file, ext_name));
        const tip::Header & out_header(out_ext->getHeader());
        std::auto_ptr<const tip::Extension> ref_ext(tip::IFileSvc::instance().readExtension(ref_file, ext_name));
        const tip::Header & ref_header(ref_ext->getHeader());

        // List header keywords to ignore in comparison.
        // Note1: COMMENT should be compared because period search results are written in COMMENT keywords.
        // Note2: HISTORY should NOT be compared because it contains a file creation time.
        std::set<std::string> ignored_keyword;
        ignored_keyword.insert("CHECKSUM");
        ignored_keyword.insert("CREATOR");
        ignored_keyword.insert("DATE");
        ignored_keyword.insert("HISTORY");

        // Collect header keywords to compare.
        typedef std::list<std::pair<int, tip::KeyRecord> > key_record_cont;
        key_record_cont out_key_record;
        key_record_cont ref_key_record;
        for (int ii = 0; ii < 2; ++ii) {
          bool is_out(ii%2 == 0);
          key_record_cont & this_key_record = (is_out ? out_key_record : ref_key_record);
          const tip::Header & this_header = (is_out ? out_header : ref_header);

          int card_number = 1;
          for (tip::Header::ConstIterator key_itor = this_header.begin(); key_itor != this_header.end(); ++key_itor, ++card_number) {
            std::string key_name = key_itor->getName();
            if (!key_name.empty() && ignored_keyword.find(key_name) == ignored_keyword.end()) {
              this_key_record.push_back(std::make_pair(card_number, *key_itor));
            }
          }
        }

        // Compare the sizes of header.
        tip::Header::KeySeq_t::size_type out_num_key = out_key_record.size();
        tip::Header::KeySeq_t::size_type ref_num_key = ref_key_record.size();
        if (out_num_key != ref_num_key) {
          err() << "HDU " << ext_name << " of file " << out_file << " contains " << out_num_key <<
            " header keyword(s) to compare, not " << ref_num_key << " as in reference file " << ref_file << std::endl;

        } else {
          // Compare each header keyword.
          key_record_cont::const_iterator out_itor = out_key_record.begin();
          key_record_cont::const_iterator ref_itor = ref_key_record.begin();
          for (; out_itor != out_key_record.end() && ref_itor != ref_key_record.end(); ++out_itor, ++ref_itor) {
            // Get card numbers.
            const int out_card_number = out_itor->first;
            const int ref_card_number = ref_itor->first;

            // Compare keyword name.
            std::string out_name = out_itor->second.getName();
            std::string ref_name = ref_itor->second.getName();
            if (out_name != ref_name) {
              err() << "Card " << out_card_number << " of HDU " << ext_name << " in file " << out_file <<
                " is header keyword " << out_name << ", not " << ref_name << " as on card " << ref_card_number <<
                " in reference file " << ref_file << std::endl;
            }

            // Compare keyword values.
            std::string out_value;
            std::string ref_value;
            if ("COMMENT" != ref_name && "HISTORY" != ref_name) {
              out_value = out_itor->second.getValue();
              ref_value = ref_itor->second.getValue();
            } else {
              out_value = out_itor->second.getComment();
              ref_value = ref_itor->second.getComment();
            }

            if (compareNumericString(out_value, ref_value)) {
              err() << "Header keyword " << out_name << " on card " << out_card_number << " of HDU " << ext_name <<
                " in file " << out_file << " has value \"" << out_value << "\", not \"" << ref_value << "\" as on card " <<
                ref_card_number << " in reference file " << ref_file << std::endl;
            }
          }
        }
      }
    }
  }

  void PulsarTestApp::checkOutputText(const std::string & out_file) {
    // Set reference file name.
    const std::string ref_file = facilities::commonUtilities::joinPath(m_outref_dir, out_file);

    // Delegate file comparison to the following method.
    checkOutputText(out_file, ref_file);
  }

  void PulsarTestApp::checkOutputText(const std::string & out_file, const std::string & ref_file) {
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
      int line_number = 1;
      for (; out_itor != out_line_list.end() && ref_itor != ref_line_list.end(); ++out_itor, ++ref_itor, ++line_number) {
        if (compareNumericString(*out_itor, *ref_itor)) {
          files_differ = true;
          err() << "Line " << line_number << " of file " << out_file << " is \"" << *out_itor << "\", not \"" << *ref_itor <<
            "\" as in reference file " << ref_file << std::endl;
        }
      }
      if (files_differ) {
        err() << "File " << out_file << " differs from reference file " << ref_file << std::endl;
      }
    }
  }

  void PulsarTestApp::testApplication(const std::string & app_name, const st_app::AppParGroup & par_group, const std::string & log_file,
    const std::string & ref_file, const std::string & out_fits, bool ignore_exception) {
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
      if (record_log) {
        if (ref_file.empty()) checkOutputText(log_file);
        else checkOutputText(log_file, ref_file);
      }

      // Compare the output FITS file with its reference.
      if (check_output) checkOutputFits(out_fits);
    }

    // Restore the application name, chatter, and debug mode.
    st_stream::SetExecName(app_name_save);
    st_stream::SetMaximumChatter(chat_save);
    st_stream::SetDebugMode(debug_mode_save);
  }

}
