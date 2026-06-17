#include "preparednavmeshdata.hpp"
#include "preparednavmeshdatatuple.hpp"
#include "recast.hpp"

#include <Recast.h>

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
}

namespace DetourNavigator
{
    PreparedNavMeshData::PreparedNavMeshData() noexcept
    {
        initPolyMeshDetail(mPolyMeshDetail);
    }

    PreparedNavMeshData::PreparedNavMeshData(const PreparedNavMeshData& other)
        : mUserId(other.mUserId)
        , mCellSize(other.mCellSize)
        , mCellHeight(other.mCellHeight)
    {
        copyPolyMesh(other.mPolyMesh, mPolyMesh);
        copyPolyMeshDetail(other.mPolyMeshDetail, mPolyMeshDetail);
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
