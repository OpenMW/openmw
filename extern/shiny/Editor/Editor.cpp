#include "Editor.hpp"


#include <QApplication>
#include <QTimer>

#include <boost/thread.hpp>

#include "../Main/Factory.hpp"

#include "MainWindow.hpp"

namespace sh
{

	Editor::Editor()
		: mMainWindow(NULL)
		, mApplication(NULL)
		, mInitialized(false)
		, mThread(NULL)
	{
	}

	Editor::~Editor()
	{
		if (mMainWindow)
			mMainWindow->mRequestExit = true;

		if (mThread)
			mThread->join();
		delete mThread;
	}

	void Editor::show()
	{
		if (!mInitialized)
		{
			mInitialized = true;

			mThread = new boost::thread(boost::bind(&Editor::runThread, this));
		}
		else
		{
			if (mMainWindow)
				mMainWindow->mRequestShowWindow = true;
		}
	}

	void Editor::runThread()
	{
		int argc = 0;
		char** argv = NULL;
		mApplication = new QApplication(argc, argv);
		mApplication->setQuitOnLastWindowClosed(false);
		mMainWindow = new MainWindow();
		mMainWindow->mSync = &mSync;
		mMainWindow->show();

		mApplication->exec();

		delete mApplication;
	}

	void Editor::update()
	{
		sh::Factory::getInstance().doMonitorShaderFiles();

		if (!mMainWindow)
			return;


		{
			boost::mutex::scoped_lock lock(mSync.mActionMutex);

			// execute pending actions
			while (mMainWindow->mActionQueue.size())
			{
				Action* action = mMainWindow->mActionQueue.front();
				action->execute();
				delete action;
				mMainWindow->mActionQueue.pop();
			}
		}
		{
			boost::mutex::scoped_lock lock(mSync.mQueryMutex);

			// execute pending queries
			for (std::vector<Query*>::iterator it = mMainWindow->mQueries.begin(); it != mMainWindow->mQueries.end(); ++it)
			{
				Query* query = *it;
				if (!query->mDone)
					query->execute();
			}
		}

		boost::mutex::scoped_lock lock2(mSync.mUpdateMutex);

		// update the list of materials
		mMainWindow->mState.mMaterialList.clear();
		sh::Factory::getInstance().listMaterials(mMainWindow->mState.mMaterialList);

		// update global settings
		mMainWindow->mState.mGlobalSettingsMap.clear();
		sh::Factory::getInstance().listGlobalSettings(mMainWindow->mState.mGlobalSettingsMap);

		// update configuration list
		mMainWindow->mState.mConfigurationList.clear();
		sh::Factory::getInstance().listConfigurationNames(mMainWindow->mState.mConfigurationList);

		// update shader list
		mMainWindow->mState.mShaderSets.clear();
		sh::Factory::getInstance().listShaderSets(mMainWindow->mState.mShaderSets);

		mMainWindow->mState.mErrors += sh::Factory::getInstance().getErrorLog();
	}

}
