#include "preparednavmeshdata.hpp"
#include "preparednavmeshdatatuple.hpp"

#include <Recast.h>
#include <RecastAlloc.h>

namespace
{
    using namespace DetourNavigator;

    void initPolyMeshDetail(rcPolyMeshDetail& value) noexcept
    {
        value.meshes = nullptr;
        value.verts = nullptr;
        value.tris = nullptr;
        value.nmeshes = 0;
        value.nverts = 0;
        value.ntris = 0;
    }

    void freePolyMeshDetail(rcPolyMeshDetail& value) noexcept
    {
        rcFree(value.meshes);
        rcFree(value.verts);
        rcFree(value.tris);
    }
}

template <class T>
inline constexpr auto operator==(const T& lhs, const T& rhs) noexcept
    -> std::enable_if_t<std::is_same_v<std::void_t<decltype(makeTuple(lhs))>, void>, bool>
{
    return makeTuple(lhs) == makeTuple(rhs);
}

namespace DetourNavigator
{
    PreparedNavMeshData::PreparedNavMeshData() noexcept
    {
        initPolyMeshDetail(mPolyMeshDetail);
    }

    PreparedNavMeshData::~PreparedNavMeshData() noexcept
    {
        freePolyMeshDetail(mPolyMeshDetail);
    }

    bool operator==(const PreparedNavMeshData& lhs, const PreparedNavMeshData& rhs) noexcept
    {
        return makeTuple(lhs) == makeTuple(rhs);
    }
}
