#include <iostream>
#include <OgreRoot.h>
#include <OgreResourceGroupManager.h>

/*
  This isn't really a test of our implementation, but a test of using
  the Ogre resource system to find files. If the Ogre interface
  changes and you have to change this test, you will have to change
  the servers/ogre_vfs.cpp implementation equivalently.

 */

using namespace std;
using namespace Ogre;

ResourceGroupManager *gm;
String group;

void find(const std::string &fileName)
{
  cout << "\nFile: " << fileName << endl;

  if(!gm->resourceExists(group, fileName))
    {
      cout << "Does not exist\n";
      return;
    }

  DataStreamPtr data = gm->openResource(fileName, group);

  cout << "Size: " << data->size() << endl;
  cout << "First line: " << data->getLine() << "\n";
}

int main()
{
  // Disable logging
  new LogManager;
  Log *log = LogManager::getSingleton().createLog("");
  log->setDebugOutputEnabled(false);

  // Set up Ogre
  Root root("","","");

  root.addResourceLocation("./", "FileSystem", "General");

  gm = ResourceGroupManager::getSingletonPtr();
  group = gm->getWorldResourceGroupName();

  find("Makefile");
  find("ogre_resource_test.cpp");
  find("bleh");

  cout << "\nAll source files:\n";
  FileInfoListPtr list = gm->findResourceFileInfo(group, "*.cpp");
  FileInfoList::iterator it, end;
  it = list->begin();
  end = list->end();
  for(; it != end; it++)
    cout << "  " << it->filename << endl;
}
