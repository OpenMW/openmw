#ifndef COMPONENTS_TERRAIN_WORLD_H
#define COMPONENTS_TERRAIN_WORLD_H

#include <osg/ref_ptr>

#include "defs.hpp"
#include "buffercache.hpp"

namespace osg
{
    class Group;
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
        World(osg::Group* parent, Resource::ResourceSystem* resourceSystem, osgUtil::IncrementalCompileOperation* ico,
              Storage* storage, int nodeMask);
        virtual ~World();

        float getHeightAt (const osg::Vec3f& worldPos);

        // This is only a hint and may be ignored by the implementation.
        virtual void loadCell(int x, int y) {}
        virtual void unloadCell(int x, int y) {}

        Storage* getStorage() { return mStorage; }

    protected:
        Storage* mStorage;

        BufferCache mCache;

        osg::ref_ptr<osg::Group> mParent;
        osg::ref_ptr<osg::Group> mTerrainRoot;

        Resource::ResourceSystem* mResourceSystem;

        osg::ref_ptr<osgUtil::IncrementalCompileOperation> mIncrementalCompileOperation;
    };

}

#endif
