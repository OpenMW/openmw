#include "loadingscreen.hpp"

#include <osgViewer/Viewer>

#include <MyGUI_RenderManager.h>
#include <MyGUI_ScrollBar.h>
#include <MyGUI_Gui.h>
#include <MyGUI_TextBox.h>

#include <components/misc/rng.hpp>

#include <components/settings/settings.hpp>
#include <components/vfs/manager.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/statemanager.hpp"

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/inputmanager.hpp"

#include "../mwrender/vismask.hpp"

#include "backgroundimage.hpp"

namespace MWGui
{

    LoadingScreen::LoadingScreen(const VFS::Manager* vfs, osgViewer::Viewer* viewer)
        : WindowBase("openmw_loading_screen.layout")
        , mVFS(vfs)
        , mViewer(viewer)
        , mLastWallpaperChangeTime(0.0)
        , mLastRenderTime(0.0)
        , mLoadingOnTime(0.0)
        , mProgress(0)
        , mVSyncWasEnabled(false)
    {
        mMainWidget->setSize(MyGUI::RenderManager::getInstance().getViewSize());

        getWidget(mLoadingText, "LoadingText");
        getWidget(mProgressBar, "ProgressBar");
        getWidget(mLoadingBox, "LoadingBox");

        mProgressBar->setScrollViewPage(1);

        mBackgroundImage = MyGUI::Gui::getInstance().createWidgetReal<BackgroundImage>("ImageBox", 0,0,1,1,
            MyGUI::Align::Stretch, "Menu");

        setVisible(false);

        findSplashScreens();
    }

    void LoadingScreen::findSplashScreens()
    {
        const std::map<std::string, VFS::File*>& index = mVFS->getIndex();
        std::string pattern = "Splash/";
        mVFS->normalizeFilename(pattern);

        std::map<std::string, VFS::File*>::const_iterator found = index.lower_bound(pattern);
        while (found != index.end())
        {
            const std::string& name = found->first;
            if (name.size() >= pattern.size() && name.substr(0, pattern.size()) == pattern)
            {
                size_t pos = name.find_last_of('.');
                if (pos != std::string::npos && name.compare(pos, name.size()-pos, ".tga") == 0)
                    mSplashScreens.push_back(found->first);
            }
            else
                break;
            ++found;
        }
        if (mSplashScreens.empty())
            std::cerr << "No splash screens found!" << std::endl;
    }

    void LoadingScreen::setLabel(const std::string &label)
    {
        mLoadingText->setCaptionWithReplacing(label);
        int padding = mLoadingBox->getWidth() - mLoadingText->getWidth();
        MyGUI::IntSize size(mLoadingText->getTextSize().width+padding, mLoadingBox->getHeight());
        size.width = std::max(300, size.width);
        mLoadingBox->setSize(size);
        mLoadingBox->setPosition(mMainWidget->getWidth()/2 - mLoadingBox->getWidth()/2, mLoadingBox->getTop());
    }

    LoadingScreen::~LoadingScreen()
    {
    }

    void LoadingScreen::setVisible(bool visible)
    {
        WindowBase::setVisible(visible);
        mBackgroundImage->setVisible(visible);
    }

    void LoadingScreen::loadingOn()
    {
        mLoadingOnTime = mTimer.time_m();
        // Early-out if already on
        if (mMainWidget->getVisible())
            return;

        if (mViewer->getIncrementalCompileOperation())
        {
            mViewer->getIncrementalCompileOperation()->setMaximumNumOfObjectsToCompilePerFrame(200);
            // keep this in sync with loadingScreenFps
            mViewer->getIncrementalCompileOperation()->setTargetFrameRate(1.0/120.0);
        }

        bool showWallpaper = (MWBase::Environment::get().getStateManager()->getState()
                == MWBase::StateManager::State_NoGame);

        if (!showWallpaper)
        {
            // TODO
            /*
            mBackgroundImage->setImageTexture("");
            int width = mWindow->getWidth();
            int height = mWindow->getHeight();
            const std::string textureName = "@loading_background";
            Ogre::TexturePtr texture;
            texture = Ogre::TextureManager::getSingleton().getByName(textureName);
            if (texture.isNull())
            {
                texture = Ogre::TextureManager::getSingleton().createManual(textureName,
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    Ogre::TEX_TYPE_2D,
                    width, height, 0, mWindow->suggestPixelFormat(), Ogre::TU_DYNAMIC_WRITE_ONLY);
            }
            texture->unload();
            texture->setWidth(width);
            texture->setHeight(height);
            texture->createInternalResources();
            mWindow->copyContentsToMemory(texture->getBuffer()->lock(Ogre::Image::Box(0,0,width,height), Ogre::HardwareBuffer::HBL_DISCARD));
            texture->getBuffer()->unlock();
            mBackgroundImage->setBackgroundImage(texture->getName(), false, false);
            */
        }

        setVisible(true);

        if (showWallpaper)
        {
            changeWallpaper();
        }

        MWBase::Environment::get().getWindowManager()->pushGuiMode(showWallpaper ? GM_LoadingWallpaper : GM_Loading);
    }

