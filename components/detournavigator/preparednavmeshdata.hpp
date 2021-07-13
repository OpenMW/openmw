#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_PREPAREDNAVMESHDATA_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_PREPAREDNAVMESHDATA_H

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
        PreparedNavMeshData(const PreparedNavMeshData&) = delete;

        ~PreparedNavMeshData() noexcept;

        friend bool operator==(const PreparedNavMeshData& lhs, const PreparedNavMeshData& rhs) noexcept;
    };

    inline constexpr std::size_t getSize(const rcPolyMesh& value) noexcept
    {
        return static_cast<std::size_t>(3 * value.nverts) * sizeof(*value.verts)
            + static_cast<std::size_t>(value.maxpolys * 2 * value.nvp) * sizeof(*value.polys)
            + static_cast<std::size_t>(value.maxpolys) * sizeof(*value.regs)
            + static_cast<std::size_t>(value.maxpolys) * sizeof(*value.flags)
            + static_cast<std::size_t>(value.maxpolys) * sizeof(*value.areas);
    }

    inline constexpr std::size_t getSize(const rcPolyMeshDetail& value) noexcept
    {
        return static_cast<std::size_t>(4 * value.nmeshes) * sizeof(*value.meshes)
            + static_cast<std::size_t>(4 * value.ntris) * sizeof(*value.tris)
            + static_cast<std::size_t>(3 * value.nverts) * sizeof(*value.verts);
    }

    inline constexpr std::size_t getSize(const PreparedNavMeshData& value) noexcept
    {
        return getSize(value.mPolyMesh) + getSize(value.mPolyMeshDetail);
    }
}

#endif
