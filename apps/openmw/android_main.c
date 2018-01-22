int stderr = 0; // Hack: fix linker error

#ifdef __ANDROID__
#include "SDL_main.h"

/*******************************************************************************
 Functions called by JNI
 *******************************************************************************/
#include <jni.h>

/* Called before  to initialize JNI bindings  */

extern void SDL_Android_Init(JNIEnv* env, jclass cls);
extern int argcData;
extern const char **argvData;
void releaseArgv();

int Java_org_libsdl_app_SDLActivity_nativeInit(JNIEnv* env, jclass cls,
        jobject obj) {

    setenv("OPENMW_DECOMPRESS_TEXTURES", "1", 1);

    SDL_Android_Init(env, cls);

    SDL_SetMainReady();

    /* Run the application code! */

    int status;

    status = main(argcData+1, argvData);
    releaseArgv();
    /* Do not issue an exit or the whole application will terminate instead of just the SDL thread */
    /* exit(status); */

    return status;
}

#endif /* __ANDROID__ */

