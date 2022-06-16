#include "makenavmesh.hpp"
#include "debug.hpp"
#include "exceptions.hpp"
#include "recastmesh.hpp"
#include "settings.hpp"
#include "settingsutils.hpp"
#include "sharednavmesh.hpp"
#include "flags.hpp"
#include "navmeshtilescache.hpp"
#include "preparednavmeshdata.hpp"
#include "navmeshdata.hpp"
#include "recastmeshbuilder.hpp"
#include "navmeshdb.hpp"
#include "serialization.hpp"
#include "dbrefgeometryobject.hpp"
#include "navmeshdbutils.hpp"
#include "recastparams.hpp"

#include <components/misc/convert.hpp>
#include <components/bullethelpers/processtrianglecallback.hpp>
#include <components/misc/convert.hpp>
#include <components/misc/guarded.hpp>

#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <Recast.h>
#include <RecastAlloc.h>

#include <components/debug/debuglog.hpp>

#include <algorithm>
#include <iomanip>
#include <limits>
#include <array>

namespace DetourNavigator
{
namespace
{
    struct Rectangle
    {
        TileBounds mBounds;
        float mHeight;
    };

    std::vector<float> getOffMeshVerts(const std::vector<OffMeshConnection>& connections)
    {
        std::vector<float> result;

        result.reserve(connections.size() * 6);

        const auto add = [&] (const osg::Vec3f& v)
        {
            result.push_back(v.x());
            result.push_back(v.y());
            result.push_back(v.z());
        };

        for (const auto& v : connections)
        {
            add(v.mStart);
            add(v.mEnd);
        }

        return result;
    }

    Flag getFlag(AreaType areaType)
    {
        switch (areaType)
        {
            case AreaType_null:
                return Flag_none;
            case AreaType_ground:
                return Flag_walk;
            case AreaType_water:
                return Flag_swim;
            case AreaType_door:
                return Flag_openDoor;
            case AreaType_pathgrid:
                return Flag_usePathgrid;
        }
        return Flag_none;
    }

    std::vector<unsigned char> getOffMeshConAreas(const std::vector<OffMeshConnection>& connections)
    {
        std::vector<unsigned char> result;
        result.reserve(connections.size());
        std::transform(connections.begin(), connections.end(), std::back_inserter(result),
                       [] (const OffMeshConnection& v) { return v.mAreaType; });
        return result;
    }

    std::vector<unsigned short> getOffMeshFlags(const std::vector<OffMeshConnection>& connections)
    {
        std::vector<unsigned short> result;
        result.reserve(connections.size());
        std::transform(connections.begin(), connections.end(), std::back_inserter(result),
                       [] (const OffMeshConnection& v) { return getFlag(v.mAreaType); });
        return result;
    }

    float getHeight(const RecastSettings& settings,const AgentBounds& agentBounds)
    {
        return getAgentHeight(agentBounds) * settings.mRecastScaleFactor;
    }

    float getMaxClimb(const RecastSettings& settings)
    {
        return settings.mMaxClimb * settings.mRecastScaleFactor;
    }

    float getRadius(const RecastSettings& settings, const AgentBounds& agentBounds)
    {
        return getAgentRadius(agentBounds) * settings.mRecastScaleFactor;
    }

    float getSwimLevel(const RecastSettings& settings, const float waterLevel, const float agentHalfExtentsZ)
    {
        return waterLevel - settings.mSwimHeightScale * agentHalfExtentsZ - agentHalfExtentsZ;
    }

    struct RecastParams
    {
        float mSampleDist = 0;
        float mSampleMaxError = 0;
        int mMaxEdgeLen = 0;
        int mWalkableClimb = 0;
        int mWalkableHeight = 0;
        int mWalkableRadius = 0;
    };

    RecastParams makeRecastParams(const RecastSettings& settings, const AgentBounds& agentBounds)
    {
        RecastParams result;

        result.mWalkableHeight = static_cast<int>(std::ceil(getHeight(settings, agentBounds) / settings.mCellHeight));
        result.mWalkableClimb = static_cast<int>(std::floor(getMaxClimb(settings) / settings.mCellHeight));
        result.mWalkableRadius = static_cast<int>(std::ceil(getRadius(settings, agentBounds) / settings.mCellSize));
        result.mMaxEdgeLen = static_cast<int>(std::round(static_cast<float>(settings.mMaxEdgeLen) / settings.mCellSize));
        result.mSampleDist = settings.mDetailSampleDist < 0.9f ? 0 : settings.mCellSize * settings.mDetailSampleDist;
        result.mSampleMaxError = settings.mCellHeight * settings.mDetailSampleMaxError;

        return result;
    }

