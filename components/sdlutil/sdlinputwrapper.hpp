#ifndef OPENMW_COMPONENTS_SDLUTIL_SDLINPUTWRAPPER_H
#define OPENMW_COMPONENTS_SDLUTIL_SDLINPUTWRAPPER_H

#include <map>

#include <osg/ref_ptr>

#include <SDL_events.h>
#include <SDL_version.h>

#include "OISCompat.hpp"
#include "events.hpp"

namespace osgViewer
{
    class Viewer;
}

namespace SDLUtil
{
    /// \brief A wrapper around SDL's event queue, mostly used for handling input-related events.
    class InputWrapper
    {
    public:
        InputWrapper(SDL_Window *window, osg::ref_ptr<osgViewer::Viewer> viewer, bool grab);
        ~InputWrapper();

        void setMouseEventCallback(MouseListener* listen) { mMouseListener = listen; }
        void setSensorEventCallback(SensorListener* listen) { mSensorListener = listen; }
        void setKeyboardEventCallback(KeyListener* listen) { mKeyboardListener = listen; }
        void setWindowEventCallback(WindowListener* listen) { mWindowListener = listen; }
        void setControllerEventCallback(ControllerListener* listen) { mConListener = listen; }

        void capture(bool windowEventsOnly);
        bool isModifierHeld(int mod);
        bool isKeyDown(SDL_Scancode key);

        void setMouseVisible (bool visible);
        void setMouseRelative(bool relative);
        bool getMouseRelative() { return mMouseRelative; }
        void setGrabPointer(bool grab);

        OIS::KeyCode sdl2OISKeyCode(SDL_Keycode code);

        void warpMouse(int x, int y);

        void updateMouseSettings();

    private:
        void handleWindowEvent(const SDL_Event& evt);

        bool _handleWarpMotion(const SDL_MouseMotionEvent& evt);
        void _wrapMousePointer(const SDL_MouseMotionEvent &evt);
        MouseMotionEvent _packageMouseMotion(const SDL_Event& evt);

        void _setupOISKeys();

        SDL_Window* mSDLWindow;
        osg::ref_ptr<osgViewer::Viewer> mViewer;

        MouseListener* mMouseListener;
        SensorListener* mSensorListener;
        KeyListener* mKeyboardListener;
        WindowListener* mWindowListener;
        ControllerListener* mConListener;

        typedef std::map<SDL_Keycode, OIS::KeyCode> KeyMap;
        KeyMap mKeyMap;

        Uint16 mWarpX;
        Uint16 mWarpY;
        bool mWarpCompensate;
        bool mWrapPointer;

        bool mAllowGrab;
        bool mWantMouseVisible;
        bool mWantGrab;
        bool mWantRelative;
        bool mGrabPointer;
        bool mMouseRelative;

        bool mFirstMouseMove;

        Sint32 mMouseZ;
        Sint32 mMouseX;
        Sint32 mMouseY;

        bool mWindowHasFocus;
        bool mMouseInWindow;
    };

}

#endif
