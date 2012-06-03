#ifndef GAME_MWRENDER_COMPOSITORS_H
#define GAME_MWRENDER_COMPOSITORS_H

#include <map>
#include <string>

namespace Ogre
{
    class Viewport;
}

namespace MWRender
{
    typedef std::map < std::string, std::pair<bool, int> > CompositorMap;

    /// \brief Manages a set of compositors for one viewport
    class Compositors
    {
    public:
        Compositors(Ogre::Viewport* vp);
        virtual ~Compositors();

        /**
         *  enable or disable all compositors globally
         */
        void setEnabled (const bool enabled);

        void setViewport(Ogre::Viewport* vp) { mViewport = vp; }

        /// recreate compositors (call this after viewport size changes)
        void recreate();

        bool toggle() { setEnabled(!mEnabled); return mEnabled; }

        /**
         * enable or disable a specific compositor
         * @note enable has no effect if all compositors are globally disabled
         */
        void setCompositorEnabled (const std::string& name, const bool enabled);

        /**
         * @param name of compositor
         * @param priority, lower number will be first in the chain
         */
        void addCompositor (const std::string& name, const int priority);

        void removeAll ();

    protected:
        /// maps compositor name to its "enabled" state
        CompositorMap mCompositors;

        bool mEnabled;

        Ogre::Viewport* mViewport;
    };

}

#endif
