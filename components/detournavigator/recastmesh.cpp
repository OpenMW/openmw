#include "recastmesh.hpp"
#include "settings.hpp"
#include "exceptions.hpp"

#include <Recast.h>

namespace DetourNavigator
{
    RecastMesh::RecastMesh(std::vector<int> indices, std::vector<float> vertices,
                           std::vector<unsigned char> flags, const Settings& settings)
        : mIndices(std::move(indices))
        , mVertices(std::move(vertices))
        , mFlags(std::move(flags))
        , mChunkyTriMesh(mVertices, mIndices, mFlags, settings.mTrianglesPerChunk)
    {
        if (getTrianglesCount() != mFlags.size())
            throw InvalidArgument("number of flags doesn't match number of triangles: triangles="
                + std::to_string(getTrianglesCount()) + ", flags=" + std::to_string(mFlags.size()));
        if (getVerticesCount())
            rcCalcBounds(mVertices.data(), static_cast<int>(getVerticesCount()), mBoundsMin.ptr(), mBoundsMax.ptr());
    }
}
