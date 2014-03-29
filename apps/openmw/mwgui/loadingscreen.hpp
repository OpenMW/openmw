#ifndef MWGUI_LOADINGSCREEN_H
#define MWGUI_LOADINGSCREEN_H

#include <OgreSceneManager.h>
#include <OgreTimer.h>

#include "windowbase.hpp"

#include <components/loadinglistener/loadinglistener.hpp>

namespace MWGui
{
    class LoadingScreen : public WindowBase, public Loading::Listener
    {
    public:
        virtual void setLabel (const std::string& label);

        /// Indicate that some progress has been made, without specifying how much
        virtual void indicateProgress ();

        virtual void loadingOn();
        virtual void loadingOff();

        virtual void setProgressRange (size_t range);
        virtual void setProgress (size_t value);
        virtual void increaseProgress (size_t increase);

        virtual void removeWallpaper();

        LoadingScreen(Ogre::SceneManager* sceneMgr, Ogre::RenderWindow* rw);
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

        size_t mProgress;

        MyGUI::TextBox* mLoadingText;
        MyGUI::ScrollBar* mProgressBar;
        MyGUI::ImageBox* mBackgroundImage;

        Ogre::Rectangle2D* mRectangle;
        Ogre::MaterialPtr mBackgroundMaterial;

        Ogre::StringVector mResources;

        bool mVSyncWasEnabled;

        void changeWallpaper();

        void draw();
    };

}


#endif
