#ifndef OPENMW_COMPONENTS_SDLUTIL_SDLCURSORMANAGER_H
#define OPENMW_COMPONENTS_SDLUTIL_SDLCURSORMANAGER_H

#include <map>
#include <string>

#include <SDL_types.h>

struct SDL_Cursor;
struct SDL_Surface;

namespace osg
{
    class Image;
}

namespace SDLUtil
{
    class SDLCursorManager
    {
    public:
        SDLCursorManager();
        virtual ~SDLCursorManager();

        /// \brief sets whether to actively manage cursors or not
        virtual void setEnabled(bool enabled);

        /// \brief Tell the manager that the cursor has changed, giving the
        ///        name of the cursor we changed to ("arrow", "ibeam", etc)
        virtual void cursorChanged(std::string_view name);

        virtual void createCursor(std::string_view name, int rotDegrees, osg::Image* image, Uint8 hotspotX,
            Uint8 hotspotY, int cursorWidth, int cursorHeight);

    private:
        void _createCursorFromResource(std::string_view name, int rotDegrees, osg::Image* image, Uint8 hotspotX,
            Uint8 hotspotY, int cursorWidth, int cursorHeight);
        void _putPixel(SDL_Surface* surface, int x, int y, Uint32 pixel);

        void _setGUICursor(std::string_view name);

        typedef std::map<std::string, SDL_Cursor*, std::less<>> CursorMap;
        CursorMap mCursorMap;

        std::string mCurrentCursor;
        bool mEnabled;
        bool mInitialized;
    };
}

#endif
