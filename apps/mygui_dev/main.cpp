#include <iostream>
using namespace std;

#include <MyGUI.h>
#include <MyGUI_OgrePlatform.h>
using namespace MyGUI;

#include <components/engine/ogre/renderer.hpp>
#include <OgreResourceGroupManager.h>

int main()
{
  Render::OgreRenderer ogre;
  ogre.configure(false, "plugins.cfg", false);
  ogre.createWindow("MyGUI test");
  ogre.createScene();

  // Disable MyGUI logging
  LogManager::initialise();
  LogManager::setSTDOutputEnabled(false);

  // Set up OGRE connection to MyGUI
  OgrePlatform *platform = new OgrePlatform();
  platform->initialise(ogre.getWindow(), ogre.getScene());

  // Create GUI
  Gui *gui = new Gui();
  gui->initialise();

  // Add the Morrowind windows resources
  Ogre::ResourceGroupManager::getSingleton().
    addResourceLocation("resources/mygui/", "FileSystem", "General");

  return 0;
}
