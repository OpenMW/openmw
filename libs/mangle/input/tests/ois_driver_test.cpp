#include "common.cpp"

#include "../servers/ois_driver.hpp"
#include <Ogre.h>
#include <OIS/OIS.h>
#include <boost/filesystem.hpp>

bool isFile(const char *name)
{
  boost::filesystem::path cfg_file_path(name);
  return boost::filesystem::exists(cfg_file_path);
}

using namespace Ogre;
using namespace OIS;

Root *root;
RenderWindow *window;

void setupOgre()
{
  // Disable logging
  new LogManager;
  Log *log = LogManager::getSingleton().createLog("");
  log->setDebugOutputEnabled(false);

  bool useConfig = isFile("ogre.cfg");

  // Set up Root
  root = new Root("plugins.cfg", "ogre.cfg", "");

  // Configure
  if(!useConfig)
    root->showConfigDialog();
  else
    root->restoreConfig();

  // Initialize OGRE window
  window = root->initialise(true, "test", "");
}

int main(int argc, char** argv)
{
  setupOgre();
  input = new OISDriver(window);

  mainLoop(argc, KC_Q);

  delete root;
  return 0;
}
