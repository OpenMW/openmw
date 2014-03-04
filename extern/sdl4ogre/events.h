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
    virtual void mouseMoved( const MouseMotionEvent &arg ) = 0;
    virtual void mousePressed( const SDL_MouseButtonEvent &arg, Uint8 id ) = 0;
    virtual void mouseReleased( const SDL_MouseButtonEvent &arg, Uint8 id ) = 0;
};

class KeyListener
{
public:
    virtual ~KeyListener() {}
    virtual void textInput (const SDL_TextInputEvent& arg) {}
    virtual void keyPressed(const SDL_KeyboardEvent &arg) = 0;
    virtual void keyReleased(const SDL_KeyboardEvent &arg) = 0;
};

class JoyListener
{
public:
    virtual ~JoyListener() {}
    /** @remarks Joystick button down event */
    virtual void buttonPressed( const SDL_JoyButtonEvent &evt, int button ) = 0;

    /** @remarks Joystick button up event */
    virtual void buttonReleased( const SDL_JoyButtonEvent &evt, int button ) = 0;

    /** @remarks Joystick axis moved event */
    virtual void axisMoved( const SDL_JoyAxisEvent &arg, int axis ) = 0;

    //-- Not so common control events, so are not required --//

    //! Joystick Event, and povID
    virtual void povMoved( const SDL_JoyHatEvent &arg, int index) {}
};

class WindowListener
{
public:
    virtual ~WindowListener() {}

    /** @remarks The window's visibility changed */
    virtual void windowVisibilityChange( bool visible ) {};

    /** @remarks The window got / lost input focus */
    virtual void windowFocusChange( bool have_focus ) {}

    virtual void windowClosed () {}

    virtual void windowResized (int x, int y) {}
};

}

#endif
