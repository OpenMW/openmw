#include <iostream>
#include <OgreRoot.h>
#include <OgreResourceGroupManager.h>

/*
  This isn't really a test of our implementation, but a test of using
  the Ogre resource system to find files. If the Ogre interface
  changes and you have to change this test, you will have to change
  the ogre_vfs.cpp implementation equivalently.

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


  // Alternative - not used / fixed yet

  /* This won't work, since we don't have access to Ogre
     internals. That's a shame.

  LocationList::iterator li, liend;
  liend = grp->locationList.end();
  for (li = grp->locationList.begin(); li != liend; ++li)
    {
      Archive* arch = (*li)->archive;

      // The rest is client code - using an archive. We might make a
      // shared implementation, or possibly convert the archives into
      // a vfs list at load time (although that isn't very flexible.)

      // Do we perform these searches in each function? I guess we
      // have to.
      if (arch->exists(resourceName))
        {
          DataStreamPtr ptr = arch->open(resourceName);
          return ptr;
        }
    }
  */
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
