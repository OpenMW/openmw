#include "sdlinputwrapper.hpp"
#include <SDL2/SDL_syswm.h>

#include <OgrePlatform.h>
#include <OgreRoot.h>
#include <OgreHardwarePixelBuffer.h>
#include <cstdint>

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#   include <X11/Xlib.h>
#   include <X11/Xutil.h>
#   include <X11/Xos.h>
#endif


namespace SFO
{
    /// \brief General purpose wrapper for OGRE applications around SDL's event
    ///        queue, mostly used for handling input-related events.
    InputWrapper::InputWrapper(Ogre::RenderWindow *window) :
        mWindow(window),
        mSDLWindow(NULL),
        mWarpCompensate(false),
        mMouseRelative(false),
        mGrabPointer(false),
        mWrapPointer(false),
        mMouseZ(0),
        mMouseY(0),
        mMouseX(0)
    {
        _start();
        _setupOISKeys();
    }

    InputWrapper::~InputWrapper()
    {
        if(mSDLWindow != NULL)
            SDL_DestroyWindow(mSDLWindow);
        mSDLWindow = NULL;

        CursorMap::const_iterator curs_iter = mCursorMap.begin();

        while(curs_iter != mCursorMap.end())
        {
            SDL_FreeCursor(curs_iter->second);
            ++curs_iter;
        }

        mCursorMap.clear();

        SDL_StopTextInput();
        SDL_Quit();
    }

    bool InputWrapper::_start()
    {
        Uint32 flags = SDL_INIT_VIDEO;
        if(SDL_WasInit(flags) == 0)
        {
            //get the HWND from ogre's renderwindow
            size_t windowHnd;
            mWindow->getCustomAttribute("WINDOW", &windowHnd);

            //kindly ask SDL not to trash our OGL context
            //might this be related to http://bugzilla.libsdl.org/show_bug.cgi?id=748 ?
            SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
            if(SDL_Init(SDL_INIT_VIDEO) != 0)
                return false;

            //wrap our own event handler around ogre's
            mSDLWindow = SDL_CreateWindowFrom((void*)windowHnd);

            if(mSDLWindow == NULL)
                return false;

            //without this SDL will take ownership of the window and iconify it when
            //we alt-tab away.
            SDL_SetWindowFullscreen(mSDLWindow, 0);

            //translate our keypresses into text
            SDL_StartTextInput();

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
            //linux-specific event-handling fixups
            //see http://bugzilla.libsdl.org/show_bug.cgi?id=730
            SDL_SysWMinfo wm_info;
            SDL_VERSION(&wm_info.version);

            if(SDL_GetWindowWMInfo(mSDLWindow,&wm_info))
            {
                Display* display = wm_info.info.x11.display;
                Window w = wm_info.info.x11.window;

                // Set the input hints so we get keyboard input
                XWMHints *wmhints = XAllocWMHints();
                if (wmhints) {
                    wmhints->input = True;
                    wmhints->flags = InputHint;
                    XSetWMHints(display, w, wmhints);
                    XFree(wmhints);
                }

                //make sure to subscribe to XLib's events
                XSelectInput(display, w,
                             (FocusChangeMask | EnterWindowMask | LeaveWindowMask |
                             ExposureMask | ButtonPressMask | ButtonReleaseMask |
                             PointerMotionMask | KeyPressMask | KeyReleaseMask |
                             PropertyChangeMask | StructureNotifyMask |
                             KeymapStateMask));

                XFlush(display);
            }
#endif
            SDL_ShowCursor(SDL_FALSE);
        }

        return true;
    }

    void InputWrapper::capture()
    {
        if(!_start())
            throw std::runtime_error(SDL_GetError());

        SDL_Event evt;
        while(SDL_PollEvent(&evt))
        {
            switch(evt.type)
            {
                case SDL_MOUSEMOTION:
                    //ignore this if it happened due to a warp
                    if(!_handleWarpMotion(evt.motion))
                    {
                        mMouseListener->mouseMoved(_packageMouseMotion(evt));

                        //try to keep the mouse inside the window
                        _wrapMousePointer(evt.motion);
                    }
                    break;
                case SDL_MOUSEWHEEL:
                    mMouseListener->mouseMoved(_packageMouseMotion(evt));
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    mMouseListener->mousePressed(evt.button, evt.button.button);
                    break;
                case SDL_MOUSEBUTTONUP:
                    mMouseListener->mouseReleased(evt.button, evt.button.button);
                    break;

                case SDL_KEYDOWN:
                    _handleKeyPress(evt.key);
                    break;
                case SDL_KEYUP:
                    mKeyboardListener->keyReleased(evt.key);
                    break;

                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    mWindowListener->windowFocusChange(true);
                    break;
                case SDL_WINDOWEVENT_FOCUS_LOST:
                    mWindowListener->windowFocusChange(false);
                    break;
                case SDL_WINDOWEVENT_EXPOSED:
                    mWindowListener->windowVisibilityChange(true);
                    break;
                case SDL_WINDOWEVENT_HIDDEN:
                    mWindowListener->windowVisibilityChange(false);
                    break;

                //SDL traps ^C signals, pass it to OGRE.
                case SDL_QUIT:
                    Ogre::Root::getSingleton().queueEndRendering();
                    break;
            }
        }
    }

