#ifndef OENGINE_MYGUI_MANAGER_H
#define OENGINE_MYGUI_MANAGER_H

#include <string>

namespace MyGUI
{
  class Gui;
  class LogManager;
  class OgreDataManager;
  class OgreRenderManager;
  class ShaderBasedRenderManager;
  class LogFacility;
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
        MyGUI::Gui *mGui;
        MyGUI::LogManager* mLogManager;
        MyGUI::LogFacility* mLogFacility;
        MyGUI::OgreDataManager* mDataManager;
        MyGUI::OgreRenderManager* mRenderManager;
        MyGUI::ShaderBasedRenderManager* mShaderRenderManager;
        Ogre::SceneManager* mSceneMgr;


    public:
        MyGUIManager(Ogre::RenderWindow *wnd, Ogre::SceneManager *mgr, bool logging=false, const std::string& logDir = std::string(""))
        {
            setup(wnd,mgr,logging, logDir);
        }
        ~MyGUIManager()
        {
            shutdown();
        }

        void windowResized();

        void setup(Ogre::RenderWindow *wnd, Ogre::SceneManager *mgr, bool logging=false, const std::string& logDir = std::string(""));
        void shutdown();
    };
}
}
#endif
