#include "sdlinputwrapper.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <OgrePlatform.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#   include <X11/Xlib.h>
#   include <X11/Xutil.h>
#   include <X11/Xos.h>
#endif


namespace MWInput
{
    MWSDLInputWrapper::MWSDLInputWrapper(Ogre::RenderWindow *window) :
        mWindow(window), mStarted(false), mSDLWindow(NULL)
    {
        _start();
    }

    MWSDLInputWrapper::~MWSDLInputWrapper()
    {
        SDL_DestroyWindow(mSDLWindow);
        mSDLWindow = NULL;
        SDL_Quit();
    }

    void MWSDLInputWrapper::capture()
    {
        _start();

        SDL_Event evt;
        while(SDL_PollEvent(&evt))
        {
            switch(evt.type)
            {
                case SDL_MOUSEMOTION:
                    mMouseListener->mouseMoved(ICS::MWSDLMouseMotionEvent(evt.motion));
                    break;
                case SDL_MOUSEWHEEL:
                    mMouseListener->mouseMoved(ICS::MWSDLMouseMotionEvent(evt.wheel));
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    mMouseListener->mousePressed(evt.button, evt.button.button);
                    break;
                case SDL_MOUSEBUTTONUP:
                    mMouseListener->mouseReleased(evt.button, evt.button.button);
                    break;

                case SDL_KEYDOWN:
                    mKeyboardListener->keyPressed(evt.key);
                    break;
                case SDL_KEYUP:
                    mKeyboardListener->keyReleased(evt.key);
                    break;
            }
        }
    }

    bool MWSDLInputWrapper::isModifierHeld(int mod)
    {
        return SDL_GetModState() & mod;
    }

    void MWSDLInputWrapper::_start()
    {
        Uint32 flags = SDL_INIT_VIDEO;
        if(SDL_WasInit(flags) == 0)
        {
            //get the HWND from ogre's renderwindow
            size_t windowHnd;
            mWindow->getCustomAttribute("WINDOW", &windowHnd);

            //kindly ask SDL not to trash our OGL context
            SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
            SDL_Init(SDL_INIT_VIDEO);

            //wrap our own event handler around ogre's
            mSDLWindow = SDL_CreateWindowFrom((void*)windowHnd);

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
            //linux-specific event-handling fixups
            SDL_SysWMinfo wm_info;
            SDL_VERSION(&wm_info.version);

            if(SDL_GetWindowWMInfo(mSDLWindow,&wm_info))
            {
                printf("SDL version %d.%d.%d\n", wm_info.version.major, wm_info.version.minor, wm_info.version.patch);

                Display* display = wm_info.info.x11.display;
                Window w = wm_info.info.x11.window;

                // Set the input hints so we get keyboard input
                XWMHints *wmhints = XAllocWMHints();
                if (wmhints) {
                    wmhints->input = True;
                    wmhints->flags = InputHint;
                    XSetWMHints(display, w, wmhints);
                    XFree(wmhints);
                }

                //make sure to subscribe to XLib's events
                XSelectInput(display, w,
                             (FocusChangeMask | EnterWindowMask | LeaveWindowMask |
                             ExposureMask | ButtonPressMask | ButtonReleaseMask |
                             PointerMotionMask | KeyPressMask | KeyReleaseMask |
                             PropertyChangeMask | StructureNotifyMask |
                             KeymapStateMask));

                XFlush(display);
            }
#endif
        }
    }
}