    bool InputWrapper::isModifierHeld(int mod)
    {
        return SDL_GetModState() & mod;
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
        mGrabPointer = grab;
        SDL_SetWindowGrab(mSDLWindow, grab ? SDL_TRUE : SDL_FALSE);
    }

    /// \brief Set the mouse to relative positioning. Doesn't move the cursor
    ///        and disables mouse acceleration.
    void InputWrapper::setMouseRelative(bool relative)
    {
        if(mMouseRelative == relative)
            return;

        mMouseRelative = relative;

        mWrapPointer = false;

        //eep, wrap the pointer manually if the input driver doesn't support
        //relative positioning natively
        if(SDL_SetRelativeMouseMode(relative ? SDL_TRUE : SDL_FALSE) == -1)
        {
            if(relative)
                mWrapPointer = true;
        }

        //now remove all mouse events using the old setting from the queue
        SDL_PumpEvents();

        SDL_Event dummy[20];
        SDL_PeepEvents(dummy, 20, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEMOTION);
    }

    bool InputWrapper::cursorChanged(const std::string &name)
    {
        CursorMap::const_iterator curs_iter = mCursorMap.find(name);

        //we have this cursor
        if(curs_iter != mCursorMap.end())
        {
            SDL_SetCursor(curs_iter->second);
            return false;
        }
        else
        {
            //they should get back to use with more info
            return true;
        }
    }

    void InputWrapper::cursorVisible(bool visible)
    {
        SDL_ShowCursor(visible ? SDL_TRUE : SDL_FALSE);
    }

    void InputWrapper::receiveCursorInfo(const std::string& name, Ogre::TexturePtr tex, Uint8 size_x, Uint8 size_y, Uint8 hotspot_x, Uint8 hotspot_y)
    {
        _createCursorFromResource(name, tex, size_x, size_y, hotspot_x, hotspot_y);
    }

    /// \brief creates an SDL cursor from an Ogre texture
    void InputWrapper::_createCursorFromResource(const std::string& name, Ogre::TexturePtr tex, Uint8 size_x, Uint8 size_y, Uint8 hotspot_x, Uint8 hotspot_y)
    {
        //get the surfaces set up
        Ogre::HardwarePixelBufferSharedPtr buffer = tex.get()->getBuffer();
        buffer.get()->lock(Ogre::HardwarePixelBuffer::HBL_READ_ONLY);

        std::string tempName = "_" + name + "_processing";

        //we need to copy this to a temporary texture first because the cursors might be in DDS format,
        //and Ogre doesn't have an interface to read DDS
        Ogre::TexturePtr tempTexture = Ogre::TextureManager::getSingleton().createManual(
                tempName,
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::TEX_TYPE_2D,
                size_x, size_y,
                0,
                Ogre::PF_FLOAT16_RGBA,
                Ogre::TU_STATIC);

        tempTexture->getBuffer()->blit(buffer);
        buffer->unlock();

        // now blit to memory
        Ogre::Image destImage;
        tempTexture->convertToImage(destImage);

        SDL_Surface* surf = SDL_CreateRGBSurface(0,size_x,size_y,32,0xFF000000,0x00FF0000,0x0000FF00,0x000000FF);


        //copy the Ogre texture to an SDL surface
        for(size_t x = 0; x < size_x; ++x)
        {
            for(size_t y = 0; y < size_y; ++y)
            {
                Ogre::ColourValue clr = destImage.getColourAt(x, y, 0);

                //set the pixel on the SDL surface to the same value as the Ogre texture's
                _putPixel(surf, x, y, SDL_MapRGBA(surf->format, clr.r*255, clr.g*255, clr.b*255, clr.a*255));
            }
        }

        //set the cursor and store it for later
        SDL_Cursor* curs = SDL_CreateColorCursor(surf, hotspot_x, hotspot_y);
        SDL_SetCursor(curs);
        mCursorMap.insert(CursorMap::value_type(std::string(name), curs));

        //clean up
        SDL_FreeSurface(surf);
        Ogre::TextureManager::getSingleton().remove(tempName);
    }

