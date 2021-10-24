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
            Span(v.verts, static_cast<int>(getVertsLength(v))),
            Span(v.polys, static_cast<int>(getPolysLength(v))),
            Span(v.regs, static_cast<int>(getRegsLength(v))),
            Span(v.flags, static_cast<int>(getFlagsLength(v))),
            Span(v.areas, static_cast<int>(getAreasLength(v))),
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
            Span(v.meshes, static_cast<int>(getMeshesLength(v))),
            Span(v.verts, static_cast<int>(getVertsLength(v))),
            Span(v.tris, static_cast<int>(getTrisLength(v)))
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
