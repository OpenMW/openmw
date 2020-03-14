#include "sdlinputwrapper.hpp"

#include <components/debug/debuglog.hpp>
#include <components/settings/settings.hpp>

#include <osgViewer/Viewer>

namespace SDLUtil
{

InputWrapper::InputWrapper(SDL_Window* window, osg::ref_ptr<osgViewer::Viewer> viewer, bool grab) :
        mSDLWindow(window),
        mViewer(viewer),
        mMouseListener(nullptr),
        mSensorListener(nullptr),
        mKeyboardListener(nullptr),
        mWindowListener(nullptr),
        mConListener(nullptr),
        mWarpX(0),
        mWarpY(0),
        mWarpCompensate(false),
        mWrapPointer(false),
        mAllowGrab(grab),
        mWantMouseVisible(false),
        mWantGrab(false),
        mWantRelative(false),
        mGrabPointer(false),
        mMouseRelative(false),
        mFirstMouseMove(true),
        mMouseZ(0),
        mMouseX(0),
        mMouseY(0),
        mWindowHasFocus(true),
        mMouseInWindow(true)
    {
        _setupOISKeys();

        Uint32 flags = SDL_GetWindowFlags(mSDLWindow);
        mWindowHasFocus = (flags & SDL_WINDOW_INPUT_FOCUS);
        mMouseInWindow = (flags & SDL_WINDOW_MOUSE_FOCUS);
    }

    InputWrapper::~InputWrapper()
    {
    }

