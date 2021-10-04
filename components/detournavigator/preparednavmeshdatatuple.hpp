#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_PREPAREDNAVMESHDATATUPLE_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_PREPAREDNAVMESHDATATUPLE_H

#include "preparednavmeshdata.hpp"
#include "ref.hpp"
#include "recast.hpp"

#include <Recast.h>

#include <tuple>

namespace DetourNavigator
{
    constexpr auto makeTuple(const rcPolyMesh& v) noexcept
    {
        return std::tuple(
            Span(v.verts, getVertsLength(v)),
            Span(v.polys, getPolysLength(v)),
            Span(v.regs, getRegsLength(v)),
            Span(v.flags, getFlagsLength(v)),
            Span(v.areas, getAreasLength(v)),
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
            Span(v.meshes, getMeshesLength(v)),
            Span(v.verts, getVertsLength(v)),
            Span(v.tris, getTrisLength(v))
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
