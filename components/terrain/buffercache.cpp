#include "buffercache.hpp"

#include <cassert>

#include <osg/PrimitiveSet>

#include "defs.hpp"

namespace
{

template <typename IndexArrayType>
osg::ref_ptr<IndexArrayType> createIndexBuffer(unsigned int flags, unsigned int verts)
{
    // LOD level n means every 2^n-th vertex is kept
    size_t lodLevel = (flags >> (4*4));

    size_t lodDeltas[4];
    for (int i=0; i<4; ++i)
        lodDeltas[i] = (flags >> (4*i)) & (0xf);

    bool anyDeltas = (lodDeltas[Terrain::North] || lodDeltas[Terrain::South] || lodDeltas[Terrain::West] || lodDeltas[Terrain::East]);

    size_t increment = static_cast<size_t>(1) << lodLevel;
    assert(increment < verts);

    osg::ref_ptr<IndexArrayType> indices (new IndexArrayType(osg::PrimitiveSet::TRIANGLES));
    indices->reserve((verts-1)*(verts-1)*2*3 / increment);

    size_t rowStart = 0, colStart = 0, rowEnd = verts-1, colEnd = verts-1;
    // If any edge needs stitching we'll skip all edges at this point,
    // mainly because stitching one edge would have an effect on corners and on the adjacent edges
    if (anyDeltas)
    {
        colStart += increment;
        colEnd -= increment;
        rowEnd -= increment;
        rowStart += increment;
    }
    for (size_t row = rowStart; row < rowEnd; row += increment)
    {
        for (size_t col = colStart; col < colEnd; col += increment)
        {
            // diamond pattern
            if ((row + col%2) % 2 == 1)
            {
                indices->push_back(verts*(col+increment)+row);
                indices->push_back(verts*(col+increment)+row+increment);
                indices->push_back(verts*col+row+increment);

                indices->push_back(verts*col+row);
                indices->push_back(verts*(col+increment)+row);
                indices->push_back(verts*(col)+row+increment);
            }
            else
            {
                indices->push_back(verts*col+row);
                indices->push_back(verts*(col+increment)+row+increment);
                indices->push_back(verts*col+row+increment);

                indices->push_back(verts*col+row);
                indices->push_back(verts*(col+increment)+row);
                indices->push_back(verts*(col+increment)+row+increment);
            }
        }
    }

    size_t innerStep = increment;
    if (anyDeltas)
    {
        // Now configure LOD transitions at the edges - this is pretty tedious,
        // and some very long and boring code, but it works great

        // South
        size_t row = 0;
        size_t outerStep = static_cast<size_t>(1) << (lodDeltas[Terrain::South] + lodLevel);
        for (size_t col = 0; col < verts-1; col += outerStep)
        {
            indices->push_back(verts*col+row);
            indices->push_back(verts*(col+outerStep)+row);
            // Make sure not to touch the right edge
            if (col+outerStep == verts-1)
                indices->push_back(verts*(col+outerStep-innerStep)+row+innerStep);
            else
                indices->push_back(verts*(col+outerStep)+row+innerStep);

            for (size_t i = 0; i < outerStep; i += innerStep)
            {
                // Make sure not to touch the left or right edges
                if (col+i == 0 || col+i == verts-1-innerStep)
                    continue;
                indices->push_back(verts*(col)+row);
                indices->push_back(verts*(col+i+innerStep)+row+innerStep);
                indices->push_back(verts*(col+i)+row+innerStep);
            }
        }

        // North
        row = verts-1;
        outerStep = size_t(1) << (lodDeltas[Terrain::North] + lodLevel);
        for (size_t col = 0; col < verts-1; col += outerStep)
        {
            indices->push_back(verts*(col+outerStep)+row);
            indices->push_back(verts*col+row);
            // Make sure not to touch the left edge
            if (col == 0)
                indices->push_back(verts*(col+innerStep)+row-innerStep);
            else
                indices->push_back(verts*col+row-innerStep);

            for (size_t i = 0; i < outerStep; i += innerStep)
            {
                // Make sure not to touch the left or right edges
                if (col+i == 0 || col+i == verts-1-innerStep)
                    continue;
                indices->push_back(verts*(col+i)+row-innerStep);
                indices->push_back(verts*(col+i+innerStep)+row-innerStep);
                indices->push_back(verts*(col+outerStep)+row);
            }
        }

        // West
        size_t col = 0;
        outerStep = size_t(1) << (lodDeltas[Terrain::West] + lodLevel);
        for (row = 0; row < verts-1; row += outerStep)
        {
            indices->push_back(verts*col+row+outerStep);
            indices->push_back(verts*col+row);
            // Make sure not to touch the top edge
            if (row+outerStep == verts-1)
                indices->push_back(verts*(col+innerStep)+row+outerStep-innerStep);
            else
                indices->push_back(verts*(col+innerStep)+row+outerStep);

            for (size_t i = 0; i < outerStep; i += innerStep)
            {
                // Make sure not to touch the top or bottom edges
                if (row+i == 0 || row+i == verts-1-innerStep)
                    continue;
                indices->push_back(verts*col+row);
                indices->push_back(verts*(col+innerStep)+row+i);
                indices->push_back(verts*(col+innerStep)+row+i+innerStep);
            }
        }

        // East
        col = verts-1;
        outerStep = size_t(1) << (lodDeltas[Terrain::East] + lodLevel);
        for (row = 0; row < verts-1; row += outerStep)
        {
            indices->push_back(verts*col+row);
            indices->push_back(verts*col+row+outerStep);
            // Make sure not to touch the bottom edge
            if (row == 0)
                indices->push_back(verts*(col-innerStep)+row+innerStep);
            else
                indices->push_back(verts*(col-innerStep)+row);

            for (size_t i = 0; i < outerStep; i += innerStep)
            {
                // Make sure not to touch the top or bottom edges
                if (row+i == 0 || row+i == verts-1-innerStep)
                    continue;
                indices->push_back(verts*col+row+outerStep);
                indices->push_back(verts*(col-innerStep)+row+i+innerStep);
                indices->push_back(verts*(col-innerStep)+row+i);
            }
        }
    }

    return indices;
}

}

