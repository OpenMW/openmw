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
