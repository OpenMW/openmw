#include "navmeshtileview.hpp"

#include <DetourCommon.h>
#include <DetourNavMesh.h>

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <tuple>

namespace
{
    template <typename T>
    struct Ref
    {
        T& mRef;

        explicit Ref(T& ref) : mRef(ref) {}

        friend bool operator==(const Ref& lhs, const Ref& rhs)
        {
            return lhs.mRef == rhs.mRef;
        }
    };

    template <typename T, std::size_t size>
    struct ArrayRef
    {
        T (&mRef)[size];

        explicit ArrayRef(T (&ref)[size]) : mRef(ref) {}

        friend bool operator==(const ArrayRef& lhs, const ArrayRef& rhs)
        {
            return std::equal(std::begin(lhs.mRef), std::end(lhs.mRef), std::begin(rhs.mRef));
        }
    };

    template <typename T>
    struct Span
    {
        T* mBegin;
        T* mEnd;

        explicit Span(T* data, int size) : mBegin(data), mEnd(data + static_cast<std::size_t>(size)) {}

        friend bool operator==(const Span& lhs, const Span& rhs)
        {
            // size is already equal if headers are equal
            assert((lhs.mEnd - lhs.mBegin) == (rhs.mEnd - rhs.mBegin));
            return std::equal(lhs.mBegin, lhs.mEnd, rhs.mBegin);
        }
    };

    auto makeTuple(const dtMeshHeader& v)
    {
        return std::tuple(
            v.x,
            v.y,
            v.layer,
            v.userId,
            v.polyCount,
            v.vertCount,
            v.maxLinkCount,
            v.detailMeshCount,
            v.detailVertCount,
            v.detailTriCount,
            v.bvNodeCount,
            v.offMeshConCount,
            v.offMeshBase,
            v.walkableHeight,
            v.walkableRadius,
            v.walkableClimb,
            v.detailVertCount,
            ArrayRef(v.bmin),
            ArrayRef(v.bmax),
            v.bvQuantFactor
        );
    }

    auto makeTuple(const dtPoly& v)
    {
        return std::tuple(ArrayRef(v.verts), ArrayRef(v.neis), v.flags, v.vertCount, v.areaAndtype);
    }

    auto makeTuple(const dtPolyDetail& v)
    {
        return std::tuple(v.vertBase, v.triBase, v.vertCount, v.triCount);
    }

    auto makeTuple(const dtBVNode& v)
    {
        return std::tuple(ArrayRef(v.bmin), ArrayRef(v.bmax), v.i);
    }

    auto makeTuple(const dtOffMeshConnection& v)
    {
        return std::tuple(ArrayRef(v.pos), v.rad, v.poly, v.flags, v.side, v.userId);
    }

    auto makeTuple(const DetourNavigator::NavMeshTileConstView& v)
    {
        return std::tuple(
            Ref(*v.mHeader),
            Span(v.mPolys, v.mHeader->polyCount),
            Span(v.mVerts, v.mHeader->vertCount),
            Span(v.mDetailMeshes, v.mHeader->detailMeshCount),
            Span(v.mDetailVerts, v.mHeader->detailVertCount),
            Span(v.mDetailTris, v.mHeader->detailTriCount),
            Span(v.mBvTree, v.mHeader->bvNodeCount),
            Span(v.mOffMeshCons, v.mHeader->offMeshConCount)
        );
    }
}

template <class T>
inline auto operator==(const T& lhs, const T& rhs)
    -> std::enable_if_t<std::is_same_v<std::void_t<decltype(makeTuple(lhs))>, void>, bool>
{
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

        // Similar code to https://github.com/recastnavigation/recastnavigation/blob/c5cbd53024c8a9d8d097a4371215e3342d2fdc87/Detour/Source/DetourNavMesh.cpp#L978-L996
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

    bool operator==(const NavMeshTileConstView& lhs, const NavMeshTileConstView& rhs)
    {
        return makeTuple(lhs) == makeTuple(rhs);
    }
}
