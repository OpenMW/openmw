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
#ifndef COMPONENTS_TERRAIN_BUFFERCACHE_H
#define COMPONENTS_TERRAIN_BUFFERCACHE_H

#include <OgreHardwareIndexBuffer.h>
#include <OgreHardwareVertexBuffer.h>

#include <map>

namespace Terrain
{

    /// @brief Implements creation and caching of vertex buffers for terrain chunks.
    class BufferCache
    {
    public:
        BufferCache(unsigned int numVerts) : mNumVerts(numVerts) {}

        /// @param flags first 4*4 bits are LOD deltas on each edge, respectively (4 bits each)
        ///              next 4 bits are LOD level of the index buffer (LOD 0 = don't omit any vertices)
        Ogre::HardwareIndexBufferSharedPtr getIndexBuffer (unsigned int flags);

        Ogre::HardwareVertexBufferSharedPtr getUVBuffer ();

    private:
        // Index buffers are shared across terrain batches where possible. There is one index buffer for each
        // combination of LOD deltas and index buffer LOD we may need.
        std::map<int, Ogre::HardwareIndexBufferSharedPtr> mIndexBufferMap;

        std::map<int, Ogre::HardwareVertexBufferSharedPtr> mUvBufferMap;

        unsigned int mNumVerts;
    };

}

#endif
