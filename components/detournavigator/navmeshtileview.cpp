#include "navmeshtileview.hpp"
#include "ref.hpp"

#include <DetourCommon.h>
#include <DetourNavMesh.h>

#include <cassert>
#include <stdexcept>
#include <tuple>

inline bool operator==(const dtMeshHeader& lhs, const dtMeshHeader& rhs) noexcept
{
    const auto makeTuple = [](const dtMeshHeader& v) {
        using DetourNavigator::ArrayRef;
        return std::tuple(v.x, v.y, v.layer, v.userId, v.polyCount, v.vertCount, v.maxLinkCount, v.detailMeshCount,
            v.detailVertCount, v.detailTriCount, v.bvNodeCount, v.offMeshConCount, v.offMeshBase, v.walkableHeight,
            v.walkableRadius, v.walkableClimb, v.detailVertCount, ArrayRef(v.bmin), ArrayRef(v.bmax), v.bvQuantFactor);
    };
    return makeTuple(lhs) == makeTuple(rhs);
}

inline bool operator==(const dtPoly& lhs, const dtPoly& rhs) noexcept
{
    const auto makeTuple = [](const dtPoly& v) {
        using DetourNavigator::ArrayRef;
        return std::tuple(ArrayRef(v.verts), ArrayRef(v.neis), v.flags, v.vertCount, v.areaAndtype);
    };
    return makeTuple(lhs) == makeTuple(rhs);
}

inline bool operator==(const dtPolyDetail& lhs, const dtPolyDetail& rhs) noexcept
{
    const auto makeTuple
        = [](const dtPolyDetail& v) { return std::tuple(v.vertBase, v.triBase, v.vertCount, v.triCount); };
    return makeTuple(lhs) == makeTuple(rhs);
}

inline bool operator==(const dtBVNode& lhs, const dtBVNode& rhs) noexcept
{
    const auto makeTuple = [](const dtBVNode& v) {
        using DetourNavigator::ArrayRef;
        return std::tuple(ArrayRef(v.bmin), ArrayRef(v.bmax), v.i);
    };
    return makeTuple(lhs) == makeTuple(rhs);
}

inline bool operator==(const dtOffMeshConnection& lhs, const dtOffMeshConnection& rhs) noexcept
{
    const auto makeTuple = [](const dtOffMeshConnection& v) {
        using DetourNavigator::ArrayRef;
        return std::tuple(ArrayRef(v.pos), v.rad, v.poly, v.flags, v.side, v.userId);
    };
    return makeTuple(lhs) == makeTuple(rhs);
}

namespace DetourNavigator
{
    NavMeshTileConstView asNavMeshTileConstView(const unsigned char* data)
    {
        const dtMeshHeader* header = reinterpret_cast<const dtMeshHeader*>(data);

        if (header->magic != DT_NAVMESH_MAGIC)
            throw std::logic_error("Invalid navmesh magic");

        if (header->version != DT_NAVMESH_VERSION)
            throw std::logic_error("Invalid navmesh version");

        // Similar code to
        // https://github.com/recastnavigation/recastnavigation/blob/c5cbd53024c8a9d8d097a4371215e3342d2fdc87/Detour/Source/DetourNavMesh.cpp#L978-L996
        const int headerSize = dtAlign4(sizeof(dtMeshHeader));
        const int vertsSize = dtAlign4(sizeof(float) * 3 * header->vertCount);
        const int polysSize = dtAlign4(sizeof(dtPoly) * header->polyCount);
        const int linksSize = dtAlign4(sizeof(dtLink) * (header->maxLinkCount));
        const int detailMeshesSize = dtAlign4(sizeof(dtPolyDetail) * header->detailMeshCount);
        const int detailVertsSize = dtAlign4(sizeof(float) * 3 * header->detailVertCount);
        const int detailTrisSize = dtAlign4(sizeof(unsigned char) * 4 * header->detailTriCount);
        const int bvtreeSize = dtAlign4(sizeof(dtBVNode) * header->bvNodeCount);
        const int offMeshLinksSize = dtAlign4(sizeof(dtOffMeshConnection) * header->offMeshConCount);

        const unsigned char* ptr = data + headerSize;

        NavMeshTileConstView view;

        view.mHeader = header;
        view.mVerts = dtGetThenAdvanceBufferPointer<const float>(ptr, vertsSize);
        view.mPolys = dtGetThenAdvanceBufferPointer<const dtPoly>(ptr, polysSize);
        ptr += linksSize;
        view.mDetailMeshes = dtGetThenAdvanceBufferPointer<const dtPolyDetail>(ptr, detailMeshesSize);
        view.mDetailVerts = dtGetThenAdvanceBufferPointer<const float>(ptr, detailVertsSize);
        view.mDetailTris = dtGetThenAdvanceBufferPointer<const unsigned char>(ptr, detailTrisSize);
        view.mBvTree = dtGetThenAdvanceBufferPointer<const dtBVNode>(ptr, bvtreeSize);
        view.mOffMeshCons = dtGetThenAdvanceBufferPointer<const dtOffMeshConnection>(ptr, offMeshLinksSize);

        return view;
    }

    NavMeshTileConstView asNavMeshTileConstView(const dtMeshTile& tile)
    {
        NavMeshTileConstView view;

        view.mHeader = tile.header;
        view.mPolys = tile.polys;
        view.mVerts = tile.verts;
        view.mDetailMeshes = tile.detailMeshes;
        view.mDetailVerts = tile.detailVerts;
        view.mDetailTris = tile.detailTris;
        view.mBvTree = tile.bvTree;
        view.mOffMeshCons = tile.offMeshCons;

        return view;
    }

    bool operator==(const NavMeshTileConstView& lhs, const NavMeshTileConstView& rhs) noexcept
    {
        using DetourNavigator::Ref;
        using DetourNavigator::Span;
        const auto makeTuple = [](const DetourNavigator::NavMeshTileConstView& v) {
            return std::tuple(Ref(*v.mHeader), Span(v.mPolys, v.mHeader->polyCount),
                Span(v.mVerts, v.mHeader->vertCount), Span(v.mDetailMeshes, v.mHeader->detailMeshCount),
                Span(v.mDetailVerts, v.mHeader->detailVertCount), Span(v.mDetailTris, v.mHeader->detailTriCount),
                Span(v.mBvTree, v.mHeader->bvNodeCount), Span(v.mOffMeshCons, v.mHeader->offMeshConCount));
        };
        return makeTuple(lhs) == makeTuple(rhs);
    }
}
