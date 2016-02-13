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
        virtual void cursorChanged(const std::string &name);

        virtual void createCursor(const std::string &name, int rotDegrees, osg::Image* image, Uint8 hotspot_x, Uint8 hotspot_y);

    private:
        void _createCursorFromResource(const std::string &name, int rotDegrees, osg::Image* image, Uint8 hotspot_x, Uint8 hotspot_y);
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