    void initHeightfield(rcContext& context, const TilePosition& tilePosition, float minZ, float maxZ,
        const RecastSettings& settings, rcHeightfield& solid)
    {
        const int size = settings.mTileSize + settings.mBorderSize * 2;
        const int width = size;
        const int height = size;
        const float halfBoundsSize = size * settings.mCellSize * 0.5f;
        const osg::Vec2f shift = osg::Vec2f(tilePosition.x() + 0.5f, tilePosition.y() + 0.5f) * getTileSize(settings);
        const osg::Vec3f bmin(shift.x() - halfBoundsSize, minZ, shift.y() - halfBoundsSize);
        const osg::Vec3f bmax(shift.x() + halfBoundsSize, maxZ, shift.y() + halfBoundsSize);

        const auto result = rcCreateHeightfield(&context, solid, width, height, bmin.ptr(), bmax.ptr(),
                                                settings.mCellSize, settings.mCellHeight);

        if (!result)
            throw NavigatorException("Failed to create heightfield for navmesh");
    }

    bool rasterizeTriangles(rcContext& context, const Mesh& mesh, const RecastSettings& settings,
        const RecastParams& params, rcHeightfield& solid)
    {
        std::vector<unsigned char> areas(mesh.getAreaTypes().begin(), mesh.getAreaTypes().end());
        std::vector<float> vertices = mesh.getVertices();

        for (std::size_t i = 0; i < vertices.size(); i += 3)
        {
            for (std::size_t j = 0; j < 3; ++j)
                vertices[i + j] = toNavMeshCoordinates(settings, vertices[i + j]);
            std::swap(vertices[i + 1], vertices[i + 2]);
        }

        rcClearUnwalkableTriangles(
            &context,
            settings.mMaxSlope,
            vertices.data(),
            static_cast<int>(mesh.getVerticesCount()),
            mesh.getIndices().data(),
            static_cast<int>(areas.size()),
            areas.data()
        );

        return rcRasterizeTriangles(
            &context,
            vertices.data(),
            static_cast<int>(mesh.getVerticesCount()),
            mesh.getIndices().data(),
            areas.data(),
            static_cast<int>(areas.size()),
            solid,
            params.mWalkableClimb
        );
    }

    bool rasterizeTriangles(rcContext& context, const Rectangle& rectangle, AreaType areaType,
        const RecastParams& params, rcHeightfield& solid)
    {
        const std::array vertices {
            rectangle.mBounds.mMin.x(), rectangle.mHeight, rectangle.mBounds.mMin.y(),
            rectangle.mBounds.mMin.x(), rectangle.mHeight, rectangle.mBounds.mMax.y(),
            rectangle.mBounds.mMax.x(), rectangle.mHeight, rectangle.mBounds.mMax.y(),
            rectangle.mBounds.mMax.x(), rectangle.mHeight, rectangle.mBounds.mMin.y(),
        };

        const std::array indices {
            0, 1, 2,
            0, 2, 3,
        };

        const std::array<unsigned char, 2> areas {areaType, areaType};

        return rcRasterizeTriangles(
            &context,
            vertices.data(),
            static_cast<int>(vertices.size() / 3),
            indices.data(),
            areas.data(),
            static_cast<int>(areas.size()),
            solid,
            params.mWalkableClimb
        );
    }

    bool rasterizeTriangles(rcContext& context, float agentHalfExtentsZ, const std::vector<CellWater>& water,
        const RecastSettings& settings, const RecastParams& params, const TileBounds& realTileBounds, rcHeightfield& solid)
    {
        for (const CellWater& cellWater : water)
        {
            const TileBounds cellTileBounds = maxCellTileBounds(cellWater.mCellPosition, cellWater.mWater.mCellSize);
            if (auto intersection = getIntersection(realTileBounds, cellTileBounds))
            {
                const Rectangle rectangle {
                    toNavMeshCoordinates(settings, *intersection),
                    toNavMeshCoordinates(settings, getSwimLevel(settings, cellWater.mWater.mLevel, agentHalfExtentsZ))
                };
                if (!rasterizeTriangles(context, rectangle, AreaType_water, params, solid))
                    return false;
            }
        }
        return true;
    }

