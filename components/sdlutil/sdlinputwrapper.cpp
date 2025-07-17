#include "sdlinputwrapper.hpp"

#include <components/debug/debuglog.hpp>
#include <components/settings/values.hpp>

#include <osgViewer/Viewer>

namespace SDLUtil
{

    InputWrapper::InputWrapper(SDL_Window* window, osg::ref_ptr<osgViewer::Viewer> viewer, bool grab)
        : mSDLWindow(window)
        , mViewer(std::move(viewer))
        , mMouseListener(nullptr)
        , mSensorListener(nullptr)
        , mKeyboardListener(nullptr)
        , mWindowListener(nullptr)
        , mConListener(nullptr)
        , mWarpX(0)
        , mWarpY(0)
        , mWarpCompensate(false)
        , mWrapPointer(false)
        , mAllowGrab(grab)
        , mWantMouseVisible(false)
        , mWantGrab(false)
        , mWantRelative(false)
        , mGrabPointer(false)
        , mMouseRelative(false)
        , mFirstMouseMove(true)
        , mMouseZ(0)
        , mMouseX(0)
        , mMouseY(0)
        , mWindowHasFocus(true)
        , mMouseInWindow(true)
    {
        Uint32 flags = SDL_GetWindowFlags(mSDLWindow);
        mWindowHasFocus = (flags & SDL_WINDOW_INPUT_FOCUS);
        mMouseInWindow = (flags & SDL_WINDOW_MOUSE_FOCUS);
        _setWindowScale();
    }

    InputWrapper::~InputWrapper() {}

    void InputWrapper::_setWindowScale()
    {
        int w, h;
        SDL_GetWindowSize(mSDLWindow, &w, &h);
        int dw, dh;
        SDL_GL_GetDrawableSize(mSDLWindow, &dw, &dh);
        mScaleX = dw / w;
        mScaleY = dh / h;
    }