namespace Terrain
{

    osg::ref_ptr<osg::Vec2Array> BufferCache::getUVBuffer(unsigned int numVerts)
    {
        std::lock_guard<std::mutex> lock(mUvBufferMutex);
        if (mUvBufferMap.find(numVerts) != mUvBufferMap.end())
        {
            return mUvBufferMap[numVerts];
        }

        int vertexCount = numVerts * numVerts;

        osg::ref_ptr<osg::Vec2Array> uvs (new osg::Vec2Array(osg::Array::BIND_PER_VERTEX));
        uvs->reserve(vertexCount);

        for (unsigned int col = 0; col < numVerts; ++col)
        {
            for (unsigned int row = 0; row < numVerts; ++row)
            {
                uvs->push_back(osg::Vec2f(col / static_cast<float>(numVerts-1),
                                          ((numVerts-1) - row) / static_cast<float>(numVerts-1)));
            }
        }

        // Assign a VBO here to enable state sharing between different Geometries.
        uvs->setVertexBufferObject(new osg::VertexBufferObject);

        mUvBufferMap[numVerts] = uvs;
        return uvs;
    }

    osg::ref_ptr<osg::DrawElements> BufferCache::getIndexBuffer(unsigned int numVerts, unsigned int flags)
    {
        std::pair<int, int> id = std::make_pair(numVerts, flags);
        std::lock_guard<std::mutex> lock(mIndexBufferMutex);

        if (mIndexBufferMap.find(id) != mIndexBufferMap.end())
        {
            return mIndexBufferMap[id];
        }

        osg::ref_ptr<osg::DrawElements> buffer;

        if (numVerts*numVerts <= (0xffffu))
            buffer = createIndexBuffer<osg::DrawElementsUShort>(flags, numVerts);
        else
            buffer = createIndexBuffer<osg::DrawElementsUInt>(flags, numVerts);

        // Assign a EBO here to enable state sharing between different Geometries.
        buffer->setElementBufferObject(new osg::ElementBufferObject);

        mIndexBufferMap[id] = buffer;
        return buffer;
    }

    void BufferCache::clearCache()
    {
        {
            std::lock_guard<std::mutex> lock(mIndexBufferMutex);
            mIndexBufferMap.clear();
        }
        {
            std::lock_guard<std::mutex> lock(mUvBufferMutex);
            mUvBufferMap.clear();
        }
    }

    void BufferCache::releaseGLObjects(osg::State *state)
    {
        {
            std::lock_guard<std::mutex> lock(mIndexBufferMutex);
            for (auto indexbuffer : mIndexBufferMap)
                indexbuffer.second->releaseGLObjects(state);
        }
        {
            std::lock_guard<std::mutex> lock(mUvBufferMutex);
            for (auto uvbuffer : mUvBufferMap)
                uvbuffer.second->releaseGLObjects(state);
        }
    }

}
