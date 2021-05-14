#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESH_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESH_H

#include "areatype.hpp"
#include "bounds.hpp"

#include <components/bullethelpers/operators.hpp>

#include <memory>
#include <string>
#include <vector>
#include <tuple>

#include <LinearMath/btTransform.h>

namespace DetourNavigator
{
    class RecastMesh
    {
    public:
        struct Water
        {
            int mCellSize;
            btTransform mTransform;
        };

        RecastMesh(std::size_t generation, std::size_t revision, std::vector<int> indices, std::vector<float> vertices,
            std::vector<AreaType> areaTypes, std::vector<Water> water);

        std::size_t getGeneration() const
        {
            return mGeneration;
        }

        std::size_t getRevision() const
        {
            return mRevision;
        }

        const std::vector<int>& getIndices() const
        {
            return mIndices;
        }

        const std::vector<float>& getVertices() const
        {
            return mVertices;
        }

        const std::vector<AreaType>& getAreaTypes() const
        {
            return mAreaTypes;
        }

        const std::vector<Water>& getWater() const
        {
            return mWater;
        }

        std::size_t getVerticesCount() const
        {
            return mVertices.size() / 3;
        }

        std::size_t getTrianglesCount() const
        {
            return mIndices.size() / 3;
        }

        const Bounds& getBounds() const
        {
            return mBounds;
        }

    private:
        std::size_t mGeneration;
        std::size_t mRevision;
        std::vector<int> mIndices;
        std::vector<float> mVertices;
        std::vector<AreaType> mAreaTypes;
        std::vector<Water> mWater;
        Bounds mBounds;
    };

    inline bool operator<(const RecastMesh::Water& lhs, const RecastMesh::Water& rhs)
    {
        return std::tie(lhs.mCellSize, lhs.mTransform) < std::tie(rhs.mCellSize, rhs.mTransform);
    }

    inline bool operator <(const RecastMesh& lhs, const RecastMesh& rhs)
    {
        return std::tie(lhs.getIndices(), lhs.getVertices(), lhs.getAreaTypes(), lhs.getWater())
                < std::tie(rhs.getIndices(), rhs.getVertices(), rhs.getAreaTypes(), rhs.getWater());
    }
}

#endif
