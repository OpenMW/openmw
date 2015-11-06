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

    size_t increment = 1 << lodLevel;
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
        size_t outerStep = 1 << (lodDeltas[Terrain::South] + lodLevel);
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
        for (size_t row = 0; row < verts-1; row += outerStep)
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
        for (size_t row = 0; row < verts-1; row += outerStep)
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

    osg::ref_ptr<osg::Vec2Array> BufferCache::getUVBuffer()
    {
        if (mUvBufferMap.find(mNumVerts) != mUvBufferMap.end())
        {
            return mUvBufferMap[mNumVerts];
        }

        int vertexCount = mNumVerts * mNumVerts;

        osg::ref_ptr<osg::Vec2Array> uvs (new osg::Vec2Array);
        uvs->reserve(vertexCount);

        for (unsigned int col = 0; col < mNumVerts; ++col)
        {
            for (unsigned int row = 0; row < mNumVerts; ++row)
            {
                uvs->push_back(osg::Vec2f(col / static_cast<float>(mNumVerts-1),
                                          row / static_cast<float>(mNumVerts-1)));
            }
        }

        // Assign a VBO here to enable state sharing between different Geometries.
        uvs->setVertexBufferObject(new osg::VertexBufferObject);

        mUvBufferMap[mNumVerts] = uvs;
        return uvs;
    }

    osg::ref_ptr<osg::DrawElements> BufferCache::getIndexBuffer(unsigned int flags)
    {
        unsigned int verts = mNumVerts;

        if (mIndexBufferMap.find(flags) != mIndexBufferMap.end())
        {
            return mIndexBufferMap[flags];
        }

        osg::ref_ptr<osg::DrawElements> buffer;

        if (verts*verts > (0xffffu))
            buffer = createIndexBuffer<osg::DrawElementsUShort>(flags, verts);
        else
            buffer = createIndexBuffer<osg::DrawElementsUInt>(flags, verts);

        // Assign a EBO here to enable state sharing between different Geometries.
        buffer->setElementBufferObject(new osg::ElementBufferObject);

        mIndexBufferMap[flags] = buffer;
        return buffer;
    }

}
