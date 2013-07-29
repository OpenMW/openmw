#ifndef _SFO_EVENTS_H
#define _SFO_EVENTS_H

#include <SDL.h>


////////////
// Events //
////////////

namespace SFO {

/** Extended mouse event struct where we treat the wheel like an axis, like everyone expects */
struct MouseMotionEvent : SDL_MouseMotionEvent {

    Sint32 zrel;
    Sint32 z;
};


///////////////
// Listeners //
///////////////

class MouseListener
{
public:
    virtual ~MouseListener() {}
    virtual bool mouseMoved( const MouseMotionEvent &arg ) = 0;
    virtual bool mousePressed( const SDL_MouseButtonEvent &arg, Uint8 id ) = 0;
    virtual bool mouseReleased( const SDL_MouseButtonEvent &arg, Uint8 id ) = 0;
};

class KeyListener
{
public:
    virtual ~KeyListener() {}
    virtual void textInput (const SDL_TextInputEvent& arg) {}
    virtual bool keyPressed(const SDL_KeyboardEvent &arg) = 0;
    virtual bool keyReleased(const SDL_KeyboardEvent &arg) = 0;
};

class JoyListener
{
public:
    virtual ~JoyListener() {}
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

class WindowListener
{
public:
    virtual ~WindowListener() {}

    /** @remarks The window's visibility changed */
    virtual void windowVisibilityChange( bool visible ) {};

    /** @remarks The window got / lost input focus */
    virtual void windowFocusChange( bool have_focus ) {}

    virtual void windowResized (int x, int y) {}
};

}

#endif
