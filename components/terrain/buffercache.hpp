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
        Ogre::HardwareIndexBufferSharedPtr getIndexBuffer (int flags);

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
