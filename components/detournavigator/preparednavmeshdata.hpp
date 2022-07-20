#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_PREPAREDNAVMESHDATA_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_PREPAREDNAVMESHDATA_H

#include "recast.hpp"

#include <Recast.h>

#include <cstddef>

namespace DetourNavigator
{
    struct PreparedNavMeshData
    {
        unsigned int mUserId = 0;
        float mCellSize = 0;
        float mCellHeight = 0;
        rcPolyMesh mPolyMesh;
        rcPolyMeshDetail mPolyMeshDetail;

        PreparedNavMeshData() noexcept;
        PreparedNavMeshData(const PreparedNavMeshData& other);

        ~PreparedNavMeshData() noexcept;

        friend bool operator==(const PreparedNavMeshData& lhs, const PreparedNavMeshData& rhs) noexcept;
    };

    inline constexpr std::size_t getSize(const rcPolyMesh& value) noexcept
    {
        return getVertsLength(value) * sizeof(*value.verts)
            + getPolysLength(value) * sizeof(*value.polys)
            + getRegsLength(value) * sizeof(*value.regs)
            + getFlagsLength(value) * sizeof(*value.flags)
            + getAreasLength(value) * sizeof(*value.areas);
    }

    inline constexpr std::size_t getSize(const rcPolyMeshDetail& value) noexcept
    {
        return getMeshesLength(value) * sizeof(*value.meshes)
            + getVertsLength(value) * sizeof(*value.verts)
            + getTrisLength(value) * sizeof(*value.tris);
    }

    inline constexpr std::size_t getSize(const PreparedNavMeshData& value) noexcept
    {
        return getSize(value.mPolyMesh) + getSize(value.mPolyMeshDetail);
    }
}

#endif
