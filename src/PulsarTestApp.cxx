/** \file PulsarTestApp.cxx
    \brief Implementation of base class for unit test application for pulsar tool packages.
    \author Masaharu Hirayama, GSSC
            James Peachey, HEASARC/GSSC
*/
#include "timeSystem/PulsarTestApp.h"

#include <stdexcept>

#include "facilities/commonUtilities.h"

#include "tip/Extension.h"
#include "tip/FileSummary.h"
#include "tip/Header.h"
#include "tip/IFileSvc.h"

namespace timeSystem {

  PulsarTestApp::PulsarTestApp(const std::string & package_name): m_failed(false), m_method_name(), m_outref_dir() {
    // Find data directory for this app.
    m_data_dir = facilities::commonUtilities::getDataPath(package_name);

    // Set the directory name for output reference files.
    m_outref_dir = facilities::commonUtilities::joinPath(m_data_dir, "outref");
  }

  PulsarTestApp::~PulsarTestApp() throw() {}

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

  void PulsarTestApp::reportStatus() const {
    if (m_failed) throw std::runtime_error(getName() + ": unit test failed.");
  }

void PulsarTestApp::checkOutputFits(const std::string & file_name, bool compare_comment) {
  // Set reference file name.
  std::string out_file(file_name);
  std::string ref_file = facilities::commonUtilities::joinPath(m_outref_dir, out_file);

  // Check file existence.
  if (!tip::IFileSvc::instance().fileExists(out_file)) {
    err() << "File to check does not exist: " << out_file << std::endl;
  }
  if (!tip::IFileSvc::instance().fileExists(ref_file)) {
    err() << "Reference file for " << out_file << " does not exist: " << ref_file << std::endl;
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

          // Compare keyword value.
          std::string out_value;
          std::string ref_value;
          // Note: Do not compare CHECKSUM, CREATOR, DATE, HISTORY, COMMENT, a blank name.

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


}
