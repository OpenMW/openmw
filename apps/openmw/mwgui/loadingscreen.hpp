#ifndef MWGUI_LOADINGSCREEN_H
#define MWGUI_LOADINGSCREEN_H

#include <memory>

#include <osg/Timer>
#include <osg/ref_ptr>

#include "windowbase.hpp"

#include <components/loadinglistener/loadinglistener.hpp>

namespace osgViewer
{
    class Viewer;
}

namespace osg
{
    class Texture2D;
}

namespace VFS
{
    class Manager;
}

namespace MWGui
{
    class BackgroundImage;

    class LoadingScreen : public WindowBase, public Loading::Listener
    {
    public:
        LoadingScreen(const VFS::Manager* vfs, osgViewer::Viewer* viewer);
        virtual ~LoadingScreen();

        /// Overridden from Loading::Listener, see the Loading::Listener documentation for usage details
        virtual void setLabel (const std::string& label, bool important, bool center);
        virtual void loadingOn(bool visible=true);
        virtual void loadingOff();
        virtual void setProgressRange (size_t range);
        virtual void setProgress (size_t value);
        virtual void increaseProgress (size_t increase=1);

        virtual void setVisible(bool visible);

        double getTargetFrameRate() const;

    private:
        void findSplashScreens();
        bool needToDrawLoadingScreen();

        void setupCopyFramebufferToTextureCallback();

        const VFS::Manager* mVFS;
        osg::ref_ptr<osgViewer::Viewer> mViewer;

        double mTargetFrameRate;

        double mLastWallpaperChangeTime;
        double mLastRenderTime;
        osg::Timer mTimer;
        double mLoadingOnTime;

        bool mImportantLabel;

        bool mVisible;

        size_t mProgress;

        bool mShowWallpaper;

        MyGUI::Widget* mLoadingBox;

        MyGUI::TextBox* mLoadingText;
        MyGUI::ScrollBar* mProgressBar;
        BackgroundImage* mBackgroundImage;
        BackgroundImage* mSceneImage;

        std::vector<std::string> mSplashScreens;

        osg::ref_ptr<osg::Texture2D> mTexture;
        std::unique_ptr<MyGUI::ITexture> mGuiTexture;

        void changeWallpaper();

        void draw();
    };

}


#endif
