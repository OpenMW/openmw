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
        void setWindowEventCallback(ICS::MWSDLWindowListener* listen) { mWindowListener = listen; }

        void capture();
        bool isModifierHeld(int mod);

        void setMouseRelative(bool relative);
        bool getMouseRelative() { return mMouseRelative; }
        void setGrabPointer(bool grab);

        void warpMouse(int x, int y);
    private:
        bool _handleWarpMotion(const SDL_MouseMotionEvent& evt);
        void _wrapMousePointer(const SDL_MouseMotionEvent &evt);

        void _handleKeyPress(SDL_KeyboardEvent& evt);

        bool _start();

        ICS::MWSDLMouseListener* mMouseListener;
        ICS::MWSDLKeyListener* mKeyboardListener;
        ICS::MWSDLWindowListener* mWindowListener;

        Uint16 mWarpX;
        Uint16 mWarpY;
        bool mWarpCompensate;
        bool mMouseRelative;
        bool mWrapPointer;
        bool mGrabPointer;

        Ogre::RenderWindow* mWindow;
        SDL_Window* mSDLWindow;
    };
}

#endif
