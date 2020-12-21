#include "loadingscreen.hpp"

#include <array>
#include <condition_variable>

#include <osgViewer/Viewer>

#include <osg/Texture2D>
#include <osg/Version>

#include <MyGUI_RenderManager.h>
#include <MyGUI_ScrollBar.h>
#include <MyGUI_Gui.h>
#include <MyGUI_TextBox.h>

#include <components/misc/rng.hpp>
#include <components/debug/debuglog.hpp>
#include <components/myguiplatform/myguitexture.hpp>
#include <components/settings/settings.hpp>
#include <components/vfs/manager.hpp>
#include <components/resource/resourcesystem.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/inputmanager.hpp"

#include "../mwrender/vismask.hpp"

#include "backgroundimage.hpp"

namespace MWGui
{

    LoadingScreen::LoadingScreen(Resource::ResourceSystem* resourceSystem, osgViewer::Viewer* viewer)
        : WindowBase("openmw_loading_screen.layout")
        , mResourceSystem(resourceSystem)
        , mViewer(viewer)
        , mTargetFrameRate(120.0)
        , mLastWallpaperChangeTime(0.0)
        , mLastRenderTime(0.0)
        , mLoadingOnTime(0.0)
        , mImportantLabel(false)
        , mVisible(false)
        , mNestedLoadingCount(0)
        , mProgress(0)
        , mShowWallpaper(true)
        , mOldCallback(nullptr)
        , mHasCallback(false)
    {
        mMainWidget->setSize(MyGUI::RenderManager::getInstance().getViewSize());

        getWidget(mLoadingText, "LoadingText");
        getWidget(mProgressBar, "ProgressBar");
        getWidget(mLoadingBox, "LoadingBox");

        mProgressBar->setScrollViewPage(1);

        mBackgroundImage = MyGUI::Gui::getInstance().createWidgetReal<BackgroundImage>("ImageBox", 0,0,1,1,
            MyGUI::Align::Stretch, "Menu");
        mSceneImage = MyGUI::Gui::getInstance().createWidgetReal<BackgroundImage>("ImageBox", 0,0,1,1,
            MyGUI::Align::Stretch, "Scene");

        findSplashScreens();
    }

    LoadingScreen::~LoadingScreen()
    {
    }

    void LoadingScreen::findSplashScreens()
    {
        const std::map<std::string, VFS::File*>& index = mResourceSystem->getVFS()->getIndex();
        std::string pattern = "Splash/";
        mResourceSystem->getVFS()->normalizeFilename(pattern);

        /* priority given to the left */
        const std::array<std::string, 7> supported_extensions {{".tga", ".dds", ".ktx", ".png", ".bmp", ".jpeg", ".jpg"}};

        auto found = index.lower_bound(pattern);
        while (found != index.end())
        {
            const std::string& name = found->first;
            if (name.size() >= pattern.size() && name.substr(0, pattern.size()) == pattern)
            {
                size_t pos = name.find_last_of('.');
                if (pos != std::string::npos)
                {
                    for(auto const& extension: supported_extensions)
                    {
                        if (name.compare(pos, name.size() - pos, extension) == 0)
                        {
                            mSplashScreens.push_back(found->first);
                            break;  /* based on priority */
                        }
                    }
                }
            }
            else
                break;
            ++found;
        }
        if (mSplashScreens.empty())
            Log(Debug::Warning) << "Warning: no splash screens found!";
    }

    void LoadingScreen::setLabel(const std::string &label, bool important, bool center)
    {
        mImportantLabel = important;

        mLoadingText->setCaptionWithReplacing(label);
        int padding = mLoadingBox->getWidth() - mLoadingText->getWidth();
        MyGUI::IntSize size(mLoadingText->getTextSize().width+padding, mLoadingBox->getHeight());
        size.width = std::max(300, size.width);
        mLoadingBox->setSize(size);

        if (center)
            mLoadingBox->setPosition(mMainWidget->getWidth()/2 - mLoadingBox->getWidth()/2, mMainWidget->getHeight()/2 - mLoadingBox->getHeight()/2);
        else
            mLoadingBox->setPosition(mMainWidget->getWidth()/2 - mLoadingBox->getWidth()/2, mMainWidget->getHeight() - mLoadingBox->getHeight() - 8);
    }

    void LoadingScreen::setVisible(bool visible)
    {
        WindowBase::setVisible(visible);
        mBackgroundImage->setVisible(visible);
        mSceneImage->setVisible(visible);
    }

    double LoadingScreen::getTargetFrameRate() const
    {
        double frameRateLimit = MWBase::Environment::get().getFrameRateLimit();
        if (frameRateLimit > 0)
            return std::min(frameRateLimit, mTargetFrameRate);
        else
            return mTargetFrameRate;
    }

