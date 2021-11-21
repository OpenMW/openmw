#include "preparednavmeshdata.hpp"
#include "preparednavmeshdatatuple.hpp"

#include <Recast.h>
#include <RecastAlloc.h>

namespace
{
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
