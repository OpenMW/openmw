#include "android_commandLine.h"
#include "string.h"

 const char *argvData[15];
 int argcData;

JNIEXPORT void JNICALL Java_ui_activity_GameActivity_commandLine(JNIEnv *env,
		jobject obj, jint argc, jobjectArray stringArray) {
	jboolean iscopy;
	argcData = (int) argc;
	argvData[0]="openmw";
	for (int i = 0; i < argcData; i++) {
		jstring string = (jstring) (*env).GetObjectArrayElement(stringArray, i);
		argvData[i+1] = (env)->GetStringUTFChars(string, &iscopy);
	}

}

