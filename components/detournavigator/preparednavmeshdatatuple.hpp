#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_PREPAREDNAVMESHDATATUPLE_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_PREPAREDNAVMESHDATATUPLE_H

#include "preparednavmeshdata.hpp"
#include "ref.hpp"
#include "recast.hpp"

#include <Recast.h>

#include <tuple>

inline bool operator==(const rcPolyMesh& lhs, const rcPolyMesh& rhs) noexcept
{
    const auto makeTuple = [] (const rcPolyMesh& v)
    {
        using namespace DetourNavigator;
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
    };
    return makeTuple(lhs) == makeTuple(rhs);
}

inline bool operator==(const rcPolyMeshDetail& lhs, const rcPolyMeshDetail& rhs) noexcept
{
    const auto makeTuple = [] (const rcPolyMeshDetail& v)
    {
        using namespace DetourNavigator;
        return std::tuple(
            Span(v.meshes, static_cast<int>(getMeshesLength(v))),
            Span(v.verts, static_cast<int>(getVertsLength(v))),
            Span(v.tris, static_cast<int>(getTrisLength(v)))
        );
    };
    return makeTuple(lhs) == makeTuple(rhs);
}

namespace DetourNavigator
{
    inline auto makeTuple(const PreparedNavMeshData& v) noexcept
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
