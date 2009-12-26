#include "../imp_server/ogre_vfs.h"
#include <iostream>

#include <Ogre.h>

using namespace std;
using namespace Mangle::VFS;
using namespace Mangle::Stream;

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

void find(VFS &vfs, const std::string &file)
{
  cout << "\nFile: " << file << endl;

  if(!vfs.isFile(file))
    {
      cout << "File doesn't exist\n";
      return;
    }

  InputStream *data = vfs.open(file);

  cout << "Size: " << data->size() << endl;

  char buf[13];
  buf[12] = 0;
  data->read(buf, 12);

  cout << "First 12 bytes: " << buf << "\n";
}

int main()
{
  // Set up the engine
  setupOgre();

  // This is our entry point into the resource file system
  OgreVFS vfs("General");

  find(vfs, "Makefile");     // From the file system
  find(vfs, "testfile.txt"); // From the zip
  find(vfs, "blah_bleh");    // Doesn't exist

  return 0;
}
