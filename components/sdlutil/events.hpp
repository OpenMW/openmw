#ifndef _SFO_EVENTS_H
#define _SFO_EVENTS_H

#include <SDL_events.h>
#include <SDL_types.h>
#include <SDL_version.h>

////////////
// Events //
////////////

namespace SDLUtil
{

    /** Extended mouse event struct where we treat the wheel like an axis, like everyone expects */
    struct MouseMotionEvent : SDL_MouseMotionEvent
    {

        Sint32 zrel;
        Sint32 z;
    };

    struct TouchEvent
    {
        int mDevice;
        int mFinger;
        float mX;
        float mY;
        float mPressure;

        explicit TouchEvent(const SDL_ControllerTouchpadEvent& arg)
            : mDevice(arg.touchpad)
            , mFinger(arg.finger)
            , mX(arg.x)
            , mY(arg.y)
            , mPressure(arg.pressure)
        {
        }
    };

    ///////////////
    // Listeners //
    ///////////////

    class MouseListener
    {
    public:
        virtual ~MouseListener() {}
        virtual void mouseMoved(const MouseMotionEvent& arg) = 0;
        virtual void mousePressed(const SDL_MouseButtonEvent& arg, Uint8 id) = 0;
        virtual void mouseReleased(const SDL_MouseButtonEvent& arg, Uint8 id) = 0;
        virtual void mouseWheelMoved(const SDL_MouseWheelEvent& arg) = 0;
    };

    class SensorListener
    {
    public:
        virtual ~SensorListener() {}
        virtual void sensorUpdated(const SDL_SensorEvent& arg) = 0;
        virtual void displayOrientationChanged() = 0;
    };

    class KeyListener
    {
    public:
        virtual ~KeyListener() {}
        virtual void textInput(const SDL_TextInputEvent& arg) {}
        virtual void keyPressed(const SDL_KeyboardEvent& arg) = 0;
        virtual void keyReleased(const SDL_KeyboardEvent& arg) = 0;
    };

    class ControllerListener
    {
    public:
        virtual ~ControllerListener() {}

        virtual void buttonPressed(int deviceID, const SDL_ControllerButtonEvent& evt) = 0;
        virtual void buttonReleased(int deviceID, const SDL_ControllerButtonEvent& evt) = 0;

        virtual void axisMoved(int deviceID, const SDL_ControllerAxisEvent& arg) = 0;

        virtual void controllerAdded(int deviceID, const SDL_ControllerDeviceEvent& arg) = 0;
        virtual void controllerRemoved(const SDL_ControllerDeviceEvent& arg) = 0;

        virtual void touchpadMoved(int deviceId, const TouchEvent& arg) = 0;
        virtual void touchpadPressed(int deviceId, const TouchEvent& arg) = 0;
        virtual void touchpadReleased(int deviceId, const TouchEvent& arg) = 0;
    };

    class WindowListener
    {
    public:
        virtual ~WindowListener() {}

        /** @remarks The window's visibility changed */
        virtual void windowVisibilityChange(bool visible) {}

        virtual void windowClosed() {}

        virtual void windowResized(int x, int y) {}
    };

}

#endif
