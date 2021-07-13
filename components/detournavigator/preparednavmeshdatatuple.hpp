#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_PREPAREDNAVMESHDATATUPLE_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_PREPAREDNAVMESHDATATUPLE_H

#include "preparednavmeshdata.hpp"
#include "ref.hpp"

#include <Recast.h>

#include <tuple>

namespace DetourNavigator
{
    constexpr auto makeTuple(const rcPolyMesh& v) noexcept
    {
        return std::tuple(
            Span(v.verts, 3 * v.nverts),
            Span(v.polys, v.maxpolys * 2 * v.nvp),
            Span(v.regs, v.maxpolys),
            Span(v.flags, v.maxpolys),
            Span(v.areas, v.maxpolys),
            ArrayRef(v.bmin),
            ArrayRef(v.bmax),
            v.cs,
            v.ch,
            v.borderSize,
            v.maxEdgeError
        );
    }

    constexpr auto makeTuple(const rcPolyMeshDetail& v) noexcept
    {
        return std::tuple(
            Span(v.meshes, 4 * v.nmeshes),
            Span(v.verts, 3 * v.nverts),
            Span(v.tris, 4 * v.ntris)
        );
    }

    constexpr auto makeTuple(const PreparedNavMeshData& v) noexcept
    {
        return std::tuple(
            v.mUserId,
            v.mCellHeight,
            v.mCellSize,
            Ref(v.mPolyMesh),
            Ref(v.mPolyMeshDetail)
        );
    }
}

#endif
