#include "loadingscreen.hpp"

#include <osgViewer/Viewer>

#include <osg/Texture2D>

#include <MyGUI_RenderManager.h>
#include <MyGUI_ScrollBar.h>
#include <MyGUI_Gui.h>
#include <MyGUI_TextBox.h>

#include <components/misc/rng.hpp>

#include <components/myguiplatform/myguitexture.hpp>

#include <components/settings/settings.hpp>
#include <components/vfs/manager.hpp>

#include "../mwbase/environment.hpp"
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
        , mTargetFrameRate(120.0)
        , mLastWallpaperChangeTime(0.0)
        , mLastRenderTime(0.0)
        , mLoadingOnTime(0.0)
        , mImportantLabel(false)
        , mProgress(0)
        , mShowWallpaper(true)
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

    LoadingScreen::~LoadingScreen()
    {
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

    void LoadingScreen::setLabel(const std::string &label, bool important)
    {
        mImportantLabel = important;

        mLoadingText->setCaptionWithReplacing(label);
        int padding = mLoadingBox->getWidth() - mLoadingText->getWidth();
        MyGUI::IntSize size(mLoadingText->getTextSize().width+padding, mLoadingBox->getHeight());
        size.width = std::max(300, size.width);
        mLoadingBox->setSize(size);
        mLoadingBox->setPosition(mMainWidget->getWidth()/2 - mLoadingBox->getWidth()/2, mLoadingBox->getTop());
    }

    void LoadingScreen::setVisible(bool visible)
    {
        WindowBase::setVisible(visible);
        mBackgroundImage->setVisible(visible);
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
            : mTexture(texture)
        {
        }

        virtual void operator () (osg::RenderInfo& renderInfo) const
        {
            int w = renderInfo.getCurrentCamera()->getViewport()->width();
            int h = renderInfo.getCurrentCamera()->getViewport()->height();
            mTexture->copyTexImage2D(*renderInfo.getState(), 0, 0, w, h);

            // Callback removes itself when done
            if (renderInfo.getCurrentCamera())
                renderInfo.getCurrentCamera()->setInitialDrawCallback(NULL);
        }

    private:
        osg::ref_ptr<osg::Texture2D> mTexture;
    };

    class DontComputeBoundCallback : public osg::Node::ComputeBoundingSphereCallback
    {
    public:
        virtual osg::BoundingSphere computeBound(const osg::Node&) const { return osg::BoundingSphere(); }
    };

    void LoadingScreen::loadingOn()
    {
        mLoadingOnTime = mTimer.time_m();
        // Early-out if already on
        if (mMainWidget->getVisible())
            return;

        if (mViewer->getIncrementalCompileOperation())
        {
            mViewer->getIncrementalCompileOperation()->setMaximumNumOfObjectsToCompilePerFrame(100);
            mViewer->getIncrementalCompileOperation()->setTargetFrameRate(getTargetFrameRate());
        }

        // Assign dummy bounding sphere callback to avoid the bounding sphere of the entire scene being recomputed after each frame of loading
        // We are already using node masks to avoid the scene from being updated/rendered, but node masks don't work for computeBound()
        mViewer->getSceneData()->setComputeBoundingSphereCallback(new DontComputeBoundCallback);

        mShowWallpaper = (MWBase::Environment::get().getStateManager()->getState()
                == MWBase::StateManager::State_NoGame);

        setVisible(true);

        if (mShowWallpaper)
        {
            changeWallpaper();
        }

        MWBase::Environment::get().getWindowManager()->pushGuiMode(mShowWallpaper ? GM_LoadingWallpaper : GM_Loading);
    }

    void LoadingScreen::loadingOff()
    {
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

        mViewer->getSceneData()->setComputeBoundingSphereCallback(NULL);
        mViewer->getSceneData()->dirtyBound();

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

        mViewer->getCamera()->setInitialDrawCallback(new CopyFramebufferToTextureCallback(mTexture));

        mBackgroundImage->setBackgroundImage("");

        mBackgroundImage->setRenderItemTexture(mGuiTexture.get());
        mBackgroundImage->getSubWidgetMain()->_setUVSet(MyGUI::FloatRect(0.f, 0.f, 1.f, 1.f));
    }

    void LoadingScreen::draw()
    {
        if (!needToDrawLoadingScreen())
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

        // Turn off rendering except the GUI
        int oldUpdateMask = mViewer->getUpdateVisitor()->getTraversalMask();
        int oldCullMask = mViewer->getCamera()->getCullMask();
        mViewer->getUpdateVisitor()->setTraversalMask(MWRender::Mask_GUI|MWRender::Mask_PreCompile);
        mViewer->getCamera()->setCullMask(MWRender::Mask_GUI|MWRender::Mask_PreCompile);

        MWBase::Environment::get().getInputManager()->update(0, true, true);

        //osg::Timer timer;
        // at the time this function is called we are in the middle of a frame,
        // so out of order calls are necessary to get a correct frameNumber for the next frame.
        // refer to the advance() and frame() order in Engine::go()
        mViewer->eventTraversal();
        mViewer->updateTraversal();
        mViewer->renderingTraversals();
        mViewer->advance(mViewer->getFrameStamp()->getSimulationTime());
        //std::cout << "frame took " << timer.time_m() << std::endl;

        //if (mViewer->getIncrementalCompileOperation())
            //std::cout << "num to compile " << mViewer->getIncrementalCompileOperation()->getToCompile().size() << std::endl;

        // resume 3d rendering
        mViewer->getUpdateVisitor()->setTraversalMask(oldUpdateMask);
        mViewer->getCamera()->setCullMask(oldCullMask);

        mLastRenderTime = mTimer.time_m();
    }

}
