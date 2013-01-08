#ifndef _MWINPUT_SDLINPUTWRAPPER_H
#define _MWINPUT_SDLINPUTWRAPPER_H

#include "SDL2/SDL_events.h"
#include <extern/oics/OISCompat.h>
#include <OGRE/OgreRenderWindow.h>


namespace MWInput
{
    class MWSDLInputWrapper
    {
    public:
        MWSDLInputWrapper(Ogre::RenderWindow* window);
        ~MWSDLInputWrapper();

        void setMouseEventCallback(ICS::MWSDLMouseListener* listen) { mMouseListener = listen; }
        void setKeyboardEventCallback(ICS::MWSDLKeyListener* listen) { mKeyboardListener = listen; }

        void capture();
        bool isModifierHeld(int mod);

    private:
        ICS::MWSDLMouseListener* mMouseListener;
        ICS::MWSDLKeyListener* mKeyboardListener;
        Ogre::RenderWindow* mWindow;
        SDL_Window* mSDLWindow;

        bool mStarted;
        void _start();

    };
}

#endif
