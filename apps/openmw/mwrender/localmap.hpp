#ifndef GAME_RENDER_LOCALMAP_H
#define GAME_RENDER_LOCALMAP_H

#include <set>
#include <vector>
#include <map>

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
         * Request a map render for the given cell. Render textures will be immediately created and can be retrieved with the getMapTexture function.
         */
        void requestMap (const MWWorld::CellStore* cell);

        void addCell(MWWorld::CellStore* cell);

        void removeCell (MWWorld::CellStore* cell);

        osg::ref_ptr<osg::Texture2D> getMapTexture (int x, int y);

        osg::ref_ptr<osg::Texture2D> getFogOfWarTexture (int x, int y);

        void removeCamera(osg::Camera* cam);

        /**
         * Indicates a camera has been queued for rendering and can be cleaned up in the next frame. For internal use only.
         */
        void markForRemoval(osg::Camera* cam);

        /**
         * Removes cameras that have already been rendered. Should be called every frame to ensure that
         * we do not render the same map more than once. Note, this cleanup is difficult to implement in an
         * automated fashion, since we can't alter the scene graph structure from within an update callback.
         */
        void cleanupCameras();

        /**
         * Set the position & direction of the player, and returns the position in map space through the reference parameters.
         * @remarks This is used to draw a "fog of war" effect
         * to hide areas on the map the player has not discovered yet.
         */
        void updatePlayer (const osg::Vec3f& position, const osg::Quat& orientation,
                           float& u, float& v, int& x, int& y, osg::Vec3f& direction);

        /**
         * Save the fog of war for this cell to its CellStore.
         * @remarks This should be called when unloading a cell, and for all active cells prior to saving the game.
         */
        void saveFogOfWar(MWWorld::CellStore* cell);

        /**
         * Get the interior map texture index and normalized position on this texture, given a world position
         */
        void worldToInteriorMapPosition (osg::Vec2f pos, float& nX, float& nY, int& x, int& y);

        osg::Vec2f interiorMapToWorldPosition (float nX, float nY, int x, int y);

        /**
         * Check if a given position is explored by the player (i.e. not obscured by fog of war)
         */
        bool isPositionExplored (float nX, float nY, int x, int y);

        osg::Group* getRoot();

    private:
        osg::ref_ptr<osg::Group> mRoot;
        osg::ref_ptr<osg::Node> mSceneRoot;

        typedef std::vector< osg::ref_ptr<osg::Camera> > CameraVector;

        CameraVector mActiveCameras;

        CameraVector mCamerasPendingRemoval;

        typedef std::set<std::pair<int, int> > Grid;
        Grid mCurrentGrid;

        struct MapSegment
        {
            MapSegment();
            ~MapSegment();

            void initFogOfWar();
            void loadFogOfWar(const ESM::FogTexture& fog);
            void saveFogOfWar(ESM::FogTexture& fog) const;
            void createFogOfWarTexture();

            osg::ref_ptr<osg::Texture2D> mMapTexture;
            osg::ref_ptr<osg::Texture2D> mFogOfWarTexture;
            osg::ref_ptr<osg::Image> mFogOfWarImage;

            Grid mGrid; // the grid that was active at the time of rendering this segment

            bool mHasFogState;
        };

        typedef std::map<std::pair<int, int>, MapSegment> SegmentMap;
        SegmentMap mSegments;

        int mMapResolution;

        // the dynamic texture is a bottleneck, so don't set this too high
        static const int sFogOfWarResolution = 32;

        // size of a map segment (for exteriors, 1 cell)
        float mMapWorldSize;

        int mCellDistance;

        float mAngle;
        const osg::Vec2f rotatePoint(const osg::Vec2f& point, const osg::Vec2f& center, const float angle);

        void requestExteriorMap(const MWWorld::CellStore* cell);
        void requestInteriorMap(const MWWorld::CellStore* cell);

        osg::ref_ptr<osg::Camera> createOrthographicCamera(float left, float top, float width, float height, const osg::Vec3d& upVector, float zmin, float zmax);
        void setupRenderToTexture(osg::ref_ptr<osg::Camera> camera, int x, int y);

        bool mInterior;
        osg::BoundingBox mBounds;
    };

}
#endif
