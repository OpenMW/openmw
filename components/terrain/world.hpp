#ifndef COMPONENTS_TERRAIN_WORLD_H
#define COMPONENTS_TERRAIN_WORLD_H

#include <osg/Referenced>
#include <osg/Vec3f>
#include <osg/ref_ptr>

#include <memory>
#include <set>

#include <components/esm/refid.hpp>

#include "cellborder.hpp"

namespace osg
{
    class Group;
    class Stats;
}

namespace Resource
{
    class ResourceSystem;
}

namespace Loading
{
    class Reporter;
}

namespace Terrain
{
    class Storage;

    class TextureManager;
    class ChunkManager;
    class CompositeMapRenderer;
    class View;
    class HeightCullCallback;

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
        explicit World(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem,
            Storage* storage, unsigned int nodeMask, unsigned int preCompileMask, unsigned int borderMask,
            ESM::RefId worldspace, double expiryDelay);
        World(osg::Group* parent, Storage* storage, unsigned int nodeMask, ESM::RefId worldspace);
        virtual ~World();

        /// See CompositeMapRenderer::setTargetFrameRate
        void setTargetFrameRate(float rate);

        /// Apply the scene manager's texture filtering settings to all cached textures.
        /// @note Thread safe.
        void updateTextureFiltering();

        float getHeightAt(const osg::Vec3f& worldPos);

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
        virtual bool getBordersVisible() { return mBorderVisible; }

        /// Create a View to use with preload feature. The caller is responsible for deleting the view.
        /// @note Thread safe.
        virtual View* createView() { return nullptr; }

        /// @note Thread safe, as long as you do not attempt to load into the same view from multiple threads.

        virtual void preload(View* view, const osg::Vec3f& viewPoint, const osg::Vec4i& cellgrid,
            std::atomic<bool>& abort, Loading::Reporter& reporter)
        {
        }

        virtual void rebuildViews() {}

        virtual void reportStats(unsigned int frameNumber, osg::Stats* stats) {}

        virtual void setViewDistance(float distance) {}

        ESM::RefId getWorldspace() { return mWorldspace; }

        Storage* getStorage() { return mStorage; }

        void enableHeightCullCallback(bool enable);
        osg::Callback* getHeightCullCallback(float highz, unsigned int mask);

        void setActiveGrid(const osg::Vec4i& grid) { mActiveGrid = grid; }

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

        std::set<std::pair<int, int>> mLoadedCells;
        osg::ref_ptr<HeightCullCallback> mHeightCullCallback;

        osg::Vec4i mActiveGrid;
        ESM::RefId mWorldspace;
    };
}

#endif
