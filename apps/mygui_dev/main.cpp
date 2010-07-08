#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "manager.hpp"
#include "layout.hpp"
#include "mw_layouts.hpp"

#include <components/engine/ogre/renderer.hpp>
#include <OgreResourceGroupManager.h>

#include <components/bsa/bsa_archive.hpp>

// Frame listener
struct Listener : public Ogre::FrameListener
{
  bool exit;
  float total;
  int step;

  Listener() : exit(false), total(0.0), step(0) {}

  bool frameStarted(const Ogre::FrameEvent &evt)
  {
    total += evt.timeSinceLastFrame;

    // Countdown to exit
    const int MAX = 5;
    if(total >= step)
      {
        step++;
        if(step<MAX)
          cout << "Exit in " << (MAX-step) << endl;
        else
          exit = true;
      }

    return !exit;
  }
};

int main()
{
  Render::OgreRenderer ogre;
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

  // Make sure you load the data paths BEFORE you initialize the
  // GUI. MyGUI depends on finding core.xml in resources/mygui/.
  cout << "Setting up MyGUI\n";
  GUI::MyGUIManager gui(ogre.getWindow(), ogre.getScene());

  int w = ogre.getWindow()->getWidth();
  int h = ogre.getWindow()->getHeight();

  cout << "Setting up the window layouts\n";
  MWGUI::HUD hud(w,h);
  MWGUI::MapWindow map;
  MWGUI::MainMenu menu(w,h);
  MWGUI::StatsWindow stats;

  hud.setVisible(true);
  map.setVisible(true);
  menu.setVisible(false);
  stats.setVisible(true);

  cout << "Starting rendering loop\n";
  ogre.start();
  ogre.screenshot("mygui_test.png");

  cout << "Done.\n";
  return 0;
}

/// Old D function
/*
extern "C" MyGUI::WidgetPtr gui_createText(const char *skin,
                                           int32_t x, int32_t y,
                                           int32_t w, int32_t h,
                                           const char *layer)
{
  return mGUI->createWidget<MyGUI::StaticText>
    (skin,
     x,y,w,h,
     MyGUI::ALIGN_LEFT | MyGUI::ALIGN_TOP,
     layer);
}
*/