    class CopyFramebufferToTextureCallback : public osg::Camera::DrawCallback
    {
    public:
        CopyFramebufferToTextureCallback(osg::Texture2D* texture)
            : mOneshot(true)
            , mTexture(texture)
        {
        }

        void operator () (osg::RenderInfo& renderInfo) const override
        {
            {
                std::unique_lock<std::mutex> lock(mMutex);
                mOneshot = false;
            }
            mSignal.notify_all();

            int w = renderInfo.getCurrentCamera()->getViewport()->width();
            int h = renderInfo.getCurrentCamera()->getViewport()->height();
            mTexture->copyTexImage2D(*renderInfo.getState(), 0, 0, w, h);

            {
                std::unique_lock<std::mutex> lock(mMutex);
                mOneshot = false;
            }
            mSignal.notify_all();
        }

        void wait()
        {
            std::unique_lock<std::mutex> lock(mMutex);
            while (mOneshot)
                mSignal.wait(lock);
        }

        void waitUntilInvoked()
        {
            std::unique_lock<std::mutex> lock(mMutex);
            while (mOneshot)
                mSignal.wait(lock);
        }

        void reset()
        {
            mOneshot = true;
        }

    private:
        mutable bool mOneshot;
        mutable std::mutex mMutex;
        mutable std::condition_variable mSignal;
        osg::ref_ptr<osg::Texture2D> mTexture;
    };

    class DontComputeBoundCallback : public osg::Node::ComputeBoundingSphereCallback
    {
    public:
        osg::BoundingSphere computeBound(const osg::Node&) const override { return osg::BoundingSphere(); }
    };

    void LoadingScreen::loadingOn(bool visible)
    {
        // Early-out if already on
        if (mNestedLoadingCount++ > 0 && mMainWidget->getVisible())
            return;

        mLoadingOnTime = mTimer.time_m();

        // Assign dummy bounding sphere callback to avoid the bounding sphere of the entire scene being recomputed after each frame of loading
        // We are already using node masks to avoid the scene from being updated/rendered, but node masks don't work for computeBound()
        mViewer->getSceneData()->setComputeBoundingSphereCallback(new DontComputeBoundCallback);

        if (const osgUtil::IncrementalCompileOperation* ico = mViewer->getIncrementalCompileOperation()) {
            mOldIcoMin = ico->getMinimumTimeAvailableForGLCompileAndDeletePerFrame();
            mOldIcoMax = ico->getMaximumNumOfObjectsToCompilePerFrame();
        }

        mVisible = visible;
        mLoadingBox->setVisible(mVisible);
        setVisible(true);

        if (!mVisible)
        {
            mShowWallpaper = false;
            draw();
            return;
        }

        mShowWallpaper = MWBase::Environment::get().getStateManager()->getState() == MWBase::StateManager::State_NoGame;

        if (mShowWallpaper)
        {
            changeWallpaper();
        }

        MWBase::Environment::get().getWindowManager()->pushGuiMode(mShowWallpaper ? GM_LoadingWallpaper : GM_Loading);
    }

