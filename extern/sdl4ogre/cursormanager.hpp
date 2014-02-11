#ifndef SDL4OGRE_CURSOR_MANAGER_H
#define SDL4OGRE_CURSOR_MANAGER_H

#include <SDL_types.h>
#include <string>

#include <OgreTexture.h>
#include <OgrePrerequisites.h>

namespace SFO
{
class CursorManager
{
public:
    virtual ~CursorManager(){}

    /// \brief Tell the manager that the cursor has changed, giving the
    ///        name of the cursor we changed to ("arrow", "ibeam", etc)
    /// \return Whether the manager is interested in more information about the cursor
    virtual bool cursorChanged(const std::string &name) = 0;

    /// \brief Follow up a cursorChanged() call with enough info to create an cursor.
    virtual void receiveCursorInfo(const std::string &name, int rotDegrees, Ogre::TexturePtr tex, Uint8 size_x, Uint8 size_y, Uint8 hotspot_x, Uint8 hotspot_y) = 0;

    /// \brief sets whether to actively manage cursors or not
    virtual void setEnabled(bool enabled) = 0;
};
}

#endif