    bool rasterizeTriangles(rcContext& context, const TileBounds& realTileBounds, const std::vector<FlatHeightfield>& heightfields,
        const RecastSettings& settings, const RecastParams& params, rcHeightfield& solid)
    {
        for (const FlatHeightfield& heightfield : heightfields)
        {
            const TileBounds cellTileBounds = maxCellTileBounds(heightfield.mCellPosition, heightfield.mCellSize);
            if (auto intersection = getIntersection(realTileBounds, cellTileBounds))
            {
                const Rectangle rectangle {
                    toNavMeshCoordinates(settings, *intersection),
                    toNavMeshCoordinates(settings, heightfield.mHeight)
                };
                if (!rasterizeTriangles(context, rectangle, AreaType_ground, params, solid))
                    return false;
            }
        }
        return true;
    }

    bool rasterizeTriangles(rcContext& context, const std::vector<Heightfield>& heightfields,
        const RecastSettings& settings, const RecastParams& params, rcHeightfield& solid)
    {
        for (const Heightfield& heightfield : heightfields)
        {
            const Mesh mesh = makeMesh(heightfield);
            if (!rasterizeTriangles(context, mesh, settings, params, solid))
                return false;
        }
        return true;
    }

    bool rasterizeTriangles(rcContext& context, const TilePosition& tilePosition, float agentHalfExtentsZ,
        const RecastMesh& recastMesh, const RecastSettings& settings, const RecastParams& params, rcHeightfield& solid)
    {
        const TileBounds realTileBounds = makeRealTileBoundsWithBorder(settings, tilePosition);
        return rasterizeTriangles(context, recastMesh.getMesh(), settings, params, solid)
            && rasterizeTriangles(context, agentHalfExtentsZ, recastMesh.getWater(), settings, params, realTileBounds, solid)
            && rasterizeTriangles(context, recastMesh.getHeightfields(), settings, params, solid)
            && rasterizeTriangles(context, realTileBounds, recastMesh.getFlatHeightfields(), settings, params, solid);
    }

    void buildCompactHeightfield(rcContext& context, const int walkableHeight, const int walkableClimb,
                                 rcHeightfield& solid, rcCompactHeightfield& compact)
    {
        const auto result = rcBuildCompactHeightfield(&context, walkableHeight,
            walkableClimb, solid, compact);

        if (!result)
            throw NavigatorException("Failed to build compact heightfield for navmesh");
    }

    void erodeWalkableArea(rcContext& context, int walkableRadius, rcCompactHeightfield& compact)
    {
        const auto result = rcErodeWalkableArea(&context, walkableRadius, compact);

        if (!result)
            throw NavigatorException("Failed to erode walkable area for navmesh");
    }

    void buildDistanceField(rcContext& context, rcCompactHeightfield& compact)
    {
        const auto result = rcBuildDistanceField(&context, compact);

        if (!result)
            throw NavigatorException("Failed to build distance field for navmesh");
    }

    void buildRegions(rcContext& context, rcCompactHeightfield& compact, const int borderSize,
        const int minRegionArea, const int mergeRegionArea)
    {
        const auto result = rcBuildRegions(&context, compact, borderSize, minRegionArea, mergeRegionArea);

        if (!result)
            throw NavigatorException("Failed to build distance field for navmesh");
    }

    void buildContours(rcContext& context, rcCompactHeightfield& compact, const float maxError, const int maxEdgeLen,
        rcContourSet& contourSet, const int buildFlags = RC_CONTOUR_TESS_WALL_EDGES)
    {
        const auto result = rcBuildContours(&context, compact, maxError, maxEdgeLen, contourSet, buildFlags);

        if (!result)
            throw NavigatorException("Failed to build contours for navmesh");
    }

    void buildPolyMesh(rcContext& context, rcContourSet& contourSet, const int maxVertsPerPoly, rcPolyMesh& polyMesh)
    {
        const auto result = rcBuildPolyMesh(&context, contourSet, maxVertsPerPoly, polyMesh);

        if (!result)
            throw NavigatorException("Failed to build poly mesh for navmesh");
    }

    void buildPolyMeshDetail(rcContext& context, const rcPolyMesh& polyMesh, const rcCompactHeightfield& compact,
        const float sampleDist, const float sampleMaxError, rcPolyMeshDetail& polyMeshDetail)
    {
        const auto result = rcBuildPolyMeshDetail(&context, polyMesh, compact, sampleDist, sampleMaxError,
                                                  polyMeshDetail);

        if (!result)
            throw NavigatorException("Failed to build detail poly mesh for navmesh");
    }

