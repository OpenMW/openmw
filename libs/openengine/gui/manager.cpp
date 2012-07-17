#include <MyGUI.h>
#include <MyGUI_OgrePlatform.h>
#include <cassert>

#include "manager.hpp"

using namespace OEngine::GUI;

void MyGUIManager::setup(Ogre::RenderWindow *wnd, Ogre::SceneManager *mgr, bool logging, const std::string& logDir)
{
  assert(wnd);
  assert(mgr);

  using namespace MyGUI;

  // Enable/disable MyGUI logging to stdout. (Logging to MyGUI.log is
  // still enabled.) In order to do this we have to initialize the log
  // manager before the main gui system itself, otherwise the main
  // object will get the chance to spit out a few messages before we
  // can able to disable it.

  std::string theLogFile = std::string(MYGUI_PLATFORM_LOG_FILENAME);
  if(!logDir.empty())
      theLogFile.insert(0, logDir);

  // Set up OGRE platform. We might make this more generic later.
  mPlatform = new OgrePlatform();
  LogManager::getInstance().setSTDOutputEnabled(logging);
  mPlatform->initialise(wnd, mgr, "General", theLogFile);


  // Create GUI
  mGui = new Gui();
  mGui->initialise("core.xml");
}

void MyGUIManager::shutdown()
{
  delete mGui;
  if(mPlatform)
    {
      mPlatform->shutdown();
      delete mPlatform;
    }
  mGui = NULL;
  mPlatform = NULL;
}
