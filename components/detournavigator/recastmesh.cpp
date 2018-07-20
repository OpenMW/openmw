#include "recastmesh.hpp"
#include "settings.hpp"
#include "exceptions.hpp"

#include <Recast.h>

namespace DetourNavigator
{
    RecastMesh::RecastMesh(std::vector<int> indices, std::vector<float> vertices,
                           std::vector<AreaType> areaTypes, std::vector<Water> water, const Settings& settings)
        : mIndices(std::move(indices))
        , mVertices(std::move(vertices))
        , mAreaTypes(std::move(areaTypes))
        , mWater(std::move(water))
        , mChunkyTriMesh(mVertices, mIndices, mAreaTypes, settings.mTrianglesPerChunk)
    {
        if (getTrianglesCount() != mAreaTypes.size())
            throw InvalidArgument("number of flags doesn't match number of triangles: triangles="
                + std::to_string(getTrianglesCount()) + ", areaTypes=" + std::to_string(mAreaTypes.size()));
        if (getVerticesCount())
            rcCalcBounds(mVertices.data(), static_cast<int>(getVerticesCount()), mBoundsMin.ptr(), mBoundsMax.ptr());
    }
}