    void InputWrapper::_putPixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
    {
        int bpp = surface->format->BytesPerPixel;
        /* Here p is the address to the pixel we want to set */
        Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

        switch(bpp) {
        case 1:
            *p = pixel;
            break;

        case 2:
            *(Uint16 *)p = pixel;
            break;

        case 3:
            if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                p[0] = (pixel >> 16) & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = pixel & 0xff;
            } else {
                p[0] = pixel & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = (pixel >> 16) & 0xff;
            }
            break;

        case 4:
            *(Uint32 *)p = pixel;
            break;
        }
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

        const int FUDGE_FACTOR_X = width / 4;
        const int FUDGE_FACTOR_Y = height / 4;

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
        }
        else if(evt.type == SDL_MOUSEWHEEL)
        {
            mMouseZ += pack_evt.zrel = (evt.wheel.y * 120);
            pack_evt.z = mMouseZ;
        }
        else
        {
            throw new std::runtime_error("Tried to package non-motion event!");
        }

        return pack_evt;
    }

    void InputWrapper::_handleKeyPress(SDL_KeyboardEvent &evt)
    {
        //SDL keyboard events are followed by the actual text those keys would generate
        //to account for languages that require multiple keystrokes to produce a key.
        //Look for an event immediately following ours, assuming each key produces exactly
        //one character or none at all.

        //TODO: Check if this works properly for multibyte symbols
        //do we have to worry about endian-ness?
        //for that matter, check if we even need to do any of this.

        SDL_Event text_evts[1];
        if(SDL_PeepEvents(text_evts, 1, SDL_GETEVENT, SDL_TEXTINPUT, SDL_TEXTINPUT) != 0)
        {
            if(strlen(text_evts[0].text.text) != 0)
            {
                const unsigned char* symbol = reinterpret_cast<unsigned char*>(&(text_evts[0].text.text[0]));
                evt.keysym.unicode = _UTF8ToUTF32(symbol);
            }
        }

        mKeyboardListener->keyPressed(evt);
    }

    //Lifted from OIS' LinuxKeyboard.cpp
    Uint32 InputWrapper::_UTF8ToUTF32(const unsigned char *buf)
    {
        unsigned char FirstChar = buf[0];

        //it's an ascii char, bail out early.
        if(FirstChar < 128)
            return FirstChar;

        Uint32 val = 0;
        Sint32 len = 0;

        if((FirstChar & 0xE0) == 0xC0) //2 Chars
        {
            len = 2;
            val = FirstChar & 0x1F;
        }
        else if((FirstChar & 0xF0) == 0xE0) //3 Chars
        {
            len = 3;
            val = FirstChar & 0x0F;
        }
        else if((FirstChar & 0xF8) == 0xF0) //4 Chars
        {
            len = 4;
            val = FirstChar & 0x07;
        }
        else if((FirstChar & 0xFC) == 0xF8) //5 Chars
        {
            len = 5;
            val = FirstChar & 0x03;
        }
        else // if((FirstChar & 0xFE) == 0xFC) //6 Chars
        {
            len = 6;
            val = FirstChar & 0x01;
        }

        for(int i = 1; i < len; i++)
            val = (val << 6) | (buf[i] & 0x3F);

        return val;
    }

    OIS::KeyCode InputWrapper::sdl2OISKeyCode(SDL_Keycode code)
    {
        OIS::KeyCode kc = OIS::KC_UNASSIGNED;

        KeyMap::const_iterator ois_equiv = mKeyMap.find(code);

        if(ois_equiv != mKeyMap.end())
            kc = ois_equiv->second;
        else
            std::cerr << "Couldn't find OIS key for " << code << std::endl;

        return kc;
    }

    void InputWrapper::_setupOISKeys()
    {
        //lifted from OIS's SDLKeyboard.cpp

        //TODO: Consider switching to scancodes so we
        //can properly support international keyboards
        //look at SDL_GetKeyFromScancode and SDL_GetKeyName
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
        mKeyMap.insert( KeyMap::value_type(SDLK_LCTRL, OIS::KC_LCONTROL));
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
    }
}
