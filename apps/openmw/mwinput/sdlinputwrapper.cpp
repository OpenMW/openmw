#include "sdlinputwrapper.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <OgrePlatform.h>
#include <OgreRoot.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#   include <X11/Xlib.h>
#   include <X11/Xutil.h>
#   include <X11/Xos.h>
#endif


namespace MWInput
{
    MWSDLInputWrapper::MWSDLInputWrapper(Ogre::RenderWindow *window) :
        mWindow(window),
        mSDLWindow(NULL),
        mWarpCompensate(false),
        mMouseRelative(false),
        mGrabPointer(false),
        mWrapPointer(false)
    {
        _start();
    }

    MWSDLInputWrapper::~MWSDLInputWrapper()
    {
        if(mSDLWindow != NULL)
            SDL_DestroyWindow(mSDLWindow);
        mSDLWindow = NULL;
        SDL_StopTextInput();
        SDL_Quit();
    }

    bool MWSDLInputWrapper::_start()
    {
        Uint32 flags = SDL_INIT_VIDEO;
        if(SDL_WasInit(flags) == 0)
        {
            //get the HWND from ogre's renderwindow
            size_t windowHnd;
            mWindow->getCustomAttribute("WINDOW", &windowHnd);

            //kindly ask SDL not to trash our OGL context
            SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
            if(SDL_Init(SDL_INIT_VIDEO) != 0)
                return false;

            //wrap our own event handler around ogre's
            mSDLWindow = SDL_CreateWindowFrom((void*)windowHnd);

            if(mSDLWindow == NULL)
                return false;

            SDL_StartTextInput();

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
            //linux-specific event-handling fixups
            SDL_SysWMinfo wm_info;
            SDL_VERSION(&wm_info.version);

            if(SDL_GetWindowWMInfo(mSDLWindow,&wm_info))
            {
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
            SDL_ShowCursor(SDL_FALSE);
        }

        return true;
    }

    void MWSDLInputWrapper::capture()
    {
        if(!_start())
            throw std::runtime_error(SDL_GetError());

        SDL_Event evt;
        while(SDL_PollEvent(&evt))
        {
            switch(evt.type)
            {
                case SDL_MOUSEMOTION:
                    //ignore this if it happened due to a warp
                    if(!_handleWarpMotion(evt.motion))
                    {
                        mMouseListener->mouseMoved(ICS::MWSDLMouseMotionEvent(evt.motion));

                        //try to keep the mouse inside the window
                        _wrapMousePointer(evt.motion);
                    }
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
                    _handleKeyPress(evt.key);
                    break;
                case SDL_KEYUP:
                    mKeyboardListener->keyReleased(evt.key);
                    break;

                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    mWindowListener->windowFocusChange(true);
                    break;
                case SDL_WINDOWEVENT_FOCUS_LOST:
                    mWindowListener->windowFocusChange(false);
                    break;
                case SDL_WINDOWEVENT_EXPOSED:
                    mWindowListener->windowVisibilityChange(true);
                    break;
                case SDL_WINDOWEVENT_HIDDEN:
                    mWindowListener->windowVisibilityChange(false);
                    break;

                //SDL traps ^C signals, pass it to OGRE.
                case SDL_QUIT:
                    Ogre::Root::getSingleton().queueEndRendering();
                    break;
            }
        }
    }

    bool MWSDLInputWrapper::isModifierHeld(int mod)
    {
        return SDL_GetModState() & mod;
    }

    void MWSDLInputWrapper::warpMouse(int x, int y)
    {
        SDL_WarpMouseInWindow(mSDLWindow, x, y);
        mWarpCompensate = true;
        mWarpX = x;
        mWarpY = y;
    }

    void MWSDLInputWrapper::setGrabPointer(bool grab)
    {
        SDL_bool sdlGrab = grab ? SDL_TRUE : SDL_FALSE;

        mGrabPointer = grab;
        SDL_SetWindowGrab(mSDLWindow, sdlGrab);
    }

    void MWSDLInputWrapper::setMouseRelative(bool relative)
    {
        if(mMouseRelative == relative)
            return;

        mMouseRelative = relative;

        mWrapPointer = false;

        //eep, wrap the pointer manually if the input driver doesn't support
        //relative positioning natively
        if(SDL_SetRelativeMouseMode(relative ? SDL_TRUE : SDL_FALSE) == -1)
        {
            if(relative)
                mWrapPointer = true;
        }

        //now remove all mouse events using the old setting from the queue
        SDL_PumpEvents();

        SDL_Event dummy[20];
        SDL_PeepEvents(dummy, 20, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEMOTION);
    }

    bool MWSDLInputWrapper::_handleWarpMotion(const SDL_MouseMotionEvent& evt)
    {
        if(!mWarpCompensate)
            return false;

        //this was a warp event, signal the caller to eat it.
        if(evt.x == mWarpX && evt.y == mWarpY)
        {
            mWarpCompensate = false;
            return true;
        }

        return false;
    }

    void MWSDLInputWrapper::_wrapMousePointer(const SDL_MouseMotionEvent& evt)
    {
        //don't wrap if we don't want relative movements, support relative
        //movements natively, or aren't grabbing anyways
        if(!mMouseRelative || !mWrapPointer || !mGrabPointer)
            return;

        int width = 0;
        int height = 0;

        SDL_GetWindowSize(mSDLWindow, &width, &height);

        const int FUDGE_FACTOR_X = width / 4;
        const int FUDGE_FACTOR_Y = height / 4;

        //warp the mouse if it's about to go outside the window
        if(evt.x - FUDGE_FACTOR_X < 0  || evt.x + FUDGE_FACTOR_X > width
                || evt.y - FUDGE_FACTOR_Y < 0 || evt.y + FUDGE_FACTOR_Y > height)
        {
            warpMouse(width / 2, height / 2);
        }
    }

    void MWSDLInputWrapper::_handleKeyPress(SDL_KeyboardEvent &evt)
    {
        //SDL keyboard events are followed by the actual text those keys would generate
        //to account for languages that require multiple keystrokes to produce a key.
        //Look for an event immediately following ours, assuming each key produces exactly
        //one character.

        //TODO: This won't work for multibyte characters, but MyGUI is the only consumer
        //of these, does it even support multibyte characters?

        SDL_Event text_evts[1];
        if(SDL_PeepEvents(text_evts, 1, SDL_GETEVENT, SDL_TEXTINPUT, SDL_TEXTINPUT) != 0)
        {
            evt.keysym.unicode = text_evts[0].text.text[0];
        }

        mKeyboardListener->keyPressed(evt);
    }
}
