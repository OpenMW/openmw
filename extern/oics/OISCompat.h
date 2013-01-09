#ifndef _OIS_SDL_COMPAT_H
#define _OIS_SDL_COMPAT_H

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_types.h>

//TODO: Remove this. Right now we want to remain as close to OIS as possible
//So we can easily test the SDL backend

////////////
// Events //
////////////

namespace ICS {

/** Extended mouse event struct where we treat the wheel like an axis, like everyone expects */
struct MWSDLMouseMotionEvent : SDL_MouseMotionEvent {

    Sint16 zrel;

    MWSDLMouseMotionEvent()
    {
        _init();
    }

    MWSDLMouseMotionEvent( const SDL_MouseMotionEvent& evt)
    {
        _init();
        x = evt.x;
        y = evt.y;
        xrel = evt.xrel;
        yrel = evt.yrel;
        state = evt.state;
    }

    MWSDLMouseMotionEvent (const SDL_MouseWheelEvent& evt)
    {
        _init();
        zrel = evt.y;
    }

    void _init()
    {
        x = 0;
        y = 0;
        xrel = 0;
        yrel = 0;
        state = 0;
        zrel = 0;
    }
};


///////////////
// Listeners //
///////////////

class MWSDLMouseListener
{
public:
    virtual ~MWSDLMouseListener() {}
    virtual bool mouseMoved( const MWSDLMouseMotionEvent &arg ) = 0;
    virtual bool mousePressed( const SDL_MouseButtonEvent &arg, Uint8 id ) = 0;
    virtual bool mouseReleased( const SDL_MouseButtonEvent &arg, Uint8 id ) = 0;
};

class MWSDLKeyListener
{
public:
    virtual ~MWSDLKeyListener() {}
    virtual bool keyPressed(const SDL_KeyboardEvent &arg) = 0;
    virtual bool keyReleased(const SDL_KeyboardEvent &arg) = 0;
};

class MWSDLJoyStickListener
{
public:
    virtual ~MWSDLJoyStickListener() {}
    /** @remarks Joystick button down event */
    virtual bool buttonPressed( const SDL_JoyButtonEvent &evt, int button ) = 0;

    /** @remarks Joystick button up event */
    virtual bool buttonReleased( const SDL_JoyButtonEvent &evt, int button ) = 0;

    /** @remarks Joystick axis moved event */
    virtual bool axisMoved( const SDL_JoyAxisEvent &arg, int axis ) = 0;

    //-- Not so common control events, so are not required --//

    //! Joystick Event, and povID
    virtual bool povMoved( const SDL_JoyHatEvent &arg, int index) {return true;}
};

}
#endif
