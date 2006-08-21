#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "st_app/AppParGroup.h"
#include "st_app/StApp.h"
#include "st_app/StAppFactory.h"

#include "tip/IFileSvc.h"
#include "tip/TipFile.h"

static const std::string s_cvs_id = "$Name:  $";

extern "C" {
  int axBary(char * inFile, char * orbitFile, char * outFile, double ra, double dec, char * refFrame, int debug);
}

class GbaryApp : public st_app::StApp {
  public:
    GbaryApp();
    virtual void run();
    std::string tmpFileName(const std::string & file_name) const;
};

GbaryApp::GbaryApp() {
  setVersion(s_cvs_id);
}

void GbaryApp::run() {
  st_app::AppParGroup & pars = getParGroup("gtbary");
  pars.Prompt();
  pars.Save();
  std::string inFile_s = pars["evfile"];
  std::string orbitFile_s = pars["scfile"];
  std::string outFile_s = pars["outfile"];
  double ra = pars["ra"];
  double dec = pars["dec"];
  bool clobber = pars["clobber"];
  bool debug = pars["debug"];
  char refFrame[32] = "";

  // If output file name is blank or "default" (case insensitive), let output file be input file.
  std::string outFile_uc = outFile_s;
  for (std::string::iterator itor = outFile_uc.begin(); itor != outFile_uc.end(); ++itor) *itor = std::toupper(*itor);

  if (outFile_s.empty() || "DEFAULT" == outFile_uc) {
    outFile_s = inFile_s;
  } else if (!clobber) {
    bool file_readable = false;
    try {
      std::ifstream is(outFile_s.c_str());
      if (is.good()) file_readable = true;
    } catch (const std::exception &) {}
    if (file_readable) throw std::runtime_error("File " + outFile_s + " exists, but clobber not set");
  }

  // Confirm that outfile is writable.
  bool file_writable = false;
  try {
    std::ofstream os(outFile_s.c_str(), std::ios::out | std::ios::app);
    if (os.good()) file_writable = true;
  } catch (const std::exception &) {}
  if (!file_writable)
    throw std::runtime_error("Cannot open file " + outFile_s + " for writing");

  // Create temporary output file name.
  std::string tmpOutFile_s = tmpFileName(outFile_s);

  // Open the input file.
  tip::TipFile inTipFile = tip::IFileSvc::instance().openFile(inFile_s);
  
  // Copy the input to the temporary output file.
  inTipFile.copyFile(tmpOutFile_s, true);

  char * orbitFile = const_cast<char *>(orbitFile_s.c_str());
  char * tmpOutFile = const_cast<char *>(tmpOutFile_s.c_str());
  int error = axBary(tmpOutFile, orbitFile, tmpOutFile, ra, dec, refFrame, debug ? 1 : 0);
  if (0 != error) {
    if (!debug) std::remove(tmpOutFile);
    throw std::runtime_error("GbaryApp::run() encountered an error when calling axBary");
  }

  // Move the temporary output file to the real output file.
  std::remove(outFile_s.c_str());
  std::rename(tmpOutFile, outFile_s.c_str());
}

std::string GbaryApp::tmpFileName(const std::string & file_name) const {
  return file_name + ".tmp";
}

st_app::StAppFactory<GbaryApp> g_factory("gtbary");
