#ifndef GAME_RENDER_LOCALMAP_H
#define GAME_RENDER_LOCALMAP_H

#include <cstdint>
#include <map>
#include <set>
#include <vector>

#include <MyGUI_Types.h>
#include <osg/BoundingBox>
#include <osg/Quat>
#include <osg/ref_ptr>

namespace MWWorld
{
    class CellStore;
}

namespace ESM
{
    struct FogTexture;
}

namespace osg
{
    class Texture2D;
    class Image;
    class Camera;
    class Group;
    class Node;
}

namespace MWRender
{
    class LocalMapRenderToTexture;

    ///
    /// \brief Local map rendering
    ///
    class LocalMap
    {
    public:
        LocalMap(osg::Group* root);
        ~LocalMap();

        /**
         * Clear all savegame-specific data (i.e. fog of war textures)
         */
        void clear();

        /**
         * Request a map render for the given cell. Render textures will be immediately created and can be retrieved
         * with the getMapTexture function.
         */
        void requestMap(const MWWorld::CellStore* cell);

        void addCell(MWWorld::CellStore* cell);
        void removeExteriorCell(int x, int y);

        void removeCell(MWWorld::CellStore* cell);

        osg::ref_ptr<osg::Texture2D> getMapTexture(int x, int y);

        osg::ref_ptr<osg::Texture2D> getFogOfWarTexture(int x, int y);

        /**
         * Removes cameras that have already been rendered. Should be called every frame to ensure that
         * we do not render the same map more than once. Note, this cleanup is difficult to implement in an
         * automated fashion, since we can't alter the scene graph structure from within an update callback.
         */
        void cleanupCameras();

        /**
         * Set the position & direction of the player, and returns the position in map space through the reference
         * parameters.
         * @remarks This is used to draw a "fog of war" effect
         * to hide areas on the map the player has not discovered yet.
         */
        void updatePlayer(const osg::Vec3f& position, const osg::Quat& orientation, float& u, float& v, int& x, int& y,
            osg::Vec3f& direction);

        /**
         * Save the fog of war for this cell to its CellStore.
         * @remarks This should be called when unloading a cell, and for all active cells prior to saving the game.
         */
        void saveFogOfWar(MWWorld::CellStore* cell);

        /**
         * Get the interior map texture index and normalized position on this texture, given a world position
         */
        void worldToInteriorMapPosition(osg::Vec2f pos, float& nX, float& nY, int& x, int& y);

        osg::Vec2f interiorMapToWorldPosition(float nX, float nY, int x, int y);

        /**
         * Check if a given position is explored by the player (i.e. not obscured by fog of war)
         */
        bool isPositionExplored(float nX, float nY, int x, int y);

        osg::Group* getRoot();

        MyGUI::IntRect getInteriorGrid() const;

    private:
        osg::ref_ptr<osg::Group> mRoot;
        osg::ref_ptr<osg::Node> mSceneRoot;

        typedef std::vector<osg::ref_ptr<LocalMapRenderToTexture>> RTTVector;
        RTTVector mLocalMapRTTs;

        enum NeighbourCellFlag : std::uint8_t
        {
            NeighbourCellTopLeft = 1,
            NeighbourCellTopCenter = 1 << 1,
            NeighbourCellTopRight = 1 << 2,
            NeighbourCellMiddleLeft = 1 << 3,
            NeighbourCellMiddleRight = 1 << 4,
            NeighbourCellBottomLeft = 1 << 5,
            NeighbourCellBottomCenter = 1 << 6,
            NeighbourCellBottomRight = 1 << 7,
        };

        struct MapSegment
        {
            void initFogOfWar();
            void loadFogOfWar(const ESM::FogTexture& fog);
            void saveFogOfWar(ESM::FogTexture& fog) const;
            void createFogOfWarTexture();

            std::uint8_t mLastRenderNeighbourFlags = 0;
            bool mHasFogState = false;
            osg::ref_ptr<osg::Texture2D> mMapTexture;
            osg::ref_ptr<osg::Texture2D> mFogOfWarTexture;
            osg::ref_ptr<osg::Image> mFogOfWarImage;
        };

        typedef std::map<std::pair<int, int>, MapSegment> SegmentMap;
        SegmentMap mExteriorSegments;
        SegmentMap mInteriorSegments;

        int mMapResolution;

        // the dynamic texture is a bottleneck, so don't set this too high
        static const int sFogOfWarResolution = 32;

        // size of a map segment (for exteriors, 1 cell)
        float mMapWorldSize;

        int mCellDistance;

        float mAngle;
        const osg::Vec2f rotatePoint(const osg::Vec2f& point, const osg::Vec2f& center, const float angle);

        void requestExteriorMap(const MWWorld::CellStore* cell, MapSegment& segment);
        void requestInteriorMap(const MWWorld::CellStore* cell);

        void setupRenderToTexture(
            int segmentX, int segmentY, float left, float top, const osg::Vec3d& upVector, float zmin, float zmax);

        osg::BoundingBox mBounds;
        osg::Vec2f mCenter;
        bool mInterior;

        std::uint8_t getExteriorNeighbourFlags(int cellX, int cellY) const;
    };

}
#endif
