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
        void loadingDone();

        void onResChange(int w, int h);

        void updateWindow(Ogre::RenderWindow* rw) { mWindow = rw; }

    private:
        bool mFirstLoad;

        Ogre::SceneManager* mSceneMgr;
        Ogre::RenderWindow* mWindow;

        unsigned long mLastWallpaperChangeTime;
        unsigned long mLastRenderTime;
        Ogre::Timer mTimer;

        MyGUI::TextBox* mLoadingText;
        MyGUI::ProgressBar* mProgressBar;
        MyGUI::ImageBox* mBackgroundImage;

        int mCurrentCellLoading;
        int mTotalCellsLoading;
        int mCurrentRefLoading;
        int mTotalRefsLoading;
        int mCurrentRefList;

        Ogre::Rectangle2D* mRectangle;
        Ogre::MaterialPtr mBackgroundMaterial;

        Ogre::StringVectorPtr mResources;

        bool mLoadingOn;

        void loadingOn();
        void loadingOff();

        void changeWallpaper();
    };

}


#endif
