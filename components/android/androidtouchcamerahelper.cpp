#include "androidtouchcamerahelper.hpp"
#include <jni.h>

JavaVM* javaVM;

void AndroidTouchCameraHelper::hideTouchCamera(bool needHideTouchCamera) {
	JNIEnv *env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    jclass nativeClass = env->FindClass("listener/NativeListener");
	jboolean needHideCamera = needHideTouchCamera ? JNI_TRUE : JNI_FALSE;
	jmethodID method = env->GetStaticMethodID(nativeClass,
			"hideTouchCamera", "(Z)V");
	env->CallStaticVoidMethod(nativeClass, method, needHideCamera);
}

extern "C" {

JNIEXPORT void JNICALL Java_listener_NativeListener_initJavaVm(JNIEnv *env,
		jobject obj) {
	env->GetJavaVM(&javaVM);
}

}
