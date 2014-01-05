#include "loadingscreen.hpp"

#include <OgreRenderWindow.h>

#include <openengine/ogre/fader.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/inputmanager.hpp"

namespace MWGui
{

    LoadingScreen::LoadingScreen(Ogre::SceneManager* sceneMgr, Ogre::RenderWindow* rw)
        : mSceneMgr(sceneMgr)
        , mWindow(rw)
        , WindowBase("openmw_loading_screen.layout")
        , mLastRenderTime(0.f)
        , mLastWallpaperChangeTime(0.f)
        , mFirstLoad(true)
        , mProgress(0)
        , mVSyncWasEnabled(false)
    {
        getWidget(mLoadingText, "LoadingText");
        getWidget(mProgressBar, "ProgressBar");
        getWidget(mBackgroundImage, "BackgroundImage");

        mProgressBar->setScrollViewPage(1);

        mBackgroundMaterial = Ogre::MaterialManager::getSingleton().create("BackgroundMaterial", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        mBackgroundMaterial->getTechnique(0)->getPass(0)->setLightingEnabled(false);
        mBackgroundMaterial->getTechnique(0)->getPass(0)->setDepthCheckEnabled(false);
        mBackgroundMaterial->getTechnique(0)->getPass(0)->createTextureUnitState("");

        mRectangle = new Ogre::Rectangle2D(true);
        mRectangle->setCorners(-1.0, 1.0, 1.0, -1.0);
        mRectangle->setMaterial("BackgroundMaterial");
        // Render the background before everything else
        mRectangle->setRenderQueueGroup(Ogre::RENDER_QUEUE_OVERLAY-1);
        // Use infinite AAB to always stay visible
        Ogre::AxisAlignedBox aabInf;
        aabInf.setInfinite();
        mRectangle->setBoundingBox(aabInf);
        // Attach background to the scene
        Ogre::SceneNode* node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
        node->attachObject(mRectangle);
        mRectangle->setVisible(false);
    }

    void LoadingScreen::setLabel(const std::string &label)
    {
        mLoadingText->setCaptionWithReplacing(label);
    }

    LoadingScreen::~LoadingScreen()
    {
        delete mRectangle;
    }

    void LoadingScreen::onResChange(int w, int h)
    {
        setCoord(0,0,w,h);
    }

    void LoadingScreen::loadingOn()
    {
        // Temporarily turn off VSync, we want to do actual loading rather than waiting for the screen to sync.
        // Threaded loading would be even better, of course - especially because some drivers force VSync to on and we can't change it.
        // In Ogre 1.8, the swapBuffers argument is useless and setVSyncEnabled is bugged with GLX, nothing we can do :/
        mVSyncWasEnabled = mWindow->isVSyncEnabled();
        #if OGRE_VERSION >= (1 << 16 | 9 << 8 | 0)
        mWindow->setVSyncEnabled(false);
        #endif

        setVisible(true);

        if (mFirstLoad)
        {
            changeWallpaper();
        }
        else
        {
            mBackgroundImage->setImageTexture("");
        }

        MWBase::Environment::get().getWindowManager()->pushGuiMode(mFirstLoad ? GM_LoadingWallpaper : GM_Loading);
    }

    void LoadingScreen::loadingOff()
    {
        // Re-enable vsync now.
        // In Ogre 1.8, the swapBuffers argument is useless and setVSyncEnabled is bugged with GLX, nothing we can do :/
        #if OGRE_VERSION >= (1 << 16 | 9 << 8 | 0)
        mWindow->setVSyncEnabled(mVSyncWasEnabled);
        #endif

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
                Ogre::StringVectorPtr resourcesInThisGroup = Ogre::ResourceGroupManager::getSingleton ().findResourceNames (*it, "Splash_*.tga");
                mResources.insert(mResources.end(), resourcesInThisGroup->begin(), resourcesInThisGroup->end());
            }
        }

        if (!mResources.empty())
        {
            std::string const & randomSplash = mResources.at (rand() % mResources.size());

            Ogre::TexturePtr tex = Ogre::TextureManager::getSingleton ().load (randomSplash, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

            mBackgroundImage->setImageTexture (randomSplash);
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
        assert(value < mProgressBar->getScrollRange());
        if (value - mProgress < mProgressBar->getScrollRange()/100.f)
            return;
        mProgress = value;
        mProgressBar->setScrollPosition(0);
        mProgressBar->setTrackSize(value / (float)(mProgressBar->getScrollRange()) * mProgressBar->getLineSize());
        draw();
    }

    void LoadingScreen::increaseProgress (size_t increase)
    {
        mProgressBar->setScrollPosition(0);
        size_t value = mProgress + increase;
        mProgress = value;
        assert(mProgress < mProgressBar->getScrollRange());
        mProgressBar->setTrackSize(value / (float)(mProgressBar->getScrollRange()) * mProgressBar->getLineSize());
        draw();
    }

    void LoadingScreen::indicateProgress()
    {
        float time = (mTimer.getMilliseconds() % 2001) / 1000.f;
        if (time > 1)
            time = (time-2)*-1;

        mProgressBar->setTrackSize(50);
        mProgressBar->setScrollPosition(time * (mProgressBar->getScrollRange()-1));
        draw();
    }

    void LoadingScreen::removeWallpaper()
    {
        mFirstLoad = false;
    }

    void LoadingScreen::draw()
    {
        const float loadingScreenFps = 20.f;

        if (mTimer.getMilliseconds () > mLastRenderTime + (1.f/loadingScreenFps) * 1000.f)
        {
            mLastRenderTime = mTimer.getMilliseconds ();

            if (mFirstLoad && mTimer.getMilliseconds () > mLastWallpaperChangeTime + 5000*1)
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

            MWBase::Environment::get().getInputManager()->update(0, true);

            mWindow->getViewport(0)->setClearEveryFrame(false);

            // First, swap buffers from last draw, then, queue an update of the
            // window contents, but don't swap buffers (which would have
            // caused a sync / flush and would be expensive).
            // We're doing this so we can do some actual loading while the GPU is busy with the render.
            // This means the render is lagging a frame behind, but this is hardly noticable.
            mWindow->swapBuffers();

            mWindow->update(false);

            mWindow->getViewport(0)->setClearEveryFrame(true);


            mRectangle->setVisible(false);

            // resume 3d rendering
            mSceneMgr->clearSpecialCaseRenderQueues();
            mSceneMgr->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_EXCLUDE);
        }
    }
}
