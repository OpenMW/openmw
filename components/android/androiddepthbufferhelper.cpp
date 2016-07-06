#include <components/android/androiddepthbufferhelper.hpp>
#include <jni.h>

JavaVM* javaVM;

int AndroidDepthBufferHelper::getGlDepthBufferSize() {
	JNIEnv *env;
	javaVM->GetEnv((void**) &env, JNI_VERSION_1_6);
	jclass nativeClass = env->FindClass("listener/NativeListener");
	jmethodID method = env->GetStaticMethodID(nativeClass, "getGlDepthBufferSize",
			"()I");
	jint depth = env->CallStaticIntMethod(nativeClass, method);
	return (int) depth;
}

extern "C" {
JNIEXPORT void JNICALL Java_listener_NativeListener_initJavaVm(JNIEnv *env,
		jobject obj) {
	env->GetJavaVM(&javaVM);
}
}