    void setPolyMeshFlags(rcPolyMesh& polyMesh)
    {
        for (int i = 0; i < polyMesh.npolys; ++i)
            polyMesh.flags[i] = getFlag(static_cast<AreaType>(polyMesh.areas[i]));
    }

    bool fillPolyMesh(rcContext& context, const RecastSettings& settings, const RecastParams& params,
        rcHeightfield& solid, rcPolyMesh& polyMesh, rcPolyMeshDetail& polyMeshDetail)
    {
        rcCompactHeightfield compact;
        buildCompactHeightfield(context, params.mWalkableHeight, params.mWalkableClimb, solid, compact);

        erodeWalkableArea(context, params.mWalkableRadius, compact);
        buildDistanceField(context, compact);
        buildRegions(context, compact, settings.mBorderSize, settings.mRegionMinArea, settings.mRegionMergeArea);

        rcContourSet contourSet;
        buildContours(context, compact, settings.mMaxSimplificationError, params.mMaxEdgeLen, contourSet);

        if (contourSet.nconts == 0)
            return false;

        buildPolyMesh(context, contourSet, settings.mMaxVertsPerPoly, polyMesh);

        buildPolyMeshDetail(context, polyMesh, compact, params.mSampleDist, params.mSampleMaxError, polyMeshDetail);

        setPolyMeshFlags(polyMesh);

        return true;
    }

    template <class T>
    unsigned long getMinValuableBitsNumber(const T value)
    {
        unsigned long power = 0;
        while (power < sizeof(T) * 8 && (static_cast<T>(1) << power) < value)
            ++power;
        return power;
    }

    std::pair<float, float> getBoundsByZ(const RecastMesh& recastMesh, float agentHalfExtentsZ, const RecastSettings& settings)
    {
        float minZ = 0;
        float maxZ = 0;

        const std::vector<float>& vertices = recastMesh.getMesh().getVertices();
        for (std::size_t i = 0, n = vertices.size(); i < n; i += 3)
        {
            minZ = std::min(minZ, vertices[i + 2]);
            maxZ = std::max(maxZ, vertices[i + 2]);
        }

        for (const CellWater& water : recastMesh.getWater())
        {
            const float swimLevel = getSwimLevel(settings, water.mWater.mLevel, agentHalfExtentsZ);
            minZ = std::min(minZ, swimLevel);
            maxZ = std::max(maxZ, swimLevel);
        }

        for (const Heightfield& heightfield : recastMesh.getHeightfields())
        {
            if (heightfield.mHeights.empty())
                continue;
            const auto [minHeight, maxHeight] = std::minmax_element(heightfield.mHeights.begin(), heightfield.mHeights.end());
            minZ = std::min(minZ, *minHeight);
            maxZ = std::max(maxZ, *maxHeight);
        }

        for (const FlatHeightfield& heightfield : recastMesh.getFlatHeightfields())
        {
            minZ = std::min(minZ, heightfield.mHeight);
            maxZ = std::max(maxZ, heightfield.mHeight);
        }

        return {minZ, maxZ};
    }
}
} // namespace DetourNavigator

namespace DetourNavigator
{
    std::unique_ptr<PreparedNavMeshData> prepareNavMeshTileData(const RecastMesh& recastMesh,
        const TilePosition& tilePosition, const AgentBounds& agentBounds, const RecastSettings& settings)
    {
        rcContext context;

        const auto [minZ, maxZ] = getBoundsByZ(recastMesh, agentBounds.mHalfExtents.z(), settings);

        rcHeightfield solid;
        initHeightfield(context, tilePosition, toNavMeshCoordinates(settings, minZ),
                        toNavMeshCoordinates(settings, maxZ), settings, solid);

        const RecastParams params = makeRecastParams(settings, agentBounds);

        if (!rasterizeTriangles(context, tilePosition, agentBounds.mHalfExtents.z(), recastMesh, settings, params, solid))
            return nullptr;

        rcFilterLowHangingWalkableObstacles(&context, params.mWalkableClimb, solid);
        rcFilterLedgeSpans(&context, params.mWalkableHeight, params.mWalkableClimb, solid);
        rcFilterWalkableLowHeightSpans(&context, params.mWalkableHeight, solid);

        std::unique_ptr<PreparedNavMeshData> result = std::make_unique<PreparedNavMeshData>();

        if (!fillPolyMesh(context, settings, params, solid, result->mPolyMesh, result->mPolyMeshDetail))
            return nullptr;

        result->mCellSize = settings.mCellSize;
        result->mCellHeight = settings.mCellHeight;

        return result;
    }

