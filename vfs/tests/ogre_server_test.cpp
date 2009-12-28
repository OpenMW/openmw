#include "../servers/ogre_vfs.h"

#include <Ogre.h>

#include "server_common.cpp"

Ogre::Root *root;

void setupOgre()
{
  using namespace Ogre;

  // Disable logging
  new LogManager;
  Log *log = LogManager::getSingleton().createLog("");
  log->setDebugOutputEnabled(false);

  // Set up Root
  root = new Root("","","");

  // Add a zip file and the current directory
  root->addResourceLocation("test.zip", "Zip", "General");
  root->addResourceLocation("./", "FileSystem", "General");
}

int main()
{
  // Set up the engine
  setupOgre();

  // This is our entry point into the resource file system
  OgreVFS vfs("General");

  // Run the test
  testAll(vfs);

  return 0;
}
