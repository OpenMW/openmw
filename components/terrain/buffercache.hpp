#ifndef COMPONENTS_TERRAIN_BUFFERCACHE_H
#define COMPONENTS_TERRAIN_BUFFERCACHE_H

#include <osg/ref_ptr>
#include <osg/Array>
#include <osg/PrimitiveSet>

#include <map>
#include <mutex>

namespace Terrain
{

    /// @brief Implements creation and caching of vertex buffers for terrain chunks.
    class BufferCache
    {
    public:
        /// @param flags first 4*4 bits are LOD deltas on each edge, respectively (4 bits each)
        ///              next 4 bits are LOD level of the index buffer (LOD 0 = don't omit any vertices)
        /// @note Thread safe.
        osg::ref_ptr<osg::DrawElements> getIndexBuffer (unsigned int numVerts, unsigned int flags);

        /// @note Thread safe.
        osg::ref_ptr<osg::Vec2Array> getUVBuffer(unsigned int numVerts);

        void clearCache();

        void releaseGLObjects(osg::State* state);

    private:
        // Index buffers are shared across terrain batches where possible. There is one index buffer for each
        // combination of LOD deltas and index buffer LOD we may need.
        std::map<std::pair<int, int>, osg::ref_ptr<osg::DrawElements> > mIndexBufferMap;
        std::mutex mIndexBufferMutex;

        std::map<int, osg::ref_ptr<osg::Vec2Array> > mUvBufferMap;
        std::mutex mUvBufferMutex;
    };

}

#endif
