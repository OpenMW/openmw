#ifndef _GAME_RENDER_LOCALMAP_H
#define _GAME_RENDER_LOCALMAP_H

#include "../mwworld/ptr.hpp"

#include <openengine/ogre/renderer.hpp>

namespace MWWorld
{
    class Environment;
}

namespace MWRender
{
    ///
    /// \brief Local map rendering
    ///
    class LocalMap
    {
    public:
        LocalMap(OEngine::Render::OgreRenderer*, MWWorld::Environment* env);
        ~LocalMap();

        /**
         * Request the local map for an exterior cell.
         * @remarks It will either be loaded from a disk cache,
         * or rendered if it is not already cached.
         * @param exterior cell
         */
        void requestMap (MWWorld::Ptr::CellStore* cell);

        /**
         * Request the local map for an interior cell.
         * @remarks It will either be loaded from a disk cache,
         * or rendered if it is not already cached.
         * @param interior cell
         * @param bounding box of the cell
         */
        void requestMap (MWWorld::Ptr::CellStore* cell,
                        Ogre::AxisAlignedBox bounds);

        /**
         * Set the position of the player.
         * @remarks This is used to draw a "fog of war" effect
         * to hide areas on the map the player has not discovered yet.
         * @param position (OGRE coordinates)
         */
        void setPlayerPosition (const Ogre::Vector3& position);

        /**
         * Save the fog of war for the current cell to disk.
         * @remarks This should be called before loading a
         * new cell, as well as when the game is quit.
         * @param current cell
         */
        void saveFogOfWar(MWWorld::Ptr::CellStore* cell);

    private:
        OEngine::Render::OgreRenderer* mRendering;
        MWWorld::Environment* mEnvironment;

        Ogre::Camera* mCellCamera;

        void render(const float x, const float y,
                    const float zlow, const float zhigh,
                    const float xw, const float yw,
                    const std::string& texture);

        void saveTexture(const std::string& texname, const std::string& filename);

        std::string coordStr(const int x, const int y);

        // a buffer for the "fog of war" texture of the current cell.
        // interior cells could be divided into multiple textures,
        // so we store in a map.
        std::map <std::string, Ogre::uint32*> mBuffers;

        void deleteBuffers();

        bool mInterior;
        int mCellX, mCellY;
        Ogre::AxisAlignedBox mBounds;
        std::string mInteriorName;
    };

}
#endif
