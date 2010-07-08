#ifndef ENGINE_MYGUI_MANAGER_H
#define ENGINE_MYGUI_MANAGER_H

#include <assert.h>
#include <MyGUI.h>
#include <MyGUI_OgrePlatform.h>

namespace GUI
{
  class MyGUIManager
  {
    MyGUI::OgrePlatform *mPlatform;
    MyGUI::Gui *mGui;

  public:
    MyGUIManager() : mPlatform(NULL), mGui(NULL) {}
    MyGUIManager(Ogre::RenderWindow *wnd, Ogre::SceneManager *mgr, bool logging=false)
    { setup(wnd,mgr,logging); }
    ~MyGUIManager() { shutdown(); }

    void setup(Ogre::RenderWindow *wnd, Ogre::SceneManager *mgr, bool logging=false)
    {
      assert(wnd);
      assert(mgr);

      using namespace MyGUI;

      // Enable/disable MyGUI logging to stdout. (Logging to MyGUI.log
      // is still enabled.) In order to do this we have to initialize
      // the log manager before the main gui system itself, otherwise
      // the main object will get the chance to spit out a few messages
      // before we can able to disable it.
      LogManager::initialise();
      LogManager::setSTDOutputEnabled(logging);

      // Set up OGRE platform. We might make this more generic later.
      mPlatform = new OgrePlatform();
      mPlatform->initialise(wnd, mgr);

      // Create GUI
      mGui = new Gui();
      mGui->initialise();
    }

    void shutdown()
    {
      if(mGui) delete mGui;
      if(mPlatform)
        {
          mPlatform->shutdown();
          delete mPlatform;
        }
      mGui = NULL;
      mPlatform = NULL;
    }
  };
}
#endif
