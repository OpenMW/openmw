
#include "../../SDL_internal.h"

#ifdef __ANDROID__
#include "SDL_main.h"


/*******************************************************************************
                 Functions called by JNI
*******************************************************************************/
#include <jni.h>

/* Called before  to initialize JNI bindings  */
 
 

extern void SDL_Android_Init(JNIEnv* env, jclass cls);


int Java_org_libsdl_app_SDLActivity_nativeInit(JNIEnv* env, jclass cls, jobject obj)
{
    
    SDL_Android_Init(env, cls);
 
    SDL_SetMainReady();

     
/* Run the application code! */
   
 int status;
    char *argv[2];
    argv[0] = SDL_strdup("openmw");
    argv[1] = NULL;
    status = main(1, argv);

    /* Do not issue an exit or the whole application will terminate instead of just the SDL thread */
    /* exit(status); */

    return status;
}

#endif /* __ANDROID__ */

