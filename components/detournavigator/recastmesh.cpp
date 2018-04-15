#include "recastmesh.hpp"
#include "settings.hpp"

#include <Recast.h>

namespace DetourNavigator
{
    RecastMesh::RecastMesh(std::vector<int> indices, std::vector<float> vertices, const Settings& settings)
        : mIndices(std::move(indices))
        , mVertices(std::move(vertices))
        , mChunkyTriMesh(mVertices, mIndices, settings.mTrianglesPerChunk)
    {
        if (getVerticesCount())
            rcCalcBounds(mVertices.data(), static_cast<int>(getVerticesCount()), mBoundsMin.ptr(), mBoundsMax.ptr());
    }
}
