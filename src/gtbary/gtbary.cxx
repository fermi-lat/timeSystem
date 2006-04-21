#include <stdexcept>
#include <string>

#include "st_app/AppParGroup.h"
#include "st_app/StApp.h"
#include "st_app/StAppFactory.h"

static const std::string s_cvs_id = "$Name:  $";

extern "C" {
  int axBary(char * inFile, char * orbitFile, char * outFile, double ra, double dec, char * refFrame, int debug);
}

class GbaryApp : public st_app::StApp {
  public:
    GbaryApp();
    virtual void run();
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
  double ra = pars["ra"];
  double dec = pars["dec"];
  char refFrame[32] = "";
  int debug = 0;
  char * inFile = const_cast<char *>(inFile_s.c_str());
  char * orbitFile = const_cast<char *>(orbitFile_s.c_str());
  int error = axBary(inFile, orbitFile, inFile, ra, dec, refFrame, debug);
  if (0 != error) throw std::runtime_error("GbaryApp::run() encountered an error when calling axBary");
}

st_app::StAppFactory<GbaryApp> g_factory("gtbary");
