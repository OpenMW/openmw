#ifndef COMPONENTS_TERRAIN_WORLD_H
#define COMPONENTS_TERRAIN_WORLD_H

#include <osg/ref_ptr>
#include <osg/Referenced>
#include <osg/Vec3f>
#include <osg/NodeCallback>

#include <atomic>
#include <memory>
#include <set>
#include <atomic>

#include "defs.hpp"
#include "cellborder.hpp"

namespace osg
{
    class Group;
    class Stats;
    class Node;
    class Object;
}

namespace Resource
{
    class ResourceSystem;
}

namespace SceneUtil
{
    class WorkQueue;
}

namespace Terrain
{
    class Storage;

    class TextureManager;
    class ChunkManager;
    class CompositeMapRenderer;

class HeightCullCallback : public osg::NodeCallback
{
public:
    HeightCullCallback() : mLowZ(-FLT_MAX), mHighZ(FLT_MAX), mMask(~0) {}

    void setLowZ(float z)
    {
        mLowZ = z;
    }
    float getLowZ() const { return mLowZ; }

    void setHighZ(float highZ)
    {
        mHighZ = highZ;
    }
    float getHighZ() const { return mHighZ; }

    void setCullMask(unsigned int mask) { mMask = mask; }
    unsigned int getCullMask() const { return mMask; }

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        if (mLowZ <= mHighZ)
            traverse(node, nv);
    }
private:
    float mLowZ;
    float mHighZ;
    unsigned int mMask;
};

    /**
     * @brief A View is a collection of rendering objects that are visible from a given camera/intersection.
     * The base View class is part of the interface for usage in conjunction with preload feature.
     */
    class View : public osg::Referenced
    {
    public:
        virtual ~View() {}

        /// Reset internal structure so that the next addition to the view will override the previous frame's contents.
        virtual void reset() = 0;
    };

    /**
     * @brief The basic interface for a terrain world. How the terrain chunks are paged and displayed
     *  is up to the implementation.
     */
    class World
    {
    public:
        /// @note takes ownership of \a storage
        /// @param storage Storage instance to get terrain data from (heights, normals, colors, textures..)
        /// @param nodeMask mask for the terrain root
        /// @param preCompileMask mask for pre compiling textures
        World(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem, Storage* storage, int nodeMask, int preCompileMask, int borderMask);
        virtual ~World();

        /// Set a WorkQueue to delete objects in the background thread.
        void setWorkQueue(SceneUtil::WorkQueue* workQueue);

        /// See CompositeMapRenderer::setTargetFrameRate
        void setTargetFrameRate(float rate);

        /// Apply the scene manager's texture filtering settings to all cached textures.
        /// @note Thread safe.
        void updateTextureFiltering();

        float getHeightAt (const osg::Vec3f& worldPos);

        /// Clears the cached land and landtexture data.
        /// @note Thread safe.
        virtual void clearAssociatedCaches();

        /// Load a terrain cell and store it in the View for later use.
        /// @note Thread safe.
        virtual void cacheCell(View* view, int x, int y) {}

        /// Load the cell into the scene graph.
        /// @note Not thread safe.
        virtual void loadCell(int x, int y);

        /// Remove the cell from the scene graph.
        /// @note Not thread safe.
        virtual void unloadCell(int x, int y);

        virtual void enable(bool enabled) {}

        virtual void setBordersVisible(bool visible);

        /// Create a View to use with preload feature. The caller is responsible for deleting the view.
        /// @note Thread safe.
        virtual View* createView() { return nullptr; }

        /// @note Thread safe, as long as you do not attempt to load into the same view from multiple threads.

        virtual void preload(View* view, const osg::Vec3f& viewPoint, std::atomic<bool>& abort) {}

        /// Store a preloaded view into the cache with the intent that the next rendering traversal can use it.
        /// @note Not thread safe.
        virtual void storeView(const View* view, double referenceTime) {}

        virtual void reportStats(unsigned int frameNumber, osg::Stats* stats) {}

        virtual void setViewDistance(float distance) {}

        Storage* getStorage() { return mStorage; }

        osg::Callback* getHeightCullCallback(float highz, unsigned int mask);

    protected:
        Storage* mStorage;

        osg::ref_ptr<osg::Group> mParent;
        osg::ref_ptr<osg::Group> mTerrainRoot;

        osg::ref_ptr<osg::Group> mCompositeMapCamera;
        osg::ref_ptr<CompositeMapRenderer> mCompositeMapRenderer;

        Resource::ResourceSystem* mResourceSystem;

        std::unique_ptr<TextureManager> mTextureManager;
        std::unique_ptr<ChunkManager> mChunkManager;

        std::unique_ptr<CellBorder> mCellBorder;

        bool mBorderVisible;

        std::set<std::pair<int,int>> mLoadedCells;
        osg::ref_ptr<HeightCullCallback> mHeightCullCallback;
    };
}

#endif
