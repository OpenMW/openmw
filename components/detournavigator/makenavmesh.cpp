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

#include <components/misc/convert.hpp>
#include <components/bullethelpers/processtrianglecallback.hpp>

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

    Rectangle getSwimRectangle(const CellWater& water, const Settings& settings, const osg::Vec3f& agentHalfExtents)
    {
        if (water.mWater.mCellSize == std::numeric_limits<int>::max())
        {
            return Rectangle {
                TileBounds {
                    osg::Vec2f(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()),
                    osg::Vec2f(std::numeric_limits<float>::max(), std::numeric_limits<float>::max())
                },
                toNavMeshCoordinates(settings, getSwimLevel(settings, water.mWater.mLevel, agentHalfExtents.z()))
            };
        }
        else
        {
            const osg::Vec2f shift = getWaterShift2d(water.mCellPosition, water.mWater.mCellSize);
            const float halfCellSize = water.mWater.mCellSize / 2.0f;
            return Rectangle {
                TileBounds{
                    toNavMeshCoordinates(settings, shift + osg::Vec2f(-halfCellSize, -halfCellSize)),
                    toNavMeshCoordinates(settings, shift + osg::Vec2f(halfCellSize, halfCellSize))
                },
                toNavMeshCoordinates(settings, getSwimLevel(settings, water.mWater.mLevel, agentHalfExtents.z()))
            };
        }
    }

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

    rcConfig makeConfig(const osg::Vec3f& agentHalfExtents, const TilePosition& tile, float minZ, float maxZ,
        const Settings& settings)
    {
        rcConfig config;

        config.cs = settings.mCellSize;
        config.ch = settings.mCellHeight;
        config.walkableSlopeAngle = settings.mMaxSlope;
        config.walkableHeight = static_cast<int>(std::ceil(getHeight(settings, agentHalfExtents) / config.ch));
        config.walkableClimb = static_cast<int>(std::floor(getMaxClimb(settings) / config.ch));
        config.walkableRadius = static_cast<int>(std::ceil(getRadius(settings, agentHalfExtents) / config.cs));
        config.maxEdgeLen = static_cast<int>(std::round(settings.mMaxEdgeLen / config.cs));
        config.maxSimplificationError = settings.mMaxSimplificationError;
        config.minRegionArea = settings.mRegionMinSize * settings.mRegionMinSize;
        config.mergeRegionArea = settings.mRegionMergeSize * settings.mRegionMergeSize;
        config.maxVertsPerPoly = settings.mMaxVertsPerPoly;
        config.detailSampleDist = settings.mDetailSampleDist < 0.9f ? 0 : config.cs * settings.mDetailSampleDist;
        config.detailSampleMaxError = config.ch * settings.mDetailSampleMaxError;
        config.borderSize = settings.mBorderSize;
        config.tileSize = settings.mTileSize;
        const int size = config.tileSize + config.borderSize * 2;
        config.width = size;
        config.height = size;
        const float halfBoundsSize = size * config.cs * 0.5f;
        const osg::Vec2f shift = osg::Vec2f(tile.x() + 0.5f, tile.y() + 0.5f) * getTileSize(settings);
        config.bmin[0] = shift.x() - halfBoundsSize;
        config.bmin[1] = minZ;
        config.bmin[2] = shift.y() - halfBoundsSize;
        config.bmax[0] = shift.x() + halfBoundsSize;
        config.bmax[1] = maxZ;
        config.bmax[2] = shift.y() + halfBoundsSize;

        return config;
    }

    void createHeightfield(rcContext& context, rcHeightfield& solid, int width, int height, const float* bmin,
        const float* bmax, const float cs, const float ch)
    {
        const auto result = rcCreateHeightfield(&context, solid, width, height, bmin, bmax, cs, ch);

        if (!result)
            throw NavigatorException("Failed to create heightfield for navmesh");
    }

    bool rasterizeTriangles(rcContext& context, const Mesh& mesh, const Settings& settings, const rcConfig& config,
        rcHeightfield& solid)
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
            config.walkableSlopeAngle,
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
            config.walkableClimb
        );
    }

    bool rasterizeTriangles(rcContext& context, const Rectangle& rectangle, const rcConfig& config,
        AreaType areaType, rcHeightfield& solid)
    {
        const osg::Vec2f tileBoundsMin(
            std::clamp(rectangle.mBounds.mMin.x(), config.bmin[0], config.bmax[0]),
            std::clamp(rectangle.mBounds.mMin.y(), config.bmin[2], config.bmax[2])
        );
        const osg::Vec2f tileBoundsMax(
            std::clamp(rectangle.mBounds.mMax.x(), config.bmin[0], config.bmax[0]),
            std::clamp(rectangle.mBounds.mMax.y(), config.bmin[2], config.bmax[2])
        );

        if (tileBoundsMax == tileBoundsMin)
            return true;

        const std::array vertices {
            tileBoundsMin.x(), rectangle.mHeight, tileBoundsMin.y(),
            tileBoundsMin.x(), rectangle.mHeight, tileBoundsMax.y(),
            tileBoundsMax.x(), rectangle.mHeight, tileBoundsMax.y(),
            tileBoundsMax.x(), rectangle.mHeight, tileBoundsMin.y(),
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
            config.walkableClimb
        );
    }

    bool rasterizeTriangles(rcContext& context, const osg::Vec3f& agentHalfExtents, const std::vector<CellWater>& water,
        const Settings& settings, const rcConfig& config, rcHeightfield& solid)
    {
        for (const CellWater& cellWater : water)
        {
            const Rectangle rectangle = getSwimRectangle(cellWater, settings, agentHalfExtents);
            if (!rasterizeTriangles(context, rectangle, config, AreaType_water, solid))
                return false;
        }
        return true;
    }

    bool rasterizeTriangles(rcContext& context, const TileBounds& tileBounds, const std::vector<FlatHeightfield>& heightfields,
        const Settings& settings, const rcConfig& config, rcHeightfield& solid)
    {
        for (const FlatHeightfield& heightfield : heightfields)
        {
            if (auto intersection = getIntersection(tileBounds, maxCellTileBounds(heightfield.mCellPosition, heightfield.mCellSize)))
            {
                const Rectangle rectangle {*intersection, toNavMeshCoordinates(settings, heightfield.mHeight)};
                if (!rasterizeTriangles(context, rectangle, config, AreaType_ground, solid))
                    return false;
            }
        }
        return true;
    }

    bool rasterizeTriangles(rcContext& context, const std::vector<Heightfield>& heightfields,
        const Settings& settings, const rcConfig& config, rcHeightfield& solid)
    {
        using BulletHelpers::makeProcessTriangleCallback;

        for (const Heightfield& heightfield : heightfields)
        {
            const Mesh mesh = makeMesh(heightfield);
            if (!rasterizeTriangles(context, mesh, settings, config, solid))
                return false;
        }
        return true;
    }

    bool rasterizeTriangles(rcContext& context, const TilePosition& tilePosition, const osg::Vec3f& agentHalfExtents,
        const RecastMesh& recastMesh, const rcConfig& config, const Settings& settings, rcHeightfield& solid)
    {
        return rasterizeTriangles(context, recastMesh.getMesh(), settings, config, solid)
            && rasterizeTriangles(context, agentHalfExtents, recastMesh.getWater(), settings, config, solid)
            && rasterizeTriangles(context, recastMesh.getHeightfields(), settings, config, solid)
            && rasterizeTriangles(context, makeRealTileBoundsWithBorder(settings, tilePosition),
                                  recastMesh.getFlatHeightfields(), settings, config, solid);
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

    bool fillPolyMesh(rcContext& context, const rcConfig& config, rcHeightfield& solid, rcPolyMesh& polyMesh,
        rcPolyMeshDetail& polyMeshDetail)
    {
        rcCompactHeightfield compact;
        compact.dist = nullptr;
        buildCompactHeightfield(context, config.walkableHeight, config.walkableClimb, solid, compact);

        erodeWalkableArea(context, config.walkableRadius, compact);
        buildDistanceField(context, compact);
        buildRegions(context, compact, config.borderSize, config.minRegionArea, config.mergeRegionArea);

        rcContourSet contourSet;
        buildContours(context, compact, config.maxSimplificationError, config.maxEdgeLen, contourSet);

        if (contourSet.nconts == 0)
            return false;

        buildPolyMesh(context, contourSet, config.maxVertsPerPoly, polyMesh);

        buildPolyMeshDetail(context, polyMesh, compact, config.detailSampleDist, config.detailSampleMaxError,
                            polyMeshDetail);

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

    std::pair<float, float> getBoundsByZ(const RecastMesh& recastMesh, const osg::Vec3f& agentHalfExtents, const Settings& settings)
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
            const float swimLevel = getSwimLevel(settings, water.mWater.mLevel, agentHalfExtents.z());
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
        const TilePosition& tilePosition, const osg::Vec3f& agentHalfExtents, const Settings& settings)
    {
        const auto [minZ, maxZ] = getBoundsByZ(recastMesh, agentHalfExtents, settings);

        rcContext context;
        const auto config = makeConfig(agentHalfExtents, tilePosition, toNavMeshCoordinates(settings, minZ),
                                       toNavMeshCoordinates(settings, maxZ), settings);

        rcHeightfield solid;
        createHeightfield(context, solid, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch);

        if (!rasterizeTriangles(context, tilePosition, agentHalfExtents, recastMesh, config, settings, solid))
            return nullptr;

        rcFilterLowHangingWalkableObstacles(&context, config.walkableClimb, solid);
        rcFilterLedgeSpans(&context, config.walkableHeight, config.walkableClimb, solid);
        rcFilterWalkableLowHeightSpans(&context, config.walkableHeight, solid);

        std::unique_ptr<PreparedNavMeshData> result = std::make_unique<PreparedNavMeshData>();

        if (!fillPolyMesh(context, config, solid, result->mPolyMesh, result->mPolyMeshDetail))
            return nullptr;

        result->mCellSize = config.cs;
        result->mCellHeight = config.ch;

        return result;
    }

    NavMeshData makeNavMeshTileData(const PreparedNavMeshData& data,
        const std::vector<OffMeshConnection>& offMeshConnections, const osg::Vec3f& agentHalfExtents,
        const TilePosition& tile, const Settings& settings)
    {
        const auto offMeshConVerts = getOffMeshVerts(offMeshConnections);
        const std::vector<float> offMeshConRad(offMeshConnections.size(), getRadius(settings, agentHalfExtents));
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
        params.walkableHeight = getHeight(settings, agentHalfExtents);
        params.walkableRadius = getRadius(settings, agentHalfExtents);
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
        const auto polysBits = getMinValuableBitsNumber(settings.mMaxPolys);

        if (polysBits >= polysAndTilesBits)
            throw InvalidArgument("Too many polygons per tile");

        const auto tilesBits = polysAndTilesBits - polysBits;

        dtNavMeshParams params;
        std::fill_n(params.orig, 3, 0.0f);
        params.tileWidth = settings.mTileSize * settings.mCellSize;
        params.tileHeight = settings.mTileSize * settings.mCellSize;
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

    UpdateNavMeshStatus updateNavMesh(const osg::Vec3f& agentHalfExtents, const RecastMesh* recastMesh,
        const TilePosition& changedTile, const TilePosition& playerTile,
        const std::vector<OffMeshConnection>& offMeshConnections, const Settings& settings,
        const SharedNavMeshCacheItem& navMeshCacheItem, NavMeshTilesCache& navMeshTilesCache, UpdateType updateType)
    {
        Log(Debug::Debug) << std::fixed << std::setprecision(2) <<
            "Update NavMesh with multiple tiles:" <<
            " agentHeight=" << getHeight(settings, agentHalfExtents) <<
            " agentMaxClimb=" << getMaxClimb(settings) <<
            " agentRadius=" << getRadius(settings, agentHalfExtents) <<
            " changedTile=(" << changedTile << ")" <<
            " playerTile=(" << playerTile << ")" <<
            " changedTileDistance=" << getDistance(changedTile, playerTile);

        if (!recastMesh)
        {
            Log(Debug::Debug) << "Ignore add tile: recastMesh is null";
            return navMeshCacheItem->lock()->removeTile(changedTile);
        }

        if (recastMesh->getMesh().getIndices().empty() && recastMesh->getWater().empty()
                && recastMesh->getHeightfields().empty() && recastMesh->getFlatHeightfields().empty())
        {
            Log(Debug::Debug) << "Ignore add tile: recastMesh is empty";
            return navMeshCacheItem->lock()->removeTile(changedTile);
        }

        const dtNavMeshParams params = *navMeshCacheItem->lockConst()->getImpl().getParams();

        if (!shouldAddTile(changedTile, playerTile, std::min(settings.mMaxTilesNumber, params.maxTiles)))
        {
            Log(Debug::Debug) << "Ignore add tile: too far from player";
            return navMeshCacheItem->lock()->removeTile(changedTile);
        }

        auto cachedNavMeshData = navMeshTilesCache.get(agentHalfExtents, changedTile, *recastMesh);
        bool cached = static_cast<bool>(cachedNavMeshData);

        if (!cachedNavMeshData)
        {
            auto prepared = prepareNavMeshTileData(*recastMesh, changedTile, agentHalfExtents, settings);

            if (prepared == nullptr)
            {
                Log(Debug::Debug) << "Ignore add tile: NavMeshData is null";
                return navMeshCacheItem->lock()->removeTile(changedTile);
            }

            if (updateType == UpdateType::Temporary)
                return navMeshCacheItem->lock()->updateTile(changedTile, NavMeshTilesCache::Value(),
                    makeNavMeshTileData(*prepared, offMeshConnections, agentHalfExtents, changedTile, settings));

            cachedNavMeshData = navMeshTilesCache.set(agentHalfExtents, changedTile, *recastMesh, std::move(prepared));

            if (!cachedNavMeshData)
            {
                Log(Debug::Debug) << "Navigator cache overflow";
                return navMeshCacheItem->lock()->updateTile(changedTile, NavMeshTilesCache::Value(),
                    makeNavMeshTileData(*prepared, offMeshConnections, agentHalfExtents, changedTile, settings));
            }
        }

        const auto updateStatus = navMeshCacheItem->lock()->updateTile(changedTile, std::move(cachedNavMeshData),
            makeNavMeshTileData(cachedNavMeshData.get(), offMeshConnections, agentHalfExtents, changedTile, settings));

        return UpdateNavMeshStatusBuilder(updateStatus).cached(cached).getResult();
    }
}
