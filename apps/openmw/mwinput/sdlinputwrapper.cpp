#include "sdlinputwrapper.hpp"
#include <SDL2/SDL.h>

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

            //just use that one for input
            SDL_Init(flags);
            mSDLWindow = SDL_CreateWindowFrom((void*)windowHnd);
        }
    }
}
