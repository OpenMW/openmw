#include "recastmesh.hpp"
#include "exceptions.hpp"

#include <Recast.h>

namespace DetourNavigator
{
    Mesh::Mesh(std::vector<int>&& indices, std::vector<float>&& vertices, std::vector<AreaType>&& areaTypes)
    {
        if (indices.size() / 3 != areaTypes.size())
           throw InvalidArgument("Number of flags doesn't match number of triangles: triangles="
               + std::to_string(indices.size() / 3) + ", areaTypes=" + std::to_string(areaTypes.size()));
        indices.shrink_to_fit();
        vertices.shrink_to_fit();
        areaTypes.shrink_to_fit();
        mIndices = std::move(indices);
        mVertices = std::move(vertices);
        mAreaTypes = std::move(areaTypes);
    }

    RecastMesh::RecastMesh(std::size_t generation, std::size_t revision, Mesh mesh, std::vector<Cell> water,
        std::vector<Heightfield> heightfields, std::vector<FlatHeightfield> flatHeightfields)
        : mGeneration(generation)
        , mRevision(revision)
        , mMesh(std::move(mesh))
        , mWater(std::move(water))
        , mHeightfields(std::move(heightfields))
        , mFlatHeightfields(std::move(flatHeightfields))
    {
        if (mMesh.getVerticesCount() > 0)
            rcCalcBounds(mMesh.getVertices().data(), static_cast<int>(mMesh.getVerticesCount()),
                         mBounds.mMin.ptr(), mBounds.mMax.ptr());
        mWater.shrink_to_fit();
        mHeightfields.shrink_to_fit();
        for (Heightfield& v : mHeightfields)
            v.mHeights.shrink_to_fit();
        for (const Heightfield& v : mHeightfields)
        {
            const auto [min, max] = std::minmax_element(v.mHeights.begin(), v.mHeights.end());
            mBounds.mMin.x() = std::min(mBounds.mMin.x(), v.mBounds.mMin.x());
            mBounds.mMin.y() = std::min(mBounds.mMin.y(), v.mBounds.mMin.y());
            mBounds.mMin.z() = std::min(mBounds.mMin.z(), *min);
            mBounds.mMax.x() = std::max(mBounds.mMax.x(), v.mBounds.mMax.x());
            mBounds.mMax.y() = std::max(mBounds.mMax.y(), v.mBounds.mMax.y());
            mBounds.mMax.z() = std::max(mBounds.mMax.z(), *max);
        }
        for (const FlatHeightfield& v : mFlatHeightfields)
        {
            mBounds.mMin.x() = std::min(mBounds.mMin.x(), v.mBounds.mMin.x());
            mBounds.mMin.y() = std::min(mBounds.mMin.y(), v.mBounds.mMin.y());
            mBounds.mMin.z() = std::min(mBounds.mMin.z(), v.mHeight);
            mBounds.mMax.x() = std::max(mBounds.mMax.x(), v.mBounds.mMax.x());
            mBounds.mMax.y() = std::max(mBounds.mMax.y(), v.mBounds.mMax.y());
            mBounds.mMax.z() = std::max(mBounds.mMax.z(), v.mHeight);
        }
    }
}
