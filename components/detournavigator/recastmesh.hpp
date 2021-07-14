#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESH_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESH_H

#include "areatype.hpp"
#include "bounds.hpp"

#include <components/bullethelpers/operators.hpp>

#include <osg/Vec3f>

#include <memory>
#include <string>
#include <vector>
#include <tuple>

namespace DetourNavigator
{
    class Mesh
    {
    public:
        Mesh(std::vector<int>&& indices, std::vector<float>&& vertices, std::vector<AreaType>&& areaTypes);

        const std::vector<int>& getIndices() const noexcept { return mIndices; }
        const std::vector<float>& getVertices() const noexcept { return mVertices; }
        const std::vector<AreaType>& getAreaTypes() const noexcept { return mAreaTypes; }
        std::size_t getVerticesCount() const noexcept { return mVertices.size() / 3; }
        std::size_t getTrianglesCount() const noexcept { return mAreaTypes.size(); }

    private:
        std::vector<int> mIndices;
        std::vector<float> mVertices;
        std::vector<AreaType> mAreaTypes;

        friend inline bool operator<(const Mesh& lhs, const Mesh& rhs) noexcept
        {
            return std::tie(lhs.mIndices, lhs.mVertices, lhs.mAreaTypes)
                    < std::tie(rhs.mIndices, rhs.mVertices, rhs.mAreaTypes);
        }

        friend inline std::size_t getSize(const Mesh& value) noexcept
        {
            return value.mIndices.size() * sizeof(int)
                + value.mVertices.size() * sizeof(float)
                + value.mAreaTypes.size() * sizeof(AreaType);
        }
    };

    struct Cell
    {
        int mSize;
        osg::Vec3f mShift;
    };

    class RecastMesh
    {
    public:
        RecastMesh(std::size_t generation, std::size_t revision, Mesh mesh, std::vector<Cell> water);

        std::size_t getGeneration() const
        {
            return mGeneration;
        }

        std::size_t getRevision() const
        {
            return mRevision;
        }

        const Mesh& getMesh() const noexcept { return mMesh; }

        const std::vector<Cell>& getWater() const
        {
            return mWater;
        }

        const Bounds& getBounds() const
        {
            return mBounds;
        }

    private:
        std::size_t mGeneration;
        std::size_t mRevision;
        Mesh mMesh;
        std::vector<Cell> mWater;
        Bounds mBounds;

        friend inline bool operator <(const RecastMesh& lhs, const RecastMesh& rhs) noexcept
        {
            return std::tie(lhs.mMesh, lhs.mWater) < std::tie(rhs.mMesh, rhs.mWater);
        }

        friend inline std::size_t getSize(const RecastMesh& value) noexcept
        {
            return getSize(value.mMesh) + value.mWater.size() * sizeof(Cell);
        }
    };

    inline bool operator<(const Cell& lhs, const Cell& rhs) noexcept
    {
        return std::tie(lhs.mSize, lhs.mShift) < std::tie(rhs.mSize, rhs.mShift);
    }
}

#endif
