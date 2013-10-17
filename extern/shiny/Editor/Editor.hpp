#ifndef SH_EDITOR_H
#define SH_EDITOR_H

#if SHINY_BUILD_MATERIAL_EDITOR
class QApplication;

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

namespace boost
{
	class thread;
}

namespace sh
{
	class MainWindow;

	struct SynchronizationState
	{
		boost::mutex mUpdateMutex;
		boost::mutex mActionMutex;
		boost::mutex mQueryMutex;
	};

	class Editor
	{
	public:

		Editor();
		~Editor();

		void show();
		void update();


	private:
		bool mInitialized;

		MainWindow* mMainWindow;
		QApplication* mApplication;

		SynchronizationState mSync;

		boost::thread* mThread;

		void runThread();

		void processShowWindow();
	};

}

#else

// Dummy implementation, so that the user's code does not have to be polluted with #ifdefs
namespace sh
{

	class Editor
	{
	public:
		Editor() {}
		~Editor() {}
		void show() {}
		void update() {}

	};
}

#endif

#endif
