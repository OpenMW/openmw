#ifndef _SDL4OGRE_CURSORMANAGER_H
#define _SDL4OGRE_CURSORMANAGER_H

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
        virtual void cursorVisibilityChange(bool visible);

    private:
        void _createCursorFromResource(const std::string &name, int rotDegrees, Ogre::TexturePtr tex, Uint8 size_x, Uint8 size_y, Uint8 hotspot_x, Uint8 hotspot_y);
        void _putPixel(SDL_Surface *surface, int x, int y, Uint32 pixel);

        void _setGUICursor(const std::string& name);
        void _setCursorVisible(bool visible);

        typedef std::map<std::string, SDL_Cursor*> CursorMap;
        CursorMap mCursorMap;

        SDL_Cursor* mDebugCursor;
        std::string mCurrentCursor;
        bool mEnabled;
        bool mInitialized;
        bool mCursorVisible;
    };
}

#endif
