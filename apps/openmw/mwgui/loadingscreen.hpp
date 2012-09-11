#ifndef MWGUI_LOADINGSCREEN_H
#define MWGUI_LOADINGSCREEN_H

#include <OgreSceneManager.h>
#include <OgreResourceGroupManager.h>

#include "window_base.hpp"

namespace MWGui
{

    class LoadingScreen : public WindowBase
    {
    public:
        LoadingScreen(Ogre::SceneManager* sceneMgr, Ogre::RenderWindow* rw, MWBase::WindowManager& parWindowManager);
        virtual ~LoadingScreen();

        void setLoadingProgress (const std::string& stage, int depth, int current, int total);

    private:
        Ogre::SceneManager* mSceneMgr;
        Ogre::RenderWindow* mWindow;

        unsigned long mLastRenderTime;
        Ogre::Timer mTimer;

        MyGUI::TextBox* mLoadingText;

        int mCurrentCellLoading;
        int mTotalCellsLoading;
        int mCurrentRefLoading;
        int mTotalRefsLoading;


        bool mLoadingOn;

        void loadingOn();
        void loadingOff();
    };

}


#endif