    void LoadingScreen::loadingOff()
    {
        if (--mNestedLoadingCount > 0)
            return;
        mLoadingBox->setVisible(true);   // restore

        if (mLastRenderTime < mLoadingOnTime)
        {
            // the loading was so fast that we didn't show loading screen at all
            // we may still want to show the label if the caller requested it
            if (mImportantLabel)
            {
                MWBase::Environment::get().getWindowManager()->messageBox(mLoadingText->getCaption());
                mImportantLabel = false;
            }
        }
        else
            mImportantLabel = false; // label was already shown on loading screen

        mViewer->getSceneData()->setComputeBoundingSphereCallback(nullptr);
        mViewer->getSceneData()->dirtyBound();

        //std::cout << "loading took " << mTimer.time_m() - mLoadingOnTime << std::endl;
        setVisible(false);

        if (osgUtil::IncrementalCompileOperation* ico = mViewer->getIncrementalCompileOperation())
        {
            ico->setMinimumTimeAvailableForGLCompileAndDeletePerFrame(mOldIcoMin);
            ico->setMaximumNumOfObjectsToCompilePerFrame(mOldIcoMax);
        }

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
            mBackgroundImage->setVisible(true);
            mBackgroundImage->setBackgroundImage(randomSplash, true, stretch);
        }
        mSceneImage->setBackgroundImage("");
        mSceneImage->setVisible(false);
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
        // skip expensive update if there isn't enough visible progress
        if (mProgressBar->getWidth() <= 0 || value - mProgress < mProgressBar->getScrollRange()/mProgressBar->getWidth())
            return;
        value = std::min(value, mProgressBar->getScrollRange()-1);
        mProgress = value;
        mProgressBar->setScrollPosition(0);
        mProgressBar->setTrackSize(static_cast<int>(value / (float)(mProgressBar->getScrollRange()) * mProgressBar->getLineSize()));
        draw();
    }

    void LoadingScreen::increaseProgress (size_t increase)
    {
        mProgressBar->setScrollPosition(0);
        size_t value = mProgress + increase;
        value = std::min(value, mProgressBar->getScrollRange()-1);
        mProgress = value;
        mProgressBar->setTrackSize(static_cast<int>(value / (float)(mProgressBar->getScrollRange()) * mProgressBar->getLineSize()));
        draw();
    }

    bool LoadingScreen::needToDrawLoadingScreen()
    {
        if ( mTimer.time_m() <= mLastRenderTime + (1.0/getTargetFrameRate()) * 1000.0)
            return false;

        // the minimal delay before a loading screen shows
        const float initialDelay = 0.05;

        bool alreadyShown = (mLastRenderTime > mLoadingOnTime);
        float diff = (mTimer.time_m() - mLoadingOnTime);

        if (!alreadyShown)
        {
            // bump the delay by the current progress - i.e. if during the initial delay the loading
            // has almost finished, no point showing the loading screen now
            diff -= mProgress / static_cast<float>(mProgressBar->getScrollRange()) * 100.f;
        }

        if (!mShowWallpaper && diff < initialDelay*1000)
            return false;
        return true;
    }

    void LoadingScreen::setupCopyFramebufferToTextureCallback()
    {
        // Copy the current framebuffer onto a texture and display that texture as the background image
        // Note, we could also set the camera to disable clearing and have the background image transparent,
        // but then we get shaking effects on buffer swaps.

        if (!mTexture)
        {
            mTexture = new osg::Texture2D;
            mTexture->setInternalFormat(GL_RGB);
            mTexture->setResizeNonPowerOfTwoHint(false);
        }

        if (!mGuiTexture.get())
        {
            mGuiTexture.reset(new osgMyGUI::OSGTexture(mTexture));
        }

        if (!mCopyFramebufferToTextureCallback)
        {
            mCopyFramebufferToTextureCallback = new CopyFramebufferToTextureCallback(mTexture);
        }

#if OSG_VERSION_GREATER_OR_EQUAL(3, 5, 10)
        mViewer->getCamera()->addInitialDrawCallback(mCopyFramebufferToTextureCallback);
#else
        // TODO: Remove once we officially end support for OSG versions pre 3.5.10
        mOldCallback = mViewer->getCamera()->getInitialDrawCallback();
        mViewer->getCamera()->setInitialDrawCallback(mCopyFramebufferToTextureCallback);
#endif
        mCopyFramebufferToTextureCallback->reset();
        mHasCallback = true;

        mBackgroundImage->setBackgroundImage("");
        mBackgroundImage->setVisible(false);

        mSceneImage->setRenderItemTexture(mGuiTexture.get());
        mSceneImage->getSubWidgetMain()->_setUVSet(MyGUI::FloatRect(0.f, 0.f, 1.f, 1.f));
        mSceneImage->setVisible(true);
    }

    void LoadingScreen::draw()
    {
        if (mVisible && !needToDrawLoadingScreen())
            return;

        if (mShowWallpaper && mTimer.time_m() > mLastWallpaperChangeTime + 5000*1)
        {
            mLastWallpaperChangeTime = mTimer.time_m();
            changeWallpaper();
        }

        if (!mShowWallpaper && mLastRenderTime < mLoadingOnTime)
        {
            setupCopyFramebufferToTextureCallback();
        }

        MWBase::Environment::get().getInputManager()->update(0, true, true);

        mResourceSystem->reportStats(mViewer->getFrameStamp()->getFrameNumber(), mViewer->getViewerStats());
        if (osgUtil::IncrementalCompileOperation* ico = mViewer->getIncrementalCompileOperation())
        {
            ico->setMinimumTimeAvailableForGLCompileAndDeletePerFrame(1.f/getTargetFrameRate());
            ico->setMaximumNumOfObjectsToCompilePerFrame(1000);
        }

        // at the time this function is called we are in the middle of a frame,
        // so out of order calls are necessary to get a correct frameNumber for the next frame.
        // refer to the advance() and frame() order in Engine::go()
        mViewer->eventTraversal();
        mViewer->updateTraversal();
        mViewer->renderingTraversals();
        mViewer->advance(mViewer->getFrameStamp()->getSimulationTime());

        if (mHasCallback)
        {
            mCopyFramebufferToTextureCallback->waitUntilInvoked();

            // Note that we are removing the callback before the draw thread has returned from it.
            // This is OK as we are retaining the ref_ptr.
#if OSG_VERSION_GREATER_OR_EQUAL(3, 5, 10)
            mViewer->getCamera()->removeInitialDrawCallback(mCopyFramebufferToTextureCallback);
#else
            // TODO: Remove once we officially end support for OSG versions pre 3.5.10
            mViewer->getCamera()->setInitialDrawCallback(mOldCallback);
#endif
            mHasCallback = false;
        }

        mLastRenderTime = mTimer.time_m();
    }

}
