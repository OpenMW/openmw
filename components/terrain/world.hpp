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

namespace osgUtil
{
    class IncrementalCompileOperation;
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
        World(osg::Group* parent, osg::Group* compileRoot, Resource::ResourceSystem* resourceSystem, osgUtil::IncrementalCompileOperation* ico,
              Storage* storage, int nodeMask, int preCompileMask);
        virtual ~World();

        virtual void updateTextureFiltering() {}

        float getHeightAt (const osg::Vec3f& worldPos);

        virtual osg::ref_ptr<osg::Node> cacheCell(int x, int y) {return NULL;}

        // This is only a hint and may be ignored by the implementation.
        virtual void loadCell(int x, int y) {}
        virtual void unloadCell(int x, int y) {}

        Storage* getStorage() { return mStorage; }

    protected:
        Storage* mStorage;

        osg::ref_ptr<osg::Group> mParent;
        osg::ref_ptr<osg::Group> mTerrainRoot;
        osg::ref_ptr<osg::Node> mCompositeMapRenderer;

        Resource::ResourceSystem* mResourceSystem;

        osg::ref_ptr<osgUtil::IncrementalCompileOperation> mIncrementalCompileOperation;

        std::auto_ptr<TextureManager> mTextureManager;
        std::auto_ptr<ChunkManager> mChunkManager;
    };

}

#endif