    NavMeshData makeNavMeshTileData(const PreparedNavMeshData& data,
        const std::vector<OffMeshConnection>& offMeshConnections, const AgentBounds& agentBounds,
        const TilePosition& tile, const RecastSettings& settings)
    {
        const auto offMeshConVerts = getOffMeshVerts(offMeshConnections);
        const std::vector<float> offMeshConRad(offMeshConnections.size(), getRadius(settings, agentBounds));
        const std::vector<unsigned char> offMeshConDir(offMeshConnections.size(), 0);
        const std::vector<unsigned char> offMeshConAreas = getOffMeshConAreas(offMeshConnections);
        const std::vector<unsigned short> offMeshConFlags = getOffMeshFlags(offMeshConnections);

        dtNavMeshCreateParams params;
        params.verts = data.mPolyMesh.verts;
        params.vertCount = data.mPolyMesh.nverts;
        params.polys = data.mPolyMesh.polys;
        params.polyAreas = data.mPolyMesh.areas;
        params.polyFlags = data.mPolyMesh.flags;
        params.polyCount = data.mPolyMesh.npolys;
        params.nvp = data.mPolyMesh.nvp;
        params.detailMeshes = data.mPolyMeshDetail.meshes;
        params.detailVerts = data.mPolyMeshDetail.verts;
        params.detailVertsCount = data.mPolyMeshDetail.nverts;
        params.detailTris = data.mPolyMeshDetail.tris;
        params.detailTriCount = data.mPolyMeshDetail.ntris;
        params.offMeshConVerts = offMeshConVerts.data();
        params.offMeshConRad = offMeshConRad.data();
        params.offMeshConDir = offMeshConDir.data();
        params.offMeshConAreas = offMeshConAreas.data();
        params.offMeshConFlags = offMeshConFlags.data();
        params.offMeshConUserID = nullptr;
        params.offMeshConCount = static_cast<int>(offMeshConnections.size());
        params.walkableHeight = getHeight(settings, agentBounds);
        params.walkableRadius = getRadius(settings, agentBounds);
        params.walkableClimb = getMaxClimb(settings);
        rcVcopy(params.bmin, data.mPolyMesh.bmin);
        rcVcopy(params.bmax, data.mPolyMesh.bmax);
        params.cs = data.mCellSize;
        params.ch = data.mCellHeight;
        params.buildBvTree = true;
        params.userId = data.mUserId;
        params.tileX = tile.x();
        params.tileY = tile.y();
        params.tileLayer = 0;

        unsigned char* navMeshData;
        int navMeshDataSize;
        const auto navMeshDataCreated = dtCreateNavMeshData(&params, &navMeshData, &navMeshDataSize);

        if (!navMeshDataCreated)
            throw NavigatorException("Failed to create navmesh tile data");

        return NavMeshData(navMeshData, navMeshDataSize);
    }

    NavMeshPtr makeEmptyNavMesh(const Settings& settings)
    {
        // Max tiles and max polys affect how the tile IDs are caculated.
        // There are 22 bits available for identifying a tile and a polygon.
        const int polysAndTilesBits = 22;
        const auto polysBits = getMinValuableBitsNumber(settings.mDetour.mMaxPolys);

        if (polysBits >= polysAndTilesBits)
            throw InvalidArgument("Too many polygons per tile");

        const auto tilesBits = polysAndTilesBits - polysBits;

        dtNavMeshParams params;
        std::fill_n(params.orig, 3, 0.0f);
        params.tileWidth = settings.mRecast.mTileSize * settings.mRecast.mCellSize;
        params.tileHeight = settings.mRecast.mTileSize * settings.mRecast.mCellSize;
        params.maxTiles = 1 << tilesBits;
        params.maxPolys = 1 << polysBits;

        NavMeshPtr navMesh(dtAllocNavMesh(), &dtFreeNavMesh);

        if (navMesh == nullptr)
            throw NavigatorException("Failed to allocate navmesh");

        const auto status = navMesh->init(&params);

        if (!dtStatusSucceed(status))
            throw NavigatorException("Failed to init navmesh");

        return navMesh;
    }
}
