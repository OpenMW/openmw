#ifndef OENGINE_MYGUI_MANAGER_H
#define OENGINE_MYGUI_MANAGER_H

namespace MyGUI
{
  class OgrePlatform;
  class Gui;
}

namespace Ogre
{
  class RenderWindow;
  class SceneManager;
}

namespace OEngine {
namespace GUI
{
  class MyGUIManager
  {
    MyGUI::OgrePlatform *mPlatform;
    MyGUI::Gui *mGui;

  public:
    MyGUIManager() : mPlatform(NULL), mGui(NULL) {}
    MyGUIManager(Ogre::RenderWindow *wnd, Ogre::SceneManager *mgr, bool logging=false, const std::string& logDir = std::string(""))
    { setup(wnd,mgr,logging, logDir); }
    ~MyGUIManager() { shutdown(); }

    void setup(Ogre::RenderWindow *wnd, Ogre::SceneManager *mgr, bool logging=false, const std::string& logDir = std::string(""));
    void shutdown();

    MyGUI::Gui *getGui() { return mGui; }
  };
}}
#endif
