#ifndef SDL4OGRE_CURSORMANAGER_H
#define SDL4OGRE_CURSORMANAGER_H

#include <SDL.h>

#include "cursormanager.hpp"
#include <map>

namespace SFO
{
    class SDLCursorManager :
            public CursorManager
    {
    public:
        SDLCursorManager();
        virtual ~SDLCursorManager();

        virtual void setEnabled(bool enabled);

        virtual bool cursorChanged(const std::string &name);
        virtual void receiveCursorInfo(const std::string &name, int rotDegrees, Ogre::TexturePtr tex, Uint8 size_x, Uint8 size_y, Uint8 hotspot_x, Uint8 hotspot_y);

    private:
        void _createCursorFromResource(const std::string &name, int rotDegrees, Ogre::TexturePtr tex, Uint8 size_x, Uint8 size_y, Uint8 hotspot_x, Uint8 hotspot_y);
        void _putPixel(SDL_Surface *surface, int x, int y, Uint32 pixel);

        void _setGUICursor(const std::string& name);

        typedef std::map<std::string, SDL_Cursor*> CursorMap;
        CursorMap mCursorMap;

        std::string mCurrentCursor;
        bool mEnabled;
        bool mInitialized;
    };
}

#endif