    void InputWrapper::capture(bool windowEventsOnly)
    {
        mViewer->getEventQueue()->frame(0.f);

        SDL_PumpEvents();

        SDL_Event evt;

        if (windowEventsOnly)
        {
            // During loading, just handle window events, and keep others for later
            while (SDL_PeepEvents(&evt, 1, SDL_GETEVENT, SDL_WINDOWEVENT, SDL_WINDOWEVENT))
                handleWindowEvent(evt);
            return;
        }

        while(SDL_PollEvent(&evt))
        {
            switch(evt.type)
            {
                case SDL_MOUSEMOTION:
                    // Ignore this if it happened due to a warp
                    if(!_handleWarpMotion(evt.motion))
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
                        mViewer->getEventQueue()->keyPress(osgGA::GUIEventAdapter::KEY_F1 + (evt.key.keysym.sym - SDLK_F1));
                    }

                    break;
                case SDL_KEYUP:
                    if (!evt.key.repeat)
                    {
                        mKeyboardListener->keyReleased(evt.key);

                        if (!isModifierHeld(KMOD_ALT) && evt.key.keysym.sym >= SDLK_F1 && evt.key.keysym.sym <= SDLK_F12)
                            mViewer->getEventQueue()->keyRelease(osgGA::GUIEventAdapter::KEY_F1 + (evt.key.keysym.sym - SDLK_F1));
                    }

                    break;
                case SDL_TEXTEDITING:
                    break;
                case SDL_TEXTINPUT:
                    mKeyboardListener->textInput(evt.text);
                    break;
                case SDL_KEYMAPCHANGED:
                    break;
                case SDL_JOYHATMOTION: //As we manage everything with GameController, don't even bother with these.
                case SDL_JOYAXISMOTION:
                case SDL_JOYBUTTONDOWN:
                case SDL_JOYBUTTONUP:
                case SDL_JOYDEVICEADDED:
                case SDL_JOYDEVICEREMOVED:
                    break;
                case SDL_CONTROLLERDEVICEADDED:
                    if(mConListener)
                        mConListener->controllerAdded(1, evt.cdevice); //We only support one joystick, so give everything a generic deviceID
                    break;
                case SDL_CONTROLLERDEVICEREMOVED:
                    if(mConListener)
                        mConListener->controllerRemoved(evt.cdevice);
                    break;
                case SDL_CONTROLLERBUTTONDOWN:
                    if(mConListener)
                        mConListener->buttonPressed(1, evt.cbutton);
                    break;
                case SDL_CONTROLLERBUTTONUP:
                    if(mConListener)
                        mConListener->buttonReleased(1, evt.cbutton);
                    break;
                case SDL_CONTROLLERAXISMOTION:
                    if(mConListener)
                        mConListener->axisMoved(1, evt.caxis);
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
                            if (mSensorListener && evt.display.display == (unsigned int) Settings::Manager::getInt("screen", "Video"))
                                mSensorListener->displayOrientationChanged();
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
                    Log(Debug::Warning) << "System reports that free RAM on device is running low. You may encounter an unexpected behaviour.";
                    break;

                default:
                    Log(Debug::Info) << "Unhandled SDL event of type 0x" << std::hex << evt.type;
                    break;
            }
        }
    }

    void InputWrapper::handleWindowEvent(const SDL_Event& evt)
    {
        switch (evt.window.event) {
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
                int w,h;
                SDL_GetWindowSize(mSDLWindow, &w, &h);
                int x,y;
                SDL_GetWindowPosition(mSDLWindow, &x,&y);
                mViewer->getCamera()->getGraphicsContext()->resized(x,y,w,h);

                mViewer->getEventQueue()->windowResize(x,y,w,h);

                if (mWindowListener)
                    mWindowListener->windowResized(w, h);

                break;

            case SDL_WINDOWEVENT_RESIZED:
                // This should also fire SIZE_CHANGED, so no need to handle
                break;

            case SDL_WINDOWEVENT_FOCUS_GAINED:
                mWindowHasFocus = true;
                updateMouseSettings();
                if (mWindowListener)
                    mWindowListener->windowFocusChange(true);

                break;
            case SDL_WINDOWEVENT_FOCUS_LOST:
                mWindowHasFocus = false;
                updateMouseSettings();
                if (mWindowListener)
                    mWindowListener->windowFocusChange(false);
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
        if(mMouseRelative == relative)
            return;

        mMouseRelative = relative;

        mWrapPointer = false;

        // eep, wrap the pointer manually if the input driver doesn't support
        // relative positioning natively
        // also use wrapping if no-grab was specified in options (SDL_SetRelativeMouseMode
        // appears to eat the mouse cursor when pausing in a debugger)
        bool success = mAllowGrab && SDL_SetRelativeMouseMode(relative ? SDL_TRUE : SDL_FALSE) == 0;
        if(relative && !success)
            mWrapPointer = true;

        //now remove all mouse events using the old setting from the queue
        SDL_PumpEvents();
        SDL_FlushEvent(SDL_MOUSEMOTION);
    }

    /// \brief Internal method for ignoring relative motions as a side effect
    ///        of warpMouse()
    bool InputWrapper::_handleWarpMotion(const SDL_MouseMotionEvent& evt)
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

    /// \brief Wrap the mouse to the viewport
    void InputWrapper::_wrapMousePointer(const SDL_MouseMotionEvent& evt)
    {
        //don't wrap if we don't want relative movements, support relative
        //movements natively, or aren't grabbing anyways
        if(!mMouseRelative || !mWrapPointer || !mGrabPointer)
            return;

        int width = 0;
        int height = 0;

        SDL_GetWindowSize(mSDLWindow, &width, &height);

        const int FUDGE_FACTOR_X = width/4;
        const int FUDGE_FACTOR_Y = height/4;

        //warp the mouse if it's about to go outside the window
        if(evt.x - FUDGE_FACTOR_X < 0  || evt.x + FUDGE_FACTOR_X > width
                || evt.y - FUDGE_FACTOR_Y < 0 || evt.y + FUDGE_FACTOR_Y > height)
        {
            warpMouse(width / 2, height / 2);
        }
    }

    /// \brief Package mouse and mousewheel motions into a single event
    MouseMotionEvent InputWrapper::_packageMouseMotion(const SDL_Event &evt)
    {
        MouseMotionEvent pack_evt;
        pack_evt.x = mMouseX;
        pack_evt.xrel = 0;
        pack_evt.y = mMouseY;
        pack_evt.yrel = 0;
        pack_evt.z = mMouseZ;
        pack_evt.zrel = 0;

        if(evt.type == SDL_MOUSEMOTION)
        {
            pack_evt.x = mMouseX = evt.motion.x;
            pack_evt.y = mMouseY = evt.motion.y;
            pack_evt.xrel = evt.motion.xrel;
            pack_evt.yrel = evt.motion.yrel;
            if (mFirstMouseMove)
            {
                // first event should be treated as non-relative, since there's no point of reference
                // SDL then (incorrectly) uses (0,0) as point of reference, on Linux at least...
                pack_evt.xrel = pack_evt.yrel = 0;
                mFirstMouseMove = false;
            }
        }
        else if(evt.type == SDL_MOUSEWHEEL)
        {
            mMouseZ += pack_evt.zrel = (evt.wheel.y * 120);
            pack_evt.z = mMouseZ;
        }
        else
        {
            throw std::runtime_error("Tried to package non-motion event!");
        }

        return pack_evt;
    }

    OIS::KeyCode InputWrapper::sdl2OISKeyCode(SDL_Keycode code)
    {
        OIS::KeyCode kc = OIS::KC_UNASSIGNED;

        KeyMap::const_iterator ois_equiv = mKeyMap.find(code);

        if(ois_equiv != mKeyMap.end())
            kc = ois_equiv->second;

        return kc;
    }

    void InputWrapper::_setupOISKeys()
    {
        //lifted from OIS's SDLKeyboard.cpp

        mKeyMap.insert( KeyMap::value_type(SDLK_UNKNOWN, OIS::KC_UNASSIGNED));
        mKeyMap.insert( KeyMap::value_type(SDLK_ESCAPE, OIS::KC_ESCAPE) );
        mKeyMap.insert( KeyMap::value_type(SDLK_1, OIS::KC_1) );
        mKeyMap.insert( KeyMap::value_type(SDLK_2, OIS::KC_2) );
        mKeyMap.insert( KeyMap::value_type(SDLK_3, OIS::KC_3) );
        mKeyMap.insert( KeyMap::value_type(SDLK_4, OIS::KC_4) );
        mKeyMap.insert( KeyMap::value_type(SDLK_5, OIS::KC_5) );
        mKeyMap.insert( KeyMap::value_type(SDLK_6, OIS::KC_6) );
        mKeyMap.insert( KeyMap::value_type(SDLK_7, OIS::KC_7) );
        mKeyMap.insert( KeyMap::value_type(SDLK_8, OIS::KC_8) );
        mKeyMap.insert( KeyMap::value_type(SDLK_9, OIS::KC_9) );
        mKeyMap.insert( KeyMap::value_type(SDLK_0, OIS::KC_0) );
        mKeyMap.insert( KeyMap::value_type(SDLK_MINUS, OIS::KC_MINUS) );
        mKeyMap.insert( KeyMap::value_type(SDLK_EQUALS, OIS::KC_EQUALS) );
        mKeyMap.insert( KeyMap::value_type(SDLK_BACKSPACE, OIS::KC_BACK) );
        mKeyMap.insert( KeyMap::value_type(SDLK_TAB, OIS::KC_TAB) );
        mKeyMap.insert( KeyMap::value_type(SDLK_q, OIS::KC_Q) );
        mKeyMap.insert( KeyMap::value_type(SDLK_w, OIS::KC_W) );
        mKeyMap.insert( KeyMap::value_type(SDLK_e, OIS::KC_E) );
        mKeyMap.insert( KeyMap::value_type(SDLK_r, OIS::KC_R) );
        mKeyMap.insert( KeyMap::value_type(SDLK_t, OIS::KC_T) );
        mKeyMap.insert( KeyMap::value_type(SDLK_y, OIS::KC_Y) );
        mKeyMap.insert( KeyMap::value_type(SDLK_u, OIS::KC_U) );
        mKeyMap.insert( KeyMap::value_type(SDLK_i, OIS::KC_I) );
        mKeyMap.insert( KeyMap::value_type(SDLK_o, OIS::KC_O) );
        mKeyMap.insert( KeyMap::value_type(SDLK_p, OIS::KC_P) );
        mKeyMap.insert( KeyMap::value_type(SDLK_RETURN, OIS::KC_RETURN) );
        mKeyMap.insert( KeyMap::value_type(SDLK_a, OIS::KC_A) );
        mKeyMap.insert( KeyMap::value_type(SDLK_s, OIS::KC_S) );
        mKeyMap.insert( KeyMap::value_type(SDLK_d, OIS::KC_D) );
        mKeyMap.insert( KeyMap::value_type(SDLK_f, OIS::KC_F) );
        mKeyMap.insert( KeyMap::value_type(SDLK_g, OIS::KC_G) );
        mKeyMap.insert( KeyMap::value_type(SDLK_h, OIS::KC_H) );
        mKeyMap.insert( KeyMap::value_type(SDLK_j, OIS::KC_J) );
        mKeyMap.insert( KeyMap::value_type(SDLK_k, OIS::KC_K) );
        mKeyMap.insert( KeyMap::value_type(SDLK_l, OIS::KC_L) );
        mKeyMap.insert( KeyMap::value_type(SDLK_SEMICOLON, OIS::KC_SEMICOLON) );
        mKeyMap.insert( KeyMap::value_type(SDLK_COLON, OIS::KC_COLON) );
        mKeyMap.insert( KeyMap::value_type(SDLK_QUOTE, OIS::KC_APOSTROPHE) );
        mKeyMap.insert( KeyMap::value_type(SDLK_BACKQUOTE, OIS::KC_GRAVE)  );
        mKeyMap.insert( KeyMap::value_type(SDLK_LSHIFT, OIS::KC_LSHIFT) );
        mKeyMap.insert( KeyMap::value_type(SDLK_BACKSLASH, OIS::KC_BACKSLASH) );
        mKeyMap.insert( KeyMap::value_type(SDLK_SLASH, OIS::KC_SLASH) );
        mKeyMap.insert( KeyMap::value_type(SDLK_z, OIS::KC_Z) );
        mKeyMap.insert( KeyMap::value_type(SDLK_x, OIS::KC_X) );
        mKeyMap.insert( KeyMap::value_type(SDLK_c, OIS::KC_C) );
        mKeyMap.insert( KeyMap::value_type(SDLK_v, OIS::KC_V) );
        mKeyMap.insert( KeyMap::value_type(SDLK_b, OIS::KC_B) );
        mKeyMap.insert( KeyMap::value_type(SDLK_n, OIS::KC_N) );
        mKeyMap.insert( KeyMap::value_type(SDLK_m, OIS::KC_M) );
        mKeyMap.insert( KeyMap::value_type(SDLK_COMMA, OIS::KC_COMMA)  );
        mKeyMap.insert( KeyMap::value_type(SDLK_PERIOD, OIS::KC_PERIOD));
        mKeyMap.insert( KeyMap::value_type(SDLK_RSHIFT, OIS::KC_RSHIFT));
        mKeyMap.insert( KeyMap::value_type(SDLK_KP_MULTIPLY, OIS::KC_MULTIPLY) );
        mKeyMap.insert( KeyMap::value_type(SDLK_LALT, OIS::KC_LMENU) );
        mKeyMap.insert( KeyMap::value_type(SDLK_SPACE, OIS::KC_SPACE));
        mKeyMap.insert( KeyMap::value_type(SDLK_CAPSLOCK, OIS::KC_CAPITAL) );
        mKeyMap.insert( KeyMap::value_type(SDLK_F1, OIS::KC_F1) );
        mKeyMap.insert( KeyMap::value_type(SDLK_F2, OIS::KC_F2) );
        mKeyMap.insert( KeyMap::value_type(SDLK_F3, OIS::KC_F3) );
        mKeyMap.insert( KeyMap::value_type(SDLK_F4, OIS::KC_F4) );
        mKeyMap.insert( KeyMap::value_type(SDLK_F5, OIS::KC_F5) );
        mKeyMap.insert( KeyMap::value_type(SDLK_F6, OIS::KC_F6) );
        mKeyMap.insert( KeyMap::value_type(SDLK_F7, OIS::KC_F7) );
        mKeyMap.insert( KeyMap::value_type(SDLK_F8, OIS::KC_F8) );
        mKeyMap.insert( KeyMap::value_type(SDLK_F9, OIS::KC_F9) );
        mKeyMap.insert( KeyMap::value_type(SDLK_F10, OIS::KC_F10) );
        mKeyMap.insert( KeyMap::value_type(SDLK_NUMLOCKCLEAR, OIS::KC_NUMLOCK) );
        mKeyMap.insert( KeyMap::value_type(SDLK_SCROLLLOCK, OIS::KC_SCROLL));
        mKeyMap.insert( KeyMap::value_type(SDLK_KP_7, OIS::KC_NUMPAD7) );
        mKeyMap.insert( KeyMap::value_type(SDLK_KP_8, OIS::KC_NUMPAD8) );
        mKeyMap.insert( KeyMap::value_type(SDLK_KP_9, OIS::KC_NUMPAD9) );
        mKeyMap.insert( KeyMap::value_type(SDLK_KP_MINUS, OIS::KC_SUBTRACT) );
        mKeyMap.insert( KeyMap::value_type(SDLK_KP_4, OIS::KC_NUMPAD4) );
        mKeyMap.insert( KeyMap::value_type(SDLK_KP_5, OIS::KC_NUMPAD5) );
        mKeyMap.insert( KeyMap::value_type(SDLK_KP_6, OIS::KC_NUMPAD6) );
        mKeyMap.insert( KeyMap::value_type(SDLK_KP_PLUS, OIS::KC_ADD) );
        mKeyMap.insert( KeyMap::value_type(SDLK_KP_1, OIS::KC_NUMPAD1) );
        mKeyMap.insert( KeyMap::value_type(SDLK_KP_2, OIS::KC_NUMPAD2) );
        mKeyMap.insert( KeyMap::value_type(SDLK_KP_3, OIS::KC_NUMPAD3) );
        mKeyMap.insert( KeyMap::value_type(SDLK_KP_0, OIS::KC_NUMPAD0) );
        mKeyMap.insert( KeyMap::value_type(SDLK_KP_PERIOD, OIS::KC_DECIMAL) );
        mKeyMap.insert( KeyMap::value_type(SDLK_F11, OIS::KC_F11) );
        mKeyMap.insert( KeyMap::value_type(SDLK_F12, OIS::KC_F12) );
        mKeyMap.insert( KeyMap::value_type(SDLK_F13, OIS::KC_F13) );
        mKeyMap.insert( KeyMap::value_type(SDLK_F14, OIS::KC_F14) );
        mKeyMap.insert( KeyMap::value_type(SDLK_F15, OIS::KC_F15) );
        mKeyMap.insert( KeyMap::value_type(SDLK_KP_EQUALS, OIS::KC_NUMPADEQUALS) );
        mKeyMap.insert( KeyMap::value_type(SDLK_KP_DIVIDE, OIS::KC_DIVIDE) );
        mKeyMap.insert( KeyMap::value_type(SDLK_SYSREQ, OIS::KC_SYSRQ) );
        mKeyMap.insert( KeyMap::value_type(SDLK_RALT, OIS::KC_RMENU) );
        mKeyMap.insert( KeyMap::value_type(SDLK_HOME, OIS::KC_HOME) );
        mKeyMap.insert( KeyMap::value_type(SDLK_UP, OIS::KC_UP) );
        mKeyMap.insert( KeyMap::value_type(SDLK_PAGEUP, OIS::KC_PGUP) );
        mKeyMap.insert( KeyMap::value_type(SDLK_LEFT, OIS::KC_LEFT) );
        mKeyMap.insert( KeyMap::value_type(SDLK_RIGHT, OIS::KC_RIGHT) );
        mKeyMap.insert( KeyMap::value_type(SDLK_END, OIS::KC_END) );
        mKeyMap.insert( KeyMap::value_type(SDLK_DOWN, OIS::KC_DOWN) );
        mKeyMap.insert( KeyMap::value_type(SDLK_PAGEDOWN, OIS::KC_PGDOWN) );
        mKeyMap.insert( KeyMap::value_type(SDLK_INSERT, OIS::KC_INSERT) );
        mKeyMap.insert( KeyMap::value_type(SDLK_DELETE, OIS::KC_DELETE) );
        mKeyMap.insert( KeyMap::value_type(SDLK_KP_ENTER, OIS::KC_NUMPADENTER) );
        mKeyMap.insert( KeyMap::value_type(SDLK_APPLICATION, OIS::KC_APPS) );

//The function of the Ctrl and Meta keys are switched on macOS compared to other platforms.
//For instance, Cmd+C versus Ctrl+C to copy from the system clipboard
#if defined(__APPLE__)
        mKeyMap.insert( KeyMap::value_type(SDLK_LGUI, OIS::KC_LCONTROL) );
        mKeyMap.insert( KeyMap::value_type(SDLK_RGUI, OIS::KC_RCONTROL) );
        mKeyMap.insert( KeyMap::value_type(SDLK_LCTRL, OIS::KC_LWIN));
        mKeyMap.insert( KeyMap::value_type(SDLK_RCTRL, OIS::KC_RWIN) );
#else
        mKeyMap.insert( KeyMap::value_type(SDLK_LGUI, OIS::KC_LWIN) );
        mKeyMap.insert( KeyMap::value_type(SDLK_RGUI, OIS::KC_RWIN) );
        mKeyMap.insert( KeyMap::value_type(SDLK_LCTRL, OIS::KC_LCONTROL));
        mKeyMap.insert( KeyMap::value_type(SDLK_RCTRL, OIS::KC_RCONTROL) );
#endif
    }
}
