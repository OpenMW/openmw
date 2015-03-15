/*
 * Copyright (c) 2015 scrawl <scrawl@baseoftrash.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef COMPONENTS_TERRAIN_H
#define COMPONENTS_TERRAIN_H

#include <OgreAxisAlignedBox.h>
#include <OgreTexture.h>
#include <OgreWorkQueue.h>

#include "world.hpp"

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
    class DefaultWorld : public World, public Ogre::WorkQueue::RequestHandler, public Ogre::WorkQueue::ResponseHandler
    {
    public:
        /// @note takes ownership of \a storage
        /// @param sceneMgr scene manager to use
        /// @param storage Storage instance to get terrain data from (heights, normals, colors, textures..)
        /// @param visbilityFlags visibility flags for the created meshes
        /// @param shaders Whether to use splatting shader, or multi-pass fixed function splatting. Shader is usually
        ///         faster so this is just here for compatibility.
        /// @param align The align of the terrain, see Alignment enum
        /// @param minBatchSize Minimum size of a terrain batch along one side (in cell units). Used for building the quad tree.
        /// @param maxBatchSize Maximum size of a terrain batch along one side (in cell units). Used when traversing the quad tree.
        DefaultWorld(Ogre::SceneManager* sceneMgr,
                Storage* storage, int visibilityFlags, bool shaders, Alignment align, float minBatchSize, float maxBatchSize);
        ~DefaultWorld();

        /// Update chunk LODs according to this camera position
        /// @note Calling this method might lead to composite textures being rendered, so it is best
        /// not to call it when render commands are still queued, since that would cause a flush.
        virtual void update (const Ogre::Vector3& cameraPos);

        /// Get the world bounding box of a chunk of terrain centered at \a center
        virtual Ogre::AxisAlignedBox getWorldBoundingBox (const Ogre::Vector2& center);

        Ogre::SceneNode* getRootSceneNode() { return mRootSceneNode; }

        /// Show or hide the whole terrain
        /// @note this setting will be invalidated once you call Terrain::update, so do not call it while the terrain should be hidden
        virtual void setVisible(bool visible);
        virtual bool getVisible();

        /// Recreate materials used by terrain chunks. This should be called whenever settings of
        /// the material factory are changed. (Relying on the factory to update those materials is not
        /// enough, since turning a feature on/off can change the number of texture units available for layer/blend
        /// textures, and to properly respond to this we may need to change the structure of the material, such as
        /// adding or removing passes. This can only be achieved by a full rebuild.)
        virtual void applyMaterials(bool shadows, bool splitShadows);

        int getMaxBatchSize() { return static_cast<int>(mMaxBatchSize); }

        /// Wait until all background loading is complete.
        void syncLoad();

    private:
        // Called from a background worker thread
        virtual Ogre::WorkQueue::Response* handleRequest(const Ogre::WorkQueue::Request* req, const Ogre::WorkQueue* srcQ);
        // Called from the main thread
        virtual void handleResponse(const Ogre::WorkQueue::Response* res, const Ogre::WorkQueue* srcQ);
        Ogre::uint16 mWorkQueueChannel;

        bool mVisible;

        QuadTreeNode* mRootNode;
        Ogre::SceneNode* mRootSceneNode;

        /// The number of chunks currently loading in a background thread. If 0, we have finished loading!
        int mChunksLoading;

        Ogre::SceneManager* mCompositeMapSceneMgr;

        /// Bounds in cell units
        float mMinX, mMaxX, mMinY, mMaxY;

        /// Minimum size of a terrain batch along one side (in cell units)
        float mMinBatchSize;
        /// Maximum size of a terrain batch along one side (in cell units)
        float mMaxBatchSize;

        void buildQuadTree(QuadTreeNode* node, std::vector<QuadTreeNode*>& leafs);

        // Are layers for leaf nodes loaded? This is done once at startup (but in a background thread)
        bool mLayerLoadPending;

    public:
        // ----INTERNAL----
        Ogre::SceneManager* getCompositeMapSceneManager() { return mCompositeMapSceneMgr; }

        bool areLayersLoaded() { return !mLayerLoadPending; }

        // Delete all quads
        void clearCompositeMapSceneManager();
        void renderCompositeMap (Ogre::TexturePtr target);

        // Adds a WorkQueue request to load a chunk for this node in the background.
        void queueLoad (QuadTreeNode* node);

    private:
        Ogre::RenderTarget* mCompositeMapRenderTarget;
        Ogre::TexturePtr mCompositeMapRenderTexture;
    };

    struct LoadRequestData
    {
        QuadTreeNode* mNode;

        friend std::ostream& operator<<(std::ostream& o, const LoadRequestData& r)
        { return o; }
    };

    struct LoadResponseData
    {
        std::vector<float> mPositions;
        std::vector<float> mNormals;
        std::vector<Ogre::uint8> mColours;

        friend std::ostream& operator<<(std::ostream& o, const LoadResponseData& r)
        { return o; }
    };

    struct LayersRequestData
    {
        std::vector<QuadTreeNode*> mNodes;
        bool mPack;

        friend std::ostream& operator<<(std::ostream& o, const LayersRequestData& r)
        { return o; }
    };

    struct LayersResponseData
    {
        std::vector<LayerCollection> mLayerCollections;

        friend std::ostream& operator<<(std::ostream& o, const LayersResponseData& r)
        { return o; }
    };

}

#endif