    void InputWrapper::capture(bool windowEventsOnly)
    {
        mViewer->getEventQueue()->frame(0.f);

        SDL_PumpEvents();

        SDL_Event evt;

        if (windowEventsOnly)
        {
            // During loading, handle window events, discard button presses and mouse movement and keep others for later
            while (SDL_PeepEvents(&evt, 1, SDL_GETEVENT, SDL_WINDOWEVENT, SDL_WINDOWEVENT) > 0)
                handleWindowEvent(evt);

            SDL_FlushEvent(SDL_KEYDOWN);
            SDL_FlushEvent(SDL_CONTROLLERBUTTONDOWN);
            SDL_FlushEvent(SDL_MOUSEBUTTONDOWN);
            SDL_FlushEvent(SDL_MOUSEMOTION);
            SDL_FlushEvent(SDL_MOUSEWHEEL);

            return;
        }

        while (SDL_PollEvent(&evt))
        {
#if SDL_VERSION_ATLEAST(2, 30, 50)
            // SDL2-compat may pass us SDL3 display and window events alongside the SDL2 events for funsies
            // Until we are ready to move to SDL3, we'll want to prevent the noise

            // Silence 0x151 to 0x1FF range
            if (evt.type > SDL_DISPLAYEVENT && evt.type < SDL_WINDOWEVENT)
                continue;

            // Silence 0x202 to 0x2FF range
            if (evt.type > SDL_SYSWMEVENT && evt.type < SDL_KEYDOWN)
                continue;
#endif
            switch (evt.type)
            {
                case SDL_MOUSEMOTION:
                    // Ignore this if it happened due to a warp
                    if (!_handleWarpMotion(evt.motion))
                    {
                        // If in relative mode, don't trigger events unless window has focus
                        if (!mWantRelative || mWindowHasFocus)
                            mMouseListener->mouseMoved(_packageMouseMotion(evt));

                        // Try to keep the mouse inside the window
                        if (mWindowHasFocus)
                            _wrapMousePointer(evt.motion);
                    }
                    break;
                case SDL_MOUSEWHEEL:
                    mMouseListener->mouseMoved(_packageMouseMotion(evt));
                    mMouseListener->mouseWheelMoved(evt.wheel);
                    break;
                case SDL_SENSORUPDATE:
                    mSensorListener->sensorUpdated(evt.sensor);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    mMouseListener->mousePressed(evt.button, evt.button.button);
                    break;
                case SDL_MOUSEBUTTONUP:
                    mMouseListener->mouseReleased(evt.button, evt.button.button);
                    break;
                case SDL_KEYDOWN:
                    mKeyboardListener->keyPressed(evt.key);

                    if (!isModifierHeld(KMOD_ALT) && evt.key.keysym.sym >= SDLK_F1 && evt.key.keysym.sym <= SDLK_F12)
                    {
                        mViewer->getEventQueue()->keyPress(
                            osgGA::GUIEventAdapter::KEY_F1 + (evt.key.keysym.sym - SDLK_F1));
                    }

                    break;
                case SDL_KEYUP:
                    if (!evt.key.repeat)
                    {
                        mKeyboardListener->keyReleased(evt.key);

                        if (!isModifierHeld(KMOD_ALT) && evt.key.keysym.sym >= SDLK_F1
                            && evt.key.keysym.sym <= SDLK_F12)
                            mViewer->getEventQueue()->keyRelease(
                                osgGA::GUIEventAdapter::KEY_F1 + (evt.key.keysym.sym - SDLK_F1));
                    }

                    break;
                case SDL_TEXTEDITING:
                    break;
                case SDL_TEXTINPUT:
                    mKeyboardListener->textInput(evt.text);
                    break;
                case SDL_KEYMAPCHANGED:
                    break;
                case SDL_JOYHATMOTION: // As we manage everything with GameController, don't even bother with these.
                case SDL_JOYAXISMOTION:
                case SDL_JOYBUTTONDOWN:
                case SDL_JOYBUTTONUP:
                case SDL_JOYDEVICEADDED:
                case SDL_JOYDEVICEREMOVED:
                    break;
                case SDL_CONTROLLERDEVICEADDED:
                    if (mConListener)
                        mConListener->controllerAdded(
                            1, evt.cdevice); // We only support one joystick, so give everything a generic deviceID
                    break;
                case SDL_CONTROLLERDEVICEREMOVED:
                    if (mConListener)
                        mConListener->controllerRemoved(evt.cdevice);
                    break;
                case SDL_CONTROLLERBUTTONDOWN:
                    if (mConListener)
                        mConListener->buttonPressed(1, evt.cbutton);
                    break;
                case SDL_CONTROLLERBUTTONUP:
                    if (mConListener)
                        mConListener->buttonReleased(1, evt.cbutton);
                    break;
                case SDL_CONTROLLERAXISMOTION:
                    if (mConListener)
                        mConListener->axisMoved(1, evt.caxis);
                    break;
                case SDL_CONTROLLERSENSORUPDATE:
                    // controller sensor data is received on demand
                    break;
                case SDL_CONTROLLERTOUCHPADDOWN:
                    mConListener->touchpadPressed(1, TouchEvent(evt.ctouchpad));
                    break;
                case SDL_CONTROLLERTOUCHPADMOTION:
                    mConListener->touchpadMoved(1, TouchEvent(evt.ctouchpad));
                    break;
                case SDL_CONTROLLERTOUCHPADUP:
                    mConListener->touchpadReleased(1, TouchEvent(evt.ctouchpad));
                    break;
                case SDL_WINDOWEVENT:
                    handleWindowEvent(evt);
                    break;
                case SDL_QUIT:
                    if (mWindowListener)
                        mWindowListener->windowClosed();
                    break;
                case SDL_DISPLAYEVENT:
                    switch (evt.display.event)
                    {
                        case SDL_DISPLAYEVENT_ORIENTATION:
                            if (mSensorListener
                                && evt.display.display == static_cast<Uint32>(Settings::video().mScreen))
                            {
                                mSensorListener->displayOrientationChanged();
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case SDL_CLIPBOARDUPDATE:
                    break; // We don't need this event, clipboard is retrieved on demand

                case SDL_FINGERDOWN:
                case SDL_FINGERUP:
                case SDL_FINGERMOTION:
                case SDL_DOLLARGESTURE:
                case SDL_DOLLARRECORD:
                case SDL_MULTIGESTURE:
                    // No use for touch & gesture events
                    break;

                case SDL_APP_WILLENTERBACKGROUND:
                case SDL_APP_WILLENTERFOREGROUND:
                case SDL_APP_DIDENTERBACKGROUND:
                case SDL_APP_DIDENTERFOREGROUND:
                    // We do not need background/foreground switch event for mobile devices so far
                    break;

                case SDL_APP_TERMINATING:
                    // There is nothing we can do here.
                    break;

                case SDL_APP_LOWMEMORY:
                    Log(Debug::Warning) << "System reports that free RAM on device is running low. You may encounter "
                                           "an unexpected behaviour.";
                    break;

                default:
                    Log(Debug::Info) << "Unhandled SDL event of type 0x" << std::hex << evt.type;
                    break;
            }
        }
    }

    void InputWrapper::handleWindowEvent(const SDL_Event& evt)
    {
        switch (evt.window.event)
        {
            case SDL_WINDOWEVENT_ENTER:
                mMouseInWindow = true;
                updateMouseSettings();
                break;
            case SDL_WINDOWEVENT_LEAVE:
                mMouseInWindow = false;
                updateMouseSettings();
                break;
            case SDL_WINDOWEVENT_MOVED:
                // I'm not sure what OSG is using the window position for, but I don't think it's needed,
                // so we ignore window moved events (improves window movement performance)
                break;
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                int w, h;
                SDL_GL_GetDrawableSize(mSDLWindow, &w, &h);
                int x, y;
                SDL_GetWindowPosition(mSDLWindow, &x, &y);

                // Happens when you Alt-Tab out of game
                if (w == 0 && h == 0)
                    return;

                mViewer->getCamera()->getGraphicsContext()->resized(x, y, w, h);

                mViewer->getEventQueue()->windowResize(x, y, w, h);

                if (mWindowListener)
                    mWindowListener->windowResized(w, h);

                _setWindowScale();

                break;

            case SDL_WINDOWEVENT_RESIZED:
                // This should also fire SIZE_CHANGED, so no need to handle
                break;

            case SDL_WINDOWEVENT_FOCUS_GAINED:
                mWindowHasFocus = true;
                updateMouseSettings();
                break;
            case SDL_WINDOWEVENT_FOCUS_LOST:
                mWindowHasFocus = false;
                updateMouseSettings();
                break;
            case SDL_WINDOWEVENT_CLOSE:
                break;
            case SDL_WINDOWEVENT_SHOWN:
            case SDL_WINDOWEVENT_RESTORED:
                if (mWindowListener)
                    mWindowListener->windowVisibilityChange(true);
                break;
            case SDL_WINDOWEVENT_HIDDEN:
            case SDL_WINDOWEVENT_MINIMIZED:
                if (mWindowListener)
                    mWindowListener->windowVisibilityChange(false);
                break;
        }
    }

    bool InputWrapper::isModifierHeld(int mod)
    {
        return (SDL_GetModState() & mod) != 0;
    }

    bool InputWrapper::isKeyDown(SDL_Scancode key)
    {
        return (SDL_GetKeyboardState(nullptr)[key]) != 0;
    }

    /// \brief Moves the mouse to the specified point within the viewport
    void InputWrapper::warpMouse(int x, int y)
    {
        SDL_WarpMouseInWindow(mSDLWindow, x, y);
        mWarpCompensate = true;
        mWarpX = x;
        mWarpY = y;
    }

    /// \brief Locks the pointer to the window
    void InputWrapper::setGrabPointer(bool grab)
    {
        mWantGrab = grab;
        updateMouseSettings();
    }

    /// \brief Set the mouse to relative positioning. Doesn't move the cursor
    ///        and disables mouse acceleration.
    void InputWrapper::setMouseRelative(bool relative)
    {
        mWantRelative = relative;
        updateMouseSettings();
    }

    void InputWrapper::setMouseVisible(bool visible)
    {
        mWantMouseVisible = visible;
        updateMouseSettings();
    }

    void InputWrapper::updateMouseSettings()
    {
        mGrabPointer = mWantGrab && mMouseInWindow && mWindowHasFocus;
        SDL_SetWindowGrab(mSDLWindow, mGrabPointer && mAllowGrab ? SDL_TRUE : SDL_FALSE);

        SDL_ShowCursor(mWantMouseVisible || !mWindowHasFocus);

        bool relative = mWantRelative && mMouseInWindow && mWindowHasFocus;
        if (mMouseRelative == relative)
            return;

        mMouseRelative = relative;

        mWrapPointer = false;

        // eep, wrap the pointer manually if the input driver doesn't support
        // relative positioning natively
        // also use wrapping if no-grab was specified in options (SDL_SetRelativeMouseMode
        // appears to eat the mouse cursor when pausing in a debugger)
        bool success = mAllowGrab && SDL_SetRelativeMouseMode(relative ? SDL_TRUE : SDL_FALSE) == 0;
        if (relative && !success)
            mWrapPointer = true;

        // now remove all mouse events using the old setting from the queue
        SDL_PumpEvents();
        SDL_FlushEvent(SDL_MOUSEMOTION);
    }

    /// \brief Internal method for ignoring relative motions as a side effect
    ///        of warpMouse()
    bool InputWrapper::_handleWarpMotion(const SDL_MouseMotionEvent& evt)
    {
        if (!mWarpCompensate)
            return false;

        // this was a warp event, signal the caller to eat it.
        if (evt.x == mWarpX && evt.y == mWarpY)
        {
            mWarpCompensate = false;
            return true;
        }

        return false;
    }

    /// \brief Wrap the mouse to the viewport
    void InputWrapper::_wrapMousePointer(const SDL_MouseMotionEvent& evt)
    {
        // don't wrap if we don't want relative movements, support relative
        // movements natively, or aren't grabbing anyways
        if (!mMouseRelative || !mWrapPointer || !mGrabPointer)
            return;

        int width = 0;
        int height = 0;

        SDL_GetWindowSize(mSDLWindow, &width, &height);

        const int FUDGE_FACTOR_X = width / 4;
        const int FUDGE_FACTOR_Y = height / 4;

        // warp the mouse if it's about to go outside the window
        if (evt.x - FUDGE_FACTOR_X < 0 || evt.x + FUDGE_FACTOR_X > width || evt.y - FUDGE_FACTOR_Y < 0
            || evt.y + FUDGE_FACTOR_Y > height)
        {
            warpMouse(width / 2, height / 2);
        }
    }

    /// \brief Package mouse and mousewheel motions into a single event
    MouseMotionEvent InputWrapper::_packageMouseMotion(const SDL_Event& evt)
    {
        MouseMotionEvent pack_evt = {};
        pack_evt.x = mMouseX * mScaleX;
        pack_evt.y = mMouseY * mScaleY;
        pack_evt.z = mMouseZ;

        if (evt.type == SDL_MOUSEMOTION)
        {
            pack_evt.x = mMouseX = evt.motion.x * mScaleX;
            pack_evt.y = mMouseY = evt.motion.y * mScaleY;
            pack_evt.xrel = evt.motion.xrel * mScaleX;
            pack_evt.yrel = evt.motion.yrel * mScaleY;
            pack_evt.type = SDL_MOUSEMOTION;
            if (mFirstMouseMove)
            {
                // first event should be treated as non-relative, since there's no point of reference
                // SDL then (incorrectly) uses (0,0) as point of reference, on Linux at least...
                pack_evt.xrel = pack_evt.yrel = 0;
                mFirstMouseMove = false;
            }
        }
        else if (evt.type == SDL_MOUSEWHEEL)
        {
            mMouseZ += pack_evt.zrel = (evt.wheel.y * 120);
            pack_evt.z = mMouseZ;
            pack_evt.type = SDL_MOUSEWHEEL;
        }
        else
        {
            throw std::runtime_error("Tried to package non-motion event!");
        }

        return pack_evt;
    }
}
