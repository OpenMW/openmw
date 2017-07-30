#include "android_commandLine.h"
#include "string.h"

const char **argvData;
int argcData;

extern "C" void releaseArgv();

void releaseArgv() {
    delete[] argvData;
}

JNIEXPORT void JNICALL Java_ui_activity_GameActivity_commandLine(JNIEnv *env,
    jobject obj, jint argc, jobjectArray stringArray) {
    jboolean iscopy;
    argcData = (int) argc;
    argvData = new const char *[argcData + 1];
    argvData[0] = "openmw";
    for (int i = 1; i < argcData + 1; i++) {
        jstring string = (jstring) (env)->GetObjectArrayElement(stringArray,
                i - 1);
        argvData[i] = (env)->GetStringUTFChars(string, &iscopy);
        (env)->DeleteLocalRef(string);
    }
    (env)->DeleteLocalRef(stringArray);
}

