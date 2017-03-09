#ifndef COMPONENTS_TERRAIN_WORLD_H
#define COMPONENTS_TERRAIN_WORLD_H

#include <osg/ref_ptr>
#include <osg/Vec3f>

#include <memory>

#include "defs.hpp"

namespace osg
{
    class Group;
    class Stats;
    class Node;
}

namespace Resource
{
    class ResourceSystem;
}

namespace Terrain
{
    class Storage;

    class TextureManager;
    class ChunkManager;
    class CompositeMapRenderer;

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
        World(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem, Storage* storage, int nodeMask, int preCompileMask);
        virtual ~World();

        /// Apply the scene manager's texture filtering settings to all cached textures.
        /// @note Thread safe.
        void updateTextureFiltering();

        float getHeightAt (const osg::Vec3f& worldPos);

        /// Load a terrain cell and store it in cache for later use.
        /// @note The returned ref_ptr should be kept by the caller to ensure that the terrain stays in cache for as long as needed.
        /// @note Thread safe.
        /// @note May be ignored by derived implementations that don't organize the terrain into cells.
        virtual osg::ref_ptr<osg::Node> cacheCell(int x, int y);

        /// Load the cell into the scene graph.
        /// @note Not thread safe.
        /// @note May be ignored by derived implementations that don't organize the terrain into cells.
        virtual void loadCell(int x, int y) {}

        /// Remove the cell from the scene graph.
        /// @note Not thread safe.
        /// @note May be ignored by derived implementations that don't organize the terrain into cells.
        virtual void unloadCell(int x, int y) {}

        virtual void enable(bool enabled) {}

        Storage* getStorage() { return mStorage; }

    protected:
        Storage* mStorage;

        osg::ref_ptr<osg::Group> mParent;
        osg::ref_ptr<osg::Group> mTerrainRoot;
        osg::ref_ptr<CompositeMapRenderer> mCompositeMapRenderer;

        Resource::ResourceSystem* mResourceSystem;

        std::auto_ptr<TextureManager> mTextureManager;
        std::auto_ptr<ChunkManager> mChunkManager;
    };

}

#endif
