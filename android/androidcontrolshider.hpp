#include <boost/thread/thread.hpp>
#include <iostream>
#include <boost/chrono.hpp>
#include <jni.h>
#include <apps/openmw/mwbase/environment.hpp>
#include <apps/openmw/mwgui/windowmanagerimp.hpp>
#ifndef COMPONENTS_ANDROID_ANDROIDCONTROLSHIDER_HPP_
#define COMPONENTS_ANDROID_ANDROIDCONTROLSHIDER_HPP_

class VMNativeListener {
public:
	VMNativeListener();
	~VMNativeListener();
	void updateControlsState(bool cursorVisibility);
private:
	JNIEnv *myEnv;
	bool _detach = false;
};

class AndroidControlsHider {
public:
	void startBackdroundTask();
	bool needStopBackgroundTask = false;

private:
	VMNativeListener *vmListener;
	bool cursorVisible = false;
	static void sleep();
	static void runnable();
	boost::thread thr;
	void updateCursorVisibility();
};

#endif /* COMPONENTS_ANDROID_ANDROIDCONTROLSHIDER_HPP_ */
