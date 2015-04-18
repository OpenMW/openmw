#include "loadingscreen.hpp"

#include <OgreRenderWindow.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <OgreRectangle2D.h>
#include <OgreSceneNode.h>
#include <OgreTextureManager.h>
#include <OgreViewport.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreSceneManager.h>

#include <MyGUI_RenderManager.h>
#include <MyGUI_ScrollBar.h>
#include <MyGUI_Gui.h>
#include <MyGUI_TextBox.h>

#include <openengine/misc/rng.hpp>

#include <components/settings/settings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/statemanager.hpp"

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/inputmanager.hpp"

#include "backgroundimage.hpp"

namespace MWGui
{

    LoadingScreen::LoadingScreen(Ogre::SceneManager* sceneMgr, Ogre::RenderWindow* rw)
        : mSceneMgr(sceneMgr)
        , mWindow(rw)
        , WindowBase("openmw_loading_screen.layout")
        , mLastRenderTime(0)
        , mLastWallpaperChangeTime(0)
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
        // Early-out if already on
        if (mMainWidget->getVisible())
            return;

        // Temporarily turn off VSync, we want to do actual loading rather than waiting for the screen to sync.
        // Threaded loading would be even better, of course - especially because some drivers force VSync to on and we can't change it.
        mVSyncWasEnabled = mWindow->isVSyncEnabled();
        mWindow->setVSyncEnabled(false);

        bool showWallpaper = (MWBase::Environment::get().getStateManager()->getState()
                == MWBase::StateManager::State_NoGame);


        if (!showWallpaper)
        {
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
        // Re-enable vsync now.
        mWindow->setVSyncEnabled(mVSyncWasEnabled);

        setVisible(false);

        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Loading);
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_LoadingWallpaper);
    }

    void LoadingScreen::changeWallpaper ()
    {
        if (mResources.empty())
        {
            Ogre::StringVector groups = Ogre::ResourceGroupManager::getSingleton().getResourceGroups ();
            for (Ogre::StringVector::iterator it = groups.begin(); it != groups.end(); ++it)
            {
                Ogre::StringVectorPtr resourcesInThisGroup = Ogre::ResourceGroupManager::getSingleton ().findResourceNames (*it, "Splash/*.tga");
                mResources.insert(mResources.end(), resourcesInThisGroup->begin(), resourcesInThisGroup->end());
            }
        }

        if (!mResources.empty())
        {
            std::string const & randomSplash = mResources.at(OEngine::Misc::Rng::rollDice(mResources.size()));

            Ogre::TextureManager::getSingleton ().load (randomSplash, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

            // TODO: add option (filename pattern?) to use image aspect ratio instead of 4:3
            // we can't do this by default, because the Morrowind splash screens are 1024x1024, but should be displayed as 4:3
            bool stretch = Settings::Manager::getBool("stretch menu background", "GUI");
            mBackgroundImage->setBackgroundImage(randomSplash, true, stretch);
        }
        else
            std::cerr << "No loading screens found!" << std::endl;
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
        float time = (mTimer.getMilliseconds() % 2001) / 1000.f;
        if (time > 1)
            time = (time-2)*-1;

        mProgressBar->setTrackSize(50);
        mProgressBar->setScrollPosition(static_cast<size_t>(time * (mProgressBar->getScrollRange() - 1)));
        draw();
    }

    void LoadingScreen::draw()
    {
        const float loadingScreenFps = 20.f;

        if (mTimer.getMilliseconds () > mLastRenderTime + (1.f/loadingScreenFps) * 1000.f)
        {
            mLastRenderTime = mTimer.getMilliseconds ();

            bool showWallpaper = (MWBase::Environment::get().getStateManager()->getState()
                    == MWBase::StateManager::State_NoGame);

            if (showWallpaper && mTimer.getMilliseconds () > mLastWallpaperChangeTime + 5000*1)
            {
                mLastWallpaperChangeTime = mTimer.getMilliseconds ();
                changeWallpaper();
            }

            // Turn off rendering except the GUI
            mSceneMgr->clearSpecialCaseRenderQueues();
            // SCRQM_INCLUDE with RENDER_QUEUE_OVERLAY does not work.
            for (int i = 0; i < Ogre::RENDER_QUEUE_MAX; ++i)
            {
                if (i > 0 && i < 96)
                    mSceneMgr->addSpecialCaseRenderQueue(i);
            }
            mSceneMgr->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_EXCLUDE);

            MWBase::Environment::get().getInputManager()->update(0, true, true);

            // First, swap buffers from last draw, then, queue an update of the
            // window contents, but don't swap buffers (which would have
            // caused a sync / flush and would be expensive).
            // We're doing this so we can do some actual loading while the GPU is busy with the render.
            // This means the render is lagging a frame behind, but this is hardly noticable.
            mWindow->swapBuffers();

            mWindow->update(false);

            // resume 3d rendering
            mSceneMgr->clearSpecialCaseRenderQueues();
            mSceneMgr->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_EXCLUDE);
        }
    }
}
