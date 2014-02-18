#ifndef COMPONENTS_TERRAIN_H
#define COMPONENTS_TERRAIN_H

#include <OgreHardwareIndexBuffer.h>
#include <OgreHardwareVertexBuffer.h>
#include <OgreAxisAlignedBox.h>
#include <OgreTexture.h>
#include <OgreWorkQueue.h>

#include "defs.hpp"

namespace Ogre
{
    class Camera;
}

namespace Terrain
{

    class QuadTreeNode;
    class Storage;

    /**
     * @brief A quadtree-based terrain implementation suitable for large data sets. \n
     *        Near cells are rendered with alpha splatting, distant cells are merged
     *        together in batches and have their layers pre-rendered onto a composite map. \n
     *        Cracks at LOD transitions are avoided using stitching.
     * @note  Multiple cameras are not supported yet
     */
    class World : public Ogre::WorkQueue::RequestHandler, public Ogre::WorkQueue::ResponseHandler
    {
    public:
        /// @note takes ownership of \a storage
        /// @param sceneMgr scene manager to use
        /// @param storage Storage instance to get terrain data from (heights, normals, colors, textures..)
        /// @param visbilityFlags visibility flags for the created meshes
        /// @param distantLand Whether to draw all of the terrain, or only a 3x3 grid around the camera.
        ///         This is a temporary option until it can be streamlined.
        /// @param shaders Whether to use splatting shader, or multi-pass fixed function splatting. Shader is usually
        ///         faster so this is just here for compatibility.
        /// @param align The align of the terrain, see Alignment enum
        /// @param minBatchSize Minimum size of a terrain batch along one side (in cell units). Used for building the quad tree.
        /// @param maxBatchSize Maximum size of a terrain batch along one side (in cell units). Used when traversing the quad tree.
        World(Ogre::SceneManager* sceneMgr,
                Storage* storage, int visiblityFlags, bool distantLand, bool shaders, Alignment align, float minBatchSize, float maxBatchSize);
        ~World();

        bool getDistantLandEnabled() { return mDistantLand; }
        bool getShadersEnabled() { return mShaders; }
        bool getShadowsEnabled() { return mShadows; }
        bool getSplitShadowsEnabled() { return mSplitShadows; }

        float getHeightAt (const Ogre::Vector3& worldPos);

        /// Update chunk LODs according to this camera position
        /// @note Calling this method might lead to composite textures being rendered, so it is best
        /// not to call it when render commands are still queued, since that would cause a flush.
        void update (const Ogre::Vector3& cameraPos);

        /// Get the world bounding box of a chunk of terrain centered at \a center
        Ogre::AxisAlignedBox getWorldBoundingBox (const Ogre::Vector2& center);

        Ogre::SceneManager* getSceneManager() { return mSceneMgr; }

        Ogre::SceneNode* getRootSceneNode() { return mRootSceneNode; }

        Storage* getStorage() { return mStorage; }

        /// Show or hide the whole terrain
        /// @note this setting will be invalidated once you call Terrain::update, so do not call it while the terrain should be hidden
        void setVisible(bool visible);
        bool getVisible();

        /// Recreate materials used by terrain chunks. This should be called whenever settings of
        /// the material factory are changed. (Relying on the factory to update those materials is not
        /// enough, since turning a feature on/off can change the number of texture units available for layer/blend
        /// textures, and to properly respond to this we may need to change the structure of the material, such as
        /// adding or removing passes. This can only be achieved by a full rebuild.)
        void applyMaterials(bool shadows, bool splitShadows);

        int getVisiblityFlags() { return mVisibilityFlags; }

        int getMaxBatchSize() { return mMaxBatchSize; }

        void enableSplattingShader(bool enabled);

        Alignment getAlign() { return mAlign; }

        /// Wait until all background loading is complete.
        void syncLoad();

    private:
        // Called from a background worker thread
        Ogre::WorkQueue::Response* handleRequest(const Ogre::WorkQueue::Request* req, const Ogre::WorkQueue* srcQ);
        // Called from the main thread
        void handleResponse(const Ogre::WorkQueue::Response* res, const Ogre::WorkQueue* srcQ);
        Ogre::uint16 mWorkQueueChannel;

        bool mDistantLand;
        bool mShaders;
        bool mShadows;
        bool mSplitShadows;
        bool mVisible;
        Alignment mAlign;

        QuadTreeNode* mRootNode;
        Ogre::SceneNode* mRootSceneNode;
        Storage* mStorage;

        int mVisibilityFlags;

        /// The number of chunks currently loading in a background thread. If 0, we have finished loading!
        int mChunksLoading;

        Ogre::SceneManager* mSceneMgr;
        Ogre::SceneManager* mCompositeMapSceneMgr;

        /// Bounds in cell units
        float mMinX, mMaxX, mMinY, mMaxY;

        /// Minimum size of a terrain batch along one side (in cell units)
        float mMinBatchSize;
        /// Maximum size of a terrain batch along one side (in cell units)
        float mMaxBatchSize;

        void buildQuadTree(QuadTreeNode* node);

    public:
        // ----INTERNAL----

        enum IndexBufferFlags
        {
            IBF_North = 1 << 0,
            IBF_East  = 1 << 1,
            IBF_South = 1 << 2,
            IBF_West  = 1 << 3
        };

        /// @param flags first 4*4 bits are LOD deltas on each edge, respectively (4 bits each)
        ///              next 4 bits are LOD level of the index buffer (LOD 0 = don't omit any vertices)
        /// @param numIndices number of indices that were used will be written here
        Ogre::HardwareIndexBufferSharedPtr getIndexBuffer (int flags, size_t& numIndices);

        Ogre::HardwareVertexBufferSharedPtr getVertexBuffer (int numVertsOneSide);

        Ogre::SceneManager* getCompositeMapSceneManager() { return mCompositeMapSceneMgr; }

        // Delete all quads
        void clearCompositeMapSceneManager();
        void renderCompositeMap (Ogre::TexturePtr target);

        // Convert the given position from Z-up align, i.e. Align_XY to the wanted align set in mAlign
        void convertPosition (float& x, float& y, float& z);
        void convertPosition (Ogre::Vector3& pos);
        void convertBounds (Ogre::AxisAlignedBox& bounds);

        // Adds a WorkQueue request to load a chunk for this node in the background.
        void queueLoad (QuadTreeNode* node);

    private:
        // Index buffers are shared across terrain batches where possible. There is one index buffer for each
        // combination of LOD deltas and index buffer LOD we may need.
        std::map<int, Ogre::HardwareIndexBufferSharedPtr> mIndexBufferMap;

        std::map<int, Ogre::HardwareVertexBufferSharedPtr> mUvBufferMap;

        Ogre::RenderTarget* mCompositeMapRenderTarget;
        Ogre::TexturePtr mCompositeMapRenderTexture;
    };

    struct LoadRequestData
    {
        QuadTreeNode* mNode;
        bool mPack;

        friend std::ostream& operator<<(std::ostream& o, const LoadRequestData& r)
        { return o; }
    };

    struct LoadResponseData
    {
        std::vector<float> mPositions;
        std::vector<float> mNormals;
        std::vector<Ogre::uint8> mColours;
        // Since we can't create a texture from a different thread, this only holds the raw texel data
        std::vector< std::vector<Ogre::uint8> > mBlendmaps;
        std::vector<Terrain::LayerInfo> mLayerList;

        unsigned long time;

        friend std::ostream& operator<<(std::ostream& o, const LoadResponseData& r)
        { return o; }
    };

}

#endif