    void LoadingScreen::loadingOff()
    {
        //std::cout << "loading took " << mTimer.time_m() - mLoadingOnTime << std::endl;
        setVisible(false);

        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Loading);
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_LoadingWallpaper);
    }

    void LoadingScreen::changeWallpaper ()
    {
        if (!mSplashScreens.empty())
        {
            std::string const & randomSplash = mSplashScreens.at(Misc::Rng::rollDice(mSplashScreens.size()));

            // TODO: add option (filename pattern?) to use image aspect ratio instead of 4:3
            // we can't do this by default, because the Morrowind splash screens are 1024x1024, but should be displayed as 4:3
            bool stretch = Settings::Manager::getBool("stretch menu background", "GUI");
            mBackgroundImage->setBackgroundImage(randomSplash, true, stretch);
        }
    }

    void LoadingScreen::setProgressRange (size_t range)
    {
        mProgressBar->setScrollRange(range+1);
        mProgressBar->setScrollPosition(0);
        mProgressBar->setTrackSize(0);
        mProgress = 0;
    }

    void LoadingScreen::setProgress (size_t value)
    {
        if (value - mProgress < mProgressBar->getScrollRange()/100.f)
            return;
        mProgress = value;
        mProgressBar->setScrollPosition(0);
        mProgressBar->setTrackSize(static_cast<int>(value / (float)(mProgressBar->getScrollRange()) * mProgressBar->getLineSize()));
        draw();
    }

    void LoadingScreen::increaseProgress (size_t increase)
    {
        mProgressBar->setScrollPosition(0);
        size_t value = mProgress + increase;
        mProgress = value;
        mProgressBar->setTrackSize(static_cast<int>(value / (float)(mProgressBar->getScrollRange()) * mProgressBar->getLineSize()));
        draw();
    }

    void LoadingScreen::indicateProgress()
    {
        float time = (static_cast<int>(mTimer.time_m()) % 2001) / 1000.f;
        if (time > 1)
            time = (time-2)*-1;

        mProgressBar->setTrackSize(50);
        mProgressBar->setScrollPosition(static_cast<size_t>(time * (mProgressBar->getScrollRange() - 1)));
        draw();
    }

    void LoadingScreen::draw()
    {
        const float loadingScreenFps = 120.f;

        if (mTimer.time_m() > mLastRenderTime + (1.f/loadingScreenFps) * 1000.f)
        {
            bool showWallpaper = (MWBase::Environment::get().getStateManager()->getState()
                    == MWBase::StateManager::State_NoGame);

            if (showWallpaper && mTimer.time_m() > mLastWallpaperChangeTime + 5000*1)
            {
                mLastWallpaperChangeTime = mTimer.time_m();
                changeWallpaper();
            }

            // Turn off rendering except the GUI
            int oldUpdateMask = mViewer->getUpdateVisitor()->getTraversalMask();
            int oldCullMask = mViewer->getCamera()->getCullMask();
            mViewer->getUpdateVisitor()->setTraversalMask(MWRender::Mask_GUI);
            mViewer->getCamera()->setCullMask(MWRender::Mask_GUI);

            MWBase::Environment::get().getInputManager()->update(0, true, true);

            mViewer->frame();

            // resume 3d rendering
            mViewer->getUpdateVisitor()->setTraversalMask(oldUpdateMask);
            mViewer->getCamera()->setCullMask(oldCullMask);

            mLastRenderTime = mTimer.time_m();
        }
    }
}
