#ifndef GAME_RENDER_LOCALMAP_H
#define GAME_RENDER_LOCALMAP_H

#include <openengine/ogre/renderer.hpp>

#include <OgreAxisAlignedBox.h>
#include <OgreColourValue.h>
#include <OgreResource.h>

namespace MWWorld
{
    class CellStore;
}

namespace ESM
{
    struct FogTexture;
}

namespace MWRender
{
    class RenderingManager;

    ///
    /// \brief Local map rendering
    ///
    class LocalMap : public Ogre::ManualResourceLoader
    {
    public:
        LocalMap(OEngine::Render::OgreRenderer*, MWRender::RenderingManager* rendering);
        ~LocalMap();

        virtual void loadResource(Ogre::Resource* resource);

        /**
         * Clear all savegame-specific data (i.e. fog of war textures)
         */
        void clear();

        /**
         * Request the local map for an exterior cell.
         * @remarks It will either be loaded from a disk cache,
         * or rendered if it is not already cached.
         * @param cell exterior cell
         * @param zMin min height of objects or terrain in cell
         * @param zMax max height of objects or terrain in cell
         */
        void requestMap (MWWorld::CellStore* cell, float zMin, float zMax);

        /**
         * Request the local map for an interior cell.
         * @remarks It will either be loaded from a disk cache,
         * or rendered if it is not already cached.
         * @param cell interior cell
         * @param bounds bounding box of the cell
         */
        void requestMap (MWWorld::CellStore* cell,
                        Ogre::AxisAlignedBox bounds);

        /**
         * Set the position & direction of the player.
         * @remarks This is used to draw a "fog of war" effect
         * to hide areas on the map the player has not discovered yet.
         */
        void updatePlayer (const Ogre::Vector3& position, const Ogre::Quaternion& orientation);

        /**
         * Save the fog of war for this cell to its CellStore.
         * @remarks This should be called when unloading a cell, and for all active cells prior to saving the game.
         */
        void saveFogOfWar(MWWorld::CellStore* cell);

        /**
         * Get the interior map texture index and normalized position
         * on this texture, given a world position
         */
        void worldToInteriorMapPosition (Ogre::Vector2 pos, float& nX, float& nY, int& x, int& y);

        Ogre::Vector2 interiorMapToWorldPosition (float nX, float nY, int x, int y);

        /**
         * Check if a given position is explored by the player (i.e. not obscured by fog of war)
         */
        bool isPositionExplored (float nX, float nY, int x, int y, bool interior);

    private:
        OEngine::Render::OgreRenderer* mRendering;
        MWRender::RenderingManager* mRenderingManager;

        int mMapResolution;

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

        /// @param force Always render, even if we already have a cached map
        void render(const float x, const float y,
                    const float zlow, const float zhigh,
                    const float xw, const float yw,
                    const std::string& texture, bool force=false);

        // Creates a fog of war texture and initializes it to full black
        void createFogOfWar(const std::string& texturePrefix);

        // Loads a fog of war texture from its ESM struct
        void loadFogOfWar(const std::string& texturePrefix, ESM::FogTexture& esm); // FogTexture not const because MemoryDataStream doesn't accept it

        Ogre::TexturePtr createFogOfWarTexture(const std::string& name);

        std::string coordStr(const int x, const int y);

        // A buffer for the "fog of war" textures of the current cell.
        // Both interior and exterior maps are possibly divided into multiple textures.
        std::map <std::string, std::vector<Ogre::uint32> > mBuffers;

        // The render texture we will use to create the map images
        Ogre::TexturePtr mRenderTexture;
        Ogre::RenderTarget* mRenderTarget;

        bool mInterior;
        Ogre::AxisAlignedBox mBounds;
        std::string mInteriorName;
    };

}
#endif
