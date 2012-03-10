#ifndef _GAME_RENDER_LOCALMAP_H
#define _GAME_RENDER_LOCALMAP_H

#include "../mwworld/ptr.hpp"

#include <openengine/ogre/renderer.hpp>

namespace MWRender
{
    ///
    /// \brief Local map rendering
    ///
    class LocalMap
    {
    public:
        LocalMap(OEngine::Render::OgreRenderer*);

        /**
         * Request the local map for an exterior cell.
         * It will either be loaded from a disk cache,
         * or rendered if it is not already cached.
         * @param exterior cell
         */
        void requestMap (MWWorld::Ptr::CellStore* cell);

        /**
         * Request the local map for an interior cell.
         * It will either be loaded from a disk cache,
         * or rendered if it is not already cached.
         * @param interior cell
         * @param bounding box of the cell
         */
        void requestMap (MWWorld::Ptr::CellStore* cell,
                        Ogre::AxisAlignedBox bounds);

    private:
        OEngine::Render::OgreRenderer* mRendering;

        Ogre::Camera* mCellCamera;

        void render(const float x, const float y,
                    const float zlow, const float zhigh,
                    const float xw, const float yw,
                    const std::string& texture);
    };

}
#endif
