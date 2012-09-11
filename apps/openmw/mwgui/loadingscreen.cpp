#include "loadingscreen.hpp"

#include <OgreRenderWindow.h>
#include <OgreRoot.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"

namespace MWGui
{

    LoadingScreen::LoadingScreen(Ogre::SceneManager* sceneMgr, Ogre::RenderWindow* rw, MWBase::WindowManager& parWindowManager)
        : mSceneMgr(sceneMgr)
        , mWindow(rw)
        , WindowBase("openmw_loading_screen.layout", parWindowManager)
        , mLoadingOn(false)
        , mLastRenderTime(0.f)
    {
        getWidget(mLoadingText, "LoadingText");
    }

    LoadingScreen::~LoadingScreen()
    {
    }

    void LoadingScreen::setLoadingProgress (const std::string& stage, int depth, int current, int total)
    {
        if (!mLoadingOn)
            loadingOn();

        if (depth == 0)
        {
            mCurrentCellLoading = current;
            mTotalCellsLoading = total;

            mCurrentRefLoading = 0;

        }
        if (depth == 1)
        {
            mCurrentRefLoading = current;
            mTotalRefsLoading = total;
        }

        if (mTotalCellsLoading == 0)
        {
            loadingOff();
            return;
        }

        float refProgress;
        if (mTotalRefsLoading <= 1)
            refProgress = 0;
        else
            refProgress = float(mCurrentRefLoading) / float(mTotalRefsLoading-1);

        float progress = (float(mCurrentCellLoading)+refProgress) / float(mTotalCellsLoading);
        assert(progress <= 1 && progress >= 0);
        if (progress >= 1)
        {
            loadingOff();
            return;
        }

        mLoadingText->setCaption(stage + "... " + Ogre::StringConverter::toString(progress));

        static float loadingScreenFps = 40.f;

        //if (mTimer.getMilliseconds () > mLastRenderTime + (1.f/loadingScreenFps) * 1000.f)
        {
            mLastRenderTime = mTimer.getMilliseconds ();


            // Turn off rendering except the GUI
            mSceneMgr->clearSpecialCaseRenderQueues();
            // SCRQM_INCLUDE with RENDER_QUEUE_OVERLAY does not work.
            for (int i = 0; i < Ogre::RENDER_QUEUE_MAX; ++i)
            {
                if (i > 10 && i < 90)
                    mSceneMgr->addSpecialCaseRenderQueue(i);
            }
            mSceneMgr->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_EXCLUDE);

            // always update input before rendering something, otherwise mygui goes crazy when something was entered in the frame before
            // (e.g. when using "coc" console command, it would enter an infinite loop and crash due to overflow)
            MWBase::Environment::get().getInputManager()->update(0, true);

            mWindow->update();

            // resume 3d rendering
            mSceneMgr->clearSpecialCaseRenderQueues();
            mSceneMgr->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_EXCLUDE);
        }
    }

    void LoadingScreen::loadingOn()
    {
        setVisible(true);
        mLoadingOn = true;
    }


    void LoadingScreen::loadingOff()
    {
        setVisible(false);
        mLoadingOn = false;
    }
}
