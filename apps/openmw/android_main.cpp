int stderr = 0; // Hack: fix linker error

#include "SDL_main.h"
#include <SDL_gamecontroller.h>
#include <SDL_mouse.h>
#include <SDL_events.h>

/*******************************************************************************
 Functions called by JNI
 *******************************************************************************/
#include <jni.h>

/* Called before  to initialize JNI bindings  */

extern void SDL_Android_Init(JNIEnv* env, jclass cls);
extern int argcData;
extern const char **argvData;
void releaseArgv();


extern "C" int Java_org_libsdl_app_SDLActivity_getMouseX(JNIEnv *env, jclass cls, jobject obj) {
    int ret = 0;
    SDL_GetMouseState(&ret, nullptr);
    return ret;
}


extern "C" int Java_org_libsdl_app_SDLActivity_getMouseY(JNIEnv *env, jclass cls, jobject obj) {
    int ret = 0;
    SDL_GetMouseState(nullptr, &ret);
    return ret;
}

extern "C" int Java_org_libsdl_app_SDLActivity_isMouseShown(JNIEnv *env, jclass cls, jobject obj) {
    return SDL_ShowCursor(SDL_QUERY);
}

extern SDL_Window *Android_Window;
extern "C" int SDL_SendMouseMotion(SDL_Window * window, int mouseID, int relative, int x, int y);
extern "C" void Java_org_libsdl_app_SDLActivity_sendRelativeMouseMotion(JNIEnv *env, jclass cls, int x, int y) {
    SDL_SendMouseMotion(Android_Window, 0, 1, x, y);
}

extern "C" int SDL_SendMouseButton(SDL_Window * window, int mouseID, Uint8 state, Uint8 button);
extern "C" void Java_org_libsdl_app_SDLActivity_sendMouseButton(JNIEnv *env, jclass cls, int state, int button) {
    SDL_SendMouseButton(Android_Window, 0, state, button);
}

extern "C" int Java_org_libsdl_app_SDLActivity_nativeInit(JNIEnv* env, jclass cls, jobject obj) {
    setenv("OPENMW_DECOMPRESS_TEXTURES", "1", 1);

    // On Android, we use a virtual controller with guid="Virtual"
    SDL_GameControllerAddMapping("5669727475616c000000000000000000,Virtual,a:b0,b:b1,back:b15,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b16,leftshoulder:b6,leftstick:b13,lefttrigger:a5,leftx:a0,lefty:a1,rightshoulder:b7,rightstick:b14,righttrigger:a4,rightx:a2,righty:a3,start:b11,x:b3,y:b4");

    return 0;
}
