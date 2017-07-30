#include <jni.h>
#include <android/vmstorage.hpp>

static JavaVM* javaVM;
static jclass cursorClass;
static jmethodID updateControls;

JavaVM* VMStorage::getVM() {
	return javaVM;
}

jclass VMStorage::getCursorClass() {
	return cursorClass;
}

jmethodID VMStorage::getUpdateControlsMethod() {
	return updateControls;
}

extern "C" {
JNIEXPORT void JNICALL Java_listener_NativeListener_initJavaVm(JNIEnv *env,
		jobject obj) {
	env->GetJavaVM(&javaVM);
	cursorClass = env->FindClass("cursor/CursorVisibility");
	updateControls = env->GetStaticMethodID(cursorClass,
			"updateScreenControlsState", "(Z)V");

}
}
