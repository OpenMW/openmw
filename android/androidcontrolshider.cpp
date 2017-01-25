#include <jni.h>
#include <android/vmstorage.hpp>
#include <android/androidcontrolshider.hpp>

#define THREAD_SLEEP_TIME_SECONDS 1
static AndroidControlsHider *controlsHelper;

void AndroidControlsHider::sleep() {
	boost::this_thread::sleep_for(boost::chrono::seconds {THREAD_SLEEP_TIME_SECONDS });
}

void AndroidControlsHider::runnable() {
	while (!controlsHelper->needStopBackgroundTask) {
        sleep();
		controlsHelper->updateCursorVisibility();
	}
}

void AndroidControlsHider::updateCursorVisibility() {
    MWBase::WindowManager *mwWindow = MWBase::Environment::get().getWindowManager();
	if (mwWindow && this->cursorVisible != mwWindow->getCursorVisible()) {
		this->cursorVisible = mwWindow->getCursorVisible();
		if (!this->vmListener) {
			this->vmListener = new VMNativeListener();
		}
		this->vmListener->updateControlsState(cursorVisible);
	}

}

void AndroidControlsHider::startBackdroundTask() {
	this->thr = boost::thread(runnable);
}

VMNativeListener::VMNativeListener() {
	if (VMStorage::getVM()->AttachCurrentThread((JNIEnv **) &myEnv, NULL) ==  0){
		this->_detach = true;
	}
}

VMNativeListener::~VMNativeListener() {
	if (this->_detach) {
		VMStorage::getVM()->DetachCurrentThread();
		this->myEnv = NULL;
	}
}

void VMNativeListener::updateControlsState(bool cursorVisibility) {
	if (this->_detach){
		this->myEnv->CallStaticVoidMethod(VMStorage::getCursorClass(),VMStorage::getUpdateControlsMethod(), cursorVisibility);
	}
}

extern "C" {
JNIEXPORT void JNICALL Java_cursor_CursorVisibility_InitBackgroundTask(JNIEnv *env, jobject obj) {
	if (!controlsHelper) {
		controlsHelper = new AndroidControlsHider();
		controlsHelper->startBackdroundTask();
	}
}
}

extern "C" {
JNIEXPORT void JNICALL Java_cursor_CursorVisibility_StopBackgroundTask(JNIEnv *env, jobject obj) {
	if (controlsHelper) {
		controlsHelper->needStopBackgroundTask = true;
	}
}
}

