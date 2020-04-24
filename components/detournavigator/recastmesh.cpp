#include "recastmesh.hpp"
#include "exceptions.hpp"

#include <Recast.h>

namespace DetourNavigator
{
    RecastMesh::RecastMesh(std::size_t generation, std::size_t revision, std::vector<int> indices, std::vector<float> vertices,
            std::vector<AreaType> areaTypes, std::vector<Water> water, const std::size_t trianglesPerChunk)
        : mGeneration(generation)
        , mRevision(revision)
        , mIndices(std::move(indices))
        , mVertices(std::move(vertices))
        , mAreaTypes(std::move(areaTypes))
        , mWater(std::move(water))
        , mChunkyTriMesh(mVertices, mIndices, mAreaTypes, trianglesPerChunk)
    {
        if (getTrianglesCount() != mAreaTypes.size())
            throw InvalidArgument("Number of flags doesn't match number of triangles: triangles="
                + std::to_string(getTrianglesCount()) + ", areaTypes=" + std::to_string(mAreaTypes.size()));
        if (getVerticesCount())
            rcCalcBounds(mVertices.data(), static_cast<int>(getVerticesCount()), mBounds.mMin.ptr(), mBounds.mMax.ptr());
    }
}
