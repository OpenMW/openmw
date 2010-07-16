#include <iostream>
using namespace std;

#include <openengine/gui/manager.hpp>
#include <openengine/gui/events.hpp>
#include <openengine/ogre/renderer.hpp>

#include <mangle/input/servers/ois_driver.hpp>

#include <components/mwgui/mw_layouts.hpp>
#include <components/bsa/bsa_archive.hpp>

#include <OgreResourceGroupManager.h>
#include <OgreRenderWindow.h>
#include <OgreFrameListener.h>
#include <OgreRoot.h>

#include <OIS/OISKeyboard.h>

// Frame listener
struct Listener : public Ogre::FrameListener
{
  bool exit;

  Mangle::Input::Driver *input;

  Listener() : exit(false) {}

  bool frameStarted(const Ogre::FrameEvent &evt)
  {
    if(input)
      input->capture();

    if(input->isDown(OIS::KC_ESCAPE))
      exit = true;

    return !exit;
  }
};

int main()
{
  OEngine::Render::OgreRenderer ogre;
  ogre.configure(false, "plugins.cfg", false);
  ogre.createWindow("MyGUI test");
  ogre.createScene();

  Listener listener;
  ogre.getRoot()->addFrameListener(&listener);

  cout << "Adding data path and BSA\n";
  // Add the Morrowind window resources
  Ogre::ResourceGroupManager::getSingleton().
    addResourceLocation("resources/mygui/", "FileSystem", "General");

  // And add the BSA, since most of the window bitmaps are located
  // there
  addBSA("data/Morrowind.bsa");

  cout << "Setting up input with OIS\n";
  Mangle::Input::OISDriver input(ogre.getWindow());
  listener.input = &input;

  // Make sure you load the data paths BEFORE you initialize the
  // GUI. MyGUI depends on finding core.xml in resources/mygui/.
  cout << "Setting up MyGUI\n";
  OEngine::GUI::MyGUIManager gui(ogre.getWindow(), ogre.getScene());

  int w = ogre.getWindow()->getWidth();
  int h = ogre.getWindow()->getHeight();

  cout << "Connecting to input\n";
  OEngine::GUI::EventInjector *evt = new OEngine::GUI::EventInjector(gui.getGui());
  input.setEvent(Mangle::Input::EventPtr(evt));

  cout << "Setting up the window layouts\n";
  MWGui::HUD hud(w,h);
  MWGui::MapWindow map;
  MWGui::MainMenu menu(w,h);
  MWGui::StatsWindow stats;

  hud.setVisible(true);
  map.setVisible(true);
  menu.setVisible(false);
  stats.setVisible(true);

  cout << "Starting rendering loop\n";
  cout << "PRESS ESCAPE TO EXIT\n";
  ogre.start();
  ogre.screenshot("mygui_test.png");

  cout << "Done.\n";
  return 0;
}
