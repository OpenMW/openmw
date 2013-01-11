#ifndef _MWINPUT_SDLINPUTWRAPPER_H
#define _MWINPUT_SDLINPUTWRAPPER_H

#include "SDL2/SDL_events.h"
#include <OGRE/OgreRenderWindow.h>
#include <boost/unordered_map.hpp>

#include "OISCompat.h"
#include "events.h"


namespace Ogre
{
    class Texture;
}

namespace SFO
{


    class CursorChangeClient
    {
    public:
        /// \brief Tell the client that the cursor has changed, giving the
        ///        name of the cursor we changed to ("arrow", "ibeam", etc)
        /// \return Whether the client is interested in more information about the cursor
        virtual bool cursorChanged(const std::string &name) = 0;

        /// \brief Follow up a cursorChanged() call with enough info to create an SDL cursor.
        virtual void receiveCursorInfo(const std::string &name, Ogre::TexturePtr tex, Uint8 size_x, Uint8 size_y, Uint8 hotspot_x, Uint8 hotspot_y) = 0;

        /// \brief Tell the client when the cursor visibility changed
        virtual void cursorVisible(bool visible) = 0;
    };

    class InputWrapper :
            public CursorChangeClient
    {
    public:
        InputWrapper(Ogre::RenderWindow* window);
        virtual ~InputWrapper();

        void setMouseEventCallback(MouseListener* listen) { mMouseListener = listen; }
        void setKeyboardEventCallback(KeyListener* listen) { mKeyboardListener = listen; }
        void setWindowEventCallback(WindowListener* listen) { mWindowListener = listen; }

        void capture();
        bool isModifierHeld(int mod);

        void setMouseRelative(bool relative);
        bool getMouseRelative() { return mMouseRelative; }
        void setGrabPointer(bool grab);

        virtual bool cursorChanged(const std::string &name);
        virtual void receiveCursorInfo(const std::string &name, Ogre::TexturePtr tex, Uint8 size_x, Uint8 size_y, Uint8 hotspot_x, Uint8 hotspot_y);
        virtual void cursorVisible(bool visible);

        OIS::KeyCode sdl2OISKeyCode(SDL_Keycode code);

        void warpMouse(int x, int y);

    private:
        bool _start();

        bool _handleWarpMotion(const SDL_MouseMotionEvent& evt);
        void _wrapMousePointer(const SDL_MouseMotionEvent &evt);
        MouseMotionEvent _packageMouseMotion(const SDL_Event& evt);

        void _createCursorFromResource(const std::string &name, Ogre::TexturePtr tex, Uint8 size_x, Uint8 size_y, Uint8 hotspot_x, Uint8 hotspot_y);
        void _putPixel(SDL_Surface *surface, int x, int y, Uint32 pixel);

        void _handleKeyPress(SDL_KeyboardEvent& evt);
        Uint32 _UTF8ToUTF32(const unsigned char *buf);
        void _setupOISKeys();

        SFO::MouseListener* mMouseListener;
        SFO::KeyListener* mKeyboardListener;
        SFO::WindowListener* mWindowListener;

        typedef boost::unordered_map<SDL_Keycode, OIS::KeyCode> KeyMap;
        KeyMap mKeyMap;

        typedef std::map<std::string, SDL_Cursor*> CursorMap;
        CursorMap mCursorMap;

        Uint16 mWarpX;
        Uint16 mWarpY;
        bool mWarpCompensate;
        bool mMouseRelative;
        bool mWrapPointer;
        bool mGrabPointer;

        Sint32 mMouseZ;
        Sint32 mMouseX;
        Sint32 mMouseY;

        Ogre::RenderWindow* mWindow;
        SDL_Window* mSDLWindow;
    };

}

#endif
