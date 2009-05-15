/** \file PulsarTestApp.cxx
    \brief Implementation of base class for unit test application for pulsar tool packages.
    \author Masaharu Hirayama, GSSC
            James Peachey, HEASARC/GSSC
*/
#include "timeSystem/PulsarTestApp.h"

#include <cerrno>
#include <cmath>
#include <cctype>
#include <fstream>
#include <iostream>
#include <limits>
#include <list>

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
#include "tip/IColumn.h"
#include "tip/IFileSvc.h"
#include "tip/Table.h"
#include "tip/tip_types.h"

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

  std::string PulsarTestApp::prependDataPath(const std::string & base_name) const {
    return facilities::commonUtilities::joinPath(m_data_dir, base_name);
  }

  std::string PulsarTestApp::prependOutrefPath(const std::string & base_name) const {
    return facilities::commonUtilities::joinPath(m_outref_dir, base_name);
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

  PulsarApplicationTester::PulsarApplicationTester(const std::string & app_name, PulsarTestApp & test_app):
    m_app_name(app_name), m_test_app(&test_app) {}

  PulsarApplicationTester::~PulsarApplicationTester() throw() {}

  std::string PulsarApplicationTester::getName() const {
    return m_app_name;
  }

  std::ostream & PulsarApplicationTester::err() {
    return m_test_app->err();
  }

  bool PulsarApplicationTester::compareNumericString(const std::string & string_value, const std::string & string_reference) const {
    // Try string comparison first.
    if (string_value == string_reference) return false;

    // Prepare variables for comparison.
    const char * ptr_val_cur(string_value.c_str());
    char * ptr_val_next(0);
    const char * ptr_ref_cur(string_reference.c_str());
    char * ptr_ref_next(0);

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
        static const double tolerance_high = std::numeric_limits<double>::epsilon() * 1000.;
        static const double tolerance_low = 1.e-1;
        static const double tolerance_abs = 1.e-5;
        static const double large_number_boundary = tolerance_abs / tolerance_high;

        // Convert the value of interest.
        errno = 0;
        double double_val = std::strtod(ptr_val_cur, &ptr_val_next);
        if (errno) {
          // Flag a conversion error as a mismatch.
          mismatch_found = true;

        } else if (0. == double_ref) {
          // Require exact match when a reference is 0 (zero).
          if (double_val != double_ref) mismatch_found = true;

        } else if (double_ref < large_number_boundary) {
          // Apply loose comparison criteria for smaller numbers.
          double diff = std::fabs(double_val - double_ref);
          double ratio = std::fabs(diff / double_ref);
          if (diff > tolerance_abs || ratio > tolerance_low) mismatch_found = true;

        } else {
          // Apply the highest comparison criteria for larger numbers.
          if (std::fabs(double_val/double_ref - 1.) > tolerance_high) mismatch_found = true;
        }
        ptr_ref_cur = ptr_ref_next;
        ptr_val_cur = ptr_val_next;
      }
    }

    // Report the result.
    return mismatch_found;
  }

  bool PulsarApplicationTester::testEquivalence(const std::string & keyword_name, const tip::KeyRecord & out_keyword,
    const tip::KeyRecord & ref_keyword, std::ostream & error_stream) const {
    // Extract keyword values.
    std::string out_value;
    std::string ref_value;
    if ("COMMENT" != keyword_name && "HISTORY" != keyword_name) {
      out_value = out_keyword.getValue();
      ref_value = ref_keyword.getValue();
    } else {
      out_value = out_keyword.getComment();
      ref_value = ref_keyword.getComment();
    }

    // Compare values and produce error message if any.
    if (compareNumericString(out_value, ref_value)) {
      error_stream << "Value \"" << out_value << "\" not equivalent to reference value \"" << ref_value << "\"";
      return false;
    } else {
      return true;
    }
  }

  bool PulsarApplicationTester::testEquivalence(const std::string & /*keyword_name*/, const tip::TableCell & out_cell,
    const tip::TableCell & ref_cell, std::ostream & error_stream) const {
    // Extract cell values.
    std::string out_value;
    std::string ref_value;
    out_cell.get(out_value);
    ref_cell.get(ref_value);

    // Compare values and produce error message if any.
    if (compareNumericString(out_value, ref_value)) {
      error_stream << "Value \"" << out_value << "\" not equivalent to reference value \"" << ref_value << "\"";
      return false;
    } else {
      return true;
    }
  }

  bool PulsarApplicationTester::testEquivalence(const std::string & out_string, const std::string & ref_string,
    std::ostream & error_stream) const {
    if (compareNumericString(out_string, ref_string)) {
      error_stream << "String \"" << out_string << "\" not equivalent to reference string \"" << ref_string << "\"";
      return false;
    } else {
      return true;
    }
  }

  void PulsarApplicationTester::checkOutputFits(const std::string & out_file, const std::string & ref_file) {
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
      // Compare each extension.
      int num_extension = ref_size;
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
        // Note3: DATASUM will be checked later, but not here.
        std::set<std::string> ignored_keyword;
        ignored_keyword.insert("CHECKSUM");
        ignored_keyword.insert("DATASUM");
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

            } else {
              // Compare keyword values.
              std::ostringstream os_err;
              if (!testEquivalence(ref_name, out_itor->second, ref_itor->second, os_err)) {
                err() << "Header keyword " << out_name << " on card " << out_card_number << " of HDU " << ext_name <<
                  " in file " << out_file << " differs from one on card " << ref_card_number << " in reference file " <<
                  ref_file << ": " << os_err.str() << std::endl;
              }
            }
          }
        }

        // Close files as an extension before re-opening them as a table.
        out_ext.reset(0);
        ref_ext.reset(0);

        // Compare the tables, except for primary HDU's.
        if (ext_number != 0) {
          std::auto_ptr<const tip::Table> out_table(tip::IFileSvc::instance().readTable(out_file, ext_name));
          std::auto_ptr<const tip::Table> ref_table(tip::IFileSvc::instance().readTable(ref_file, ext_name));

          // Compare the number of rows.
          tip::Index_t out_num_row = out_table->getNumRecords();
          tip::Index_t ref_num_row = ref_table->getNumRecords();
          if (out_num_row != ref_num_row) {
            err() << "HDU " << ext_name << " of file " << out_file << " contains " << out_num_row <<
              " row(s), not " << ref_num_row << " as in reference file " << ref_file << std::endl;
          }

          // Create a list of columns that are in both tables.
          std::list<std::string> common_column;

          // Compare the number of columns.
          tip::Table::FieldCont::size_type out_num_col = out_table->getValidFields().size();
          tip::Table::FieldCont::size_type ref_num_col = ref_table->getValidFields().size();
          if (out_num_col != ref_num_col) {
            err() << "HDU " << ext_name << " of file " << out_file << " contains " << out_num_col <<
              " column(s), not " << ref_num_col << " as in reference file " << ref_file << std::endl;

          } else {
            // Compare the names of all columns.
            tip::FieldIndex_t num_col = ref_num_col;
            for (tip::FieldIndex_t col_index = 0; col_index < num_col; ++col_index) {
              const tip::IColumn * out_column(out_table->getColumn(col_index));
              const tip::IColumn * ref_column(ref_table->getColumn(col_index));

              // Make all column names upper cases for consistency in error messages.
              std::string out_col_name(out_column->getId());
              for (std::string::iterator str_itor = out_col_name.begin(); str_itor != out_col_name.end(); ++str_itor) {
                *str_itor = std::toupper(*str_itor);
              }
              std::string ref_col_name(ref_column->getId());
              for (std::string::iterator str_itor = ref_col_name.begin(); str_itor != ref_col_name.end(); ++str_itor) {
                *str_itor = std::toupper(*str_itor);
              }

              // Compare column names.
              if (out_col_name == ref_col_name) {
                common_column.push_back(ref_col_name);
              } else {
                err() << "Column #" << col_index + 1 << " of HDU " << ext_name << " in file " << out_file << " is named " <<
                  out_col_name << ", not " << ref_col_name << " as in reference file " << ref_file << std::endl;
              }
            }
          }

          if (out_num_row == ref_num_row && out_num_col == ref_num_col) {
            // Compare DATASUM keyword values.
            bool datasum_matched = false;
            const tip::Header & out_header(out_table->getHeader());
            const tip::Header & ref_header(ref_table->getHeader());
            if (out_header.find("DATASUM") != out_header.end() && ref_header.find("DATASUM") != ref_header.end()) {
              std::string out_datasum;
              std::string ref_datasum;
              out_header["DATASUM"].get(out_datasum);
              ref_header["DATASUM"].get(ref_datasum);
              datasum_matched = (out_datasum == ref_datasum);
            }

            // Compare tables if DATASUM's do not match.
            if (!datasum_matched) {
              // Compare each row.
              tip::Table::ConstIterator out_itor = out_table->begin();
              tip::Table::ConstIterator ref_itor = ref_table->begin();
              tip::Index_t row_index = 1;
              for (; out_itor != out_table->end() && ref_itor != ref_table->end(); ++out_itor, ++ref_itor, ++row_index) {
                tip::ConstTableRecord & out_record = *out_itor;
                tip::ConstTableRecord & ref_record = *ref_itor;

                // Compare all columns.
                for (std::list<std::string>::const_iterator col_itor = common_column.begin(); col_itor != common_column.end();
                  ++col_itor) {
                  const std::string & col_name = *col_itor;
                  const tip::TableCell & out_cell = out_record[col_name];
                  const tip::TableCell & ref_cell = ref_record[col_name];
                  std::ostringstream os_err;
                  if (!testEquivalence(col_name, out_cell, ref_cell, os_err)) {
                    err() << "Row #" << row_index << " of column \"" << col_name << "\" in HDU " << ext_name << " in file " <<
                      out_file << " differs from reference file " << ref_file << ": " << os_err.str() << std::endl;
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  void PulsarApplicationTester::checkOutputText(const std::string & out_file, const std::string & ref_file) {
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
      int line_number = 1;
      for (; out_itor != out_line_list.end() && ref_itor != ref_line_list.end(); ++out_itor, ++ref_itor, ++line_number) {
        std::ostringstream os_err;
        if (!testEquivalence(*out_itor, *ref_itor, os_err)) {
          err() << "Line " << line_number << " of file " << out_file << " differs from reference file " << ref_file <<
            ": " << os_err.str() << std::endl;
        }
      }
    }
  }

  void PulsarApplicationTester::test(const st_app::AppParGroup & par_group, const std::string & log_file,
    const std::string & log_file_ref, const std::string & out_file, const std::string & out_file_ref, bool ignore_exception) {
    // Fake the application name for logging.
    const std::string app_name_save(st_stream::GetExecName());
    st_stream::SetExecName(m_app_name);

    // Set chatter.
    const int chat_save = st_stream::GetMaximumChatter();
    int chat = par_group["chatter"];
    st_stream::SetMaximumChatter(chat);

    // Set debug mode.
    const bool debug_mode_save = st_stream::GetDebugMode();
    bool debug_mode = par_group["debug"];
    st_stream::SetDebugMode(debug_mode);

    // Create and setup an application object.
    std::auto_ptr<st_app::StApp> app_ptr(createApplication());
    if (0 == app_ptr.get()) {
      err() << "Cannot create an application object: \"" << m_app_name << "\"" << std::endl;
      return;
    }
    app_ptr->setName(m_app_name);
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
    bool check_output(!out_file.empty());

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
      writeException(st_stream::sterr, x);

    } catch (...) {
      // Catch everything else and report it as an error in this unit test.
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
      if (record_log) checkOutputText(log_file, log_file_ref);

      // Compare the output FITS file with its reference.
      if (check_output) checkOutputFits(out_file, out_file_ref);
    }

    // Restore the application name, chatter, and debug mode.
    st_stream::SetExecName(app_name_save);
    st_stream::SetMaximumChatter(chat_save);
    st_stream::SetDebugMode(debug_mode_save);
  }

}
