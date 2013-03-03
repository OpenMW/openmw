#ifndef _GAME_RENDER_LOCALMAP_H
#define _GAME_RENDER_LOCALMAP_H

#include <openengine/ogre/renderer.hpp>

#include <OgreAxisAlignedBox.h>
#include <OgreColourValue.h>

namespace MWWorld
{
    class CellStore;
}

namespace MWRender
{
    class RenderingManager;

    ///
    /// \brief Local map rendering
    ///
    class LocalMap
    {
    public:
        LocalMap(OEngine::Render::OgreRenderer*, MWRender::RenderingManager* rendering);
        ~LocalMap();

        /**
         * Request the local map for an exterior cell.
         * @remarks It will either be loaded from a disk cache,
         * or rendered if it is not already cached.
         * @param exterior cell
         */
        void requestMap (MWWorld::CellStore* cell);

        /**
         * Request the local map for an interior cell.
         * @remarks It will either be loaded from a disk cache,
         * or rendered if it is not already cached.
         * @param interior cell
         * @param bounding box of the cell
         */
        void requestMap (MWWorld::CellStore* cell,
                        Ogre::AxisAlignedBox bounds);

        /**
         * Set the position & direction of the player.
         * @remarks This is used to draw a "fog of war" effect
         * to hide areas on the map the player has not discovered yet.
         * @param position (OGRE coordinates)
         * @param camera orientation (OGRE coordinates)
         */
        void updatePlayer (const Ogre::Vector3& position, const Ogre::Quaternion& orientation);

        /**
         * Save the fog of war for the current cell to disk.
         * @remarks This should be called before loading a
         * new cell, as well as when the game is quit.
         * @param current cell
         */
        void saveFogOfWar(MWWorld::CellStore* cell);

        /**
         * Get the interior map texture index and normalized position
         * on this texture, given a world position (in ogre coordinates)
         */
        void getInteriorMapPosition (Ogre::Vector2 pos, float& nX, float& nY, int& x, int& y);

        /**
         * Check if a given position is explored by the player (i.e. not obscured by fog of war)
         */
        bool isPositionExplored (float nX, float nY, int x, int y, bool interior);

    private:
        OEngine::Render::OgreRenderer* mRendering;
        MWRender::RenderingManager* mRenderingManager;

        // 1024*1024 pixels for a cell
        static const int sMapResolution = 512;

        // the dynamic texture is a bottleneck, so don't set this too high
        static const int sFogOfWarResolution = 32;

        // frames to skip before rendering fog of war
        static const int sFogOfWarSkip = 2;

        // size of a map segment (for exteriors, 1 cell)
        static const int sSize = 8192;

        Ogre::Camera* mCellCamera;
        Ogre::SceneNode* mCameraNode;
        Ogre::SceneNode* mCameraPosNode;
        Ogre::SceneNode* mCameraRotNode;

        // directional light from a fixed angle
        Ogre::Light* mLight;

        float mAngle;
        const Ogre::Vector2 rotatePoint(const Ogre::Vector2& p, const Ogre::Vector2& c, const float angle);

        void render(const float x, const float y,
                    const float zlow, const float zhigh,
                    const float xw, const float yw,
                    const std::string& texture);

        void saveTexture(const std::string& texname, const std::string& filename);

        std::string coordStr(const int x, const int y);

        // a buffer for the "fog of war" texture of the current cell.
        // interior cells could be divided into multiple textures,
        // so we store in a map.
        std::map <std::string, std::vector<Ogre::uint32> > mBuffers;

        void deleteBuffers();

        bool mInterior;
        int mCellX, mCellY;
        Ogre::AxisAlignedBox mBounds;
        std::string mInteriorName;
    };

}
#endif
