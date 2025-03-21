#include "makenavmesh.hpp"
#include "debug.hpp"
#include "exceptions.hpp"
#include "flags.hpp"
#include "navmeshdata.hpp"
#include "navmeshdb.hpp"
#include "navmeshtilescache.hpp"
#include "offmeshconnection.hpp"
#include "preparednavmeshdata.hpp"
#include "recastcontext.hpp"
#include "recastmesh.hpp"
#include "recastmeshbuilder.hpp"
#include "recastparams.hpp"
#include "settings.hpp"
#include "settingsutils.hpp"

#include "components/debug/debuglog.hpp"

#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <Recast.h>

#include <algorithm>
#include <array>
#include <iomanip>
#include <limits>

namespace DetourNavigator
{
    namespace
    {
        constexpr int walkableRadiusUpperLimit = 255;

        struct Rectangle
        {
            TileBounds mBounds;
            float mHeight;
        };

        std::vector<float> getOffMeshVerts(const std::vector<OffMeshConnection>& connections)
        {
            std::vector<float> result;

            result.reserve(connections.size() * 6);

            const auto add = [&](const osg::Vec3f& v) {
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
                [](const OffMeshConnection& v) { return v.mAreaType; });
            return result;
        }

        std::vector<unsigned short> getOffMeshFlags(const std::vector<OffMeshConnection>& connections)
        {
            std::vector<unsigned short> result;
            result.reserve(connections.size());
            std::transform(connections.begin(), connections.end(), std::back_inserter(result),
                [](const OffMeshConnection& v) { return getFlag(v.mAreaType); });
            return result;
        }

        float getHeight(const RecastSettings& settings, const AgentBounds& agentBounds)
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

        int getWalkableHeight(const RecastSettings& settings, const AgentBounds& agentBounds)
        {
            return static_cast<int>(std::ceil(getHeight(settings, agentBounds) / settings.mCellHeight));
        }

        int getWalkableRadius(const RecastSettings& settings, const AgentBounds& agentBounds)
        {
            return static_cast<int>(std::ceil(getRadius(settings, agentBounds) / settings.mCellSize));
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

            result.mWalkableHeight = getWalkableHeight(settings, agentBounds);
            result.mWalkableClimb = static_cast<int>(std::floor(getMaxClimb(settings) / settings.mCellHeight));
            result.mWalkableRadius = getWalkableRadius(settings, agentBounds);
            result.mMaxEdgeLen
                = static_cast<int>(std::round(static_cast<float>(settings.mMaxEdgeLen) / settings.mCellSize));
            result.mSampleDist
                = settings.mDetailSampleDist < 0.9f ? 0 : settings.mCellSize * settings.mDetailSampleDist;
            result.mSampleMaxError = settings.mCellHeight * settings.mDetailSampleMaxError;

            return result;
        }

        [[nodiscard]] bool initHeightfield(RecastContext& context, const TilePosition& tilePosition, float minZ,
            float maxZ, const RecastSettings& settings, rcHeightfield& solid)
        {
            const int size = settings.mTileSize + settings.mBorderSize * 2;
            const int width = size;
            const int height = size;
            const float halfBoundsSize = size * settings.mCellSize * 0.5f;
            const osg::Vec2f shift
                = osg::Vec2f(tilePosition.x() + 0.5f, tilePosition.y() + 0.5f) * getTileSize(settings);
            const osg::Vec3f bmin(shift.x() - halfBoundsSize, minZ, shift.y() - halfBoundsSize);
            const osg::Vec3f bmax(shift.x() + halfBoundsSize, maxZ, shift.y() + halfBoundsSize);

            if (size < 0)
            {
                Log(Debug::Warning) << context.getPrefix() << "Invalid size to init heightfield: " << size;
                return false;
            }

            if (settings.mCellHeight <= 0)
            {
                Log(Debug::Warning) << context.getPrefix()
                                    << "Invalid cell height to init heightfield: " << settings.mCellHeight;
                return false;
            }

            if (settings.mCellSize <= 0)
            {
                Log(Debug::Warning) << context.getPrefix()
                                    << "Invalid cell size to init heightfield: " << settings.mCellSize;
                return false;
            }

            return rcCreateHeightfield(
                &context, solid, width, height, bmin.ptr(), bmax.ptr(), settings.mCellSize, settings.mCellHeight);
        }

        bool isSupportedCoordinate(float value)
        {
            constexpr float maxVertexCoordinate = static_cast<float>(1 << 22);
            return -maxVertexCoordinate < value && value < maxVertexCoordinate;
        }

        template <class Iterator>
        bool isSupportedCoordinates(Iterator begin, Iterator end)
        {
            return std::all_of(begin, end, isSupportedCoordinate);
        }

        [[nodiscard]] bool rasterizeTriangles(RecastContext& context, const Mesh& mesh, const RecastSettings& settings,
            const RecastParams& params, rcHeightfield& solid)
        {
            std::vector<unsigned char> areas(mesh.getAreaTypes().begin(), mesh.getAreaTypes().end());
            std::vector<float> vertices = mesh.getVertices();

            constexpr std::size_t verticesPerTriangle = 3;

            for (std::size_t i = 0; i < vertices.size(); i += verticesPerTriangle)
            {
                for (std::size_t j = 0; j < verticesPerTriangle; ++j)
                {
                    const float coordinate = toNavMeshCoordinates(settings, vertices[i + j]);
                    if (!isSupportedCoordinate(coordinate))
                        return false;
                    vertices[i + j] = coordinate;
                }
                std::swap(vertices[i + 1], vertices[i + 2]);
            }

            rcClearUnwalkableTriangles(&context, settings.mMaxSlope, vertices.data(),
                static_cast<int>(mesh.getVerticesCount()), mesh.getIndices().data(), static_cast<int>(areas.size()),
                areas.data());

            return rcRasterizeTriangles(&context, vertices.data(), static_cast<int>(mesh.getVerticesCount()),
                mesh.getIndices().data(), areas.data(), static_cast<int>(areas.size()), solid, params.mWalkableClimb);
        }

        [[nodiscard]] bool rasterizeTriangles(RecastContext& context, const Rectangle& rectangle, AreaType areaType,
            const RecastParams& params, rcHeightfield& solid)
        {
            const std::array vertices{
                rectangle.mBounds.mMin.x(), rectangle.mHeight, rectangle.mBounds.mMin.y(), // vertex 0
                rectangle.mBounds.mMin.x(), rectangle.mHeight, rectangle.mBounds.mMax.y(), // vertex 1
                rectangle.mBounds.mMax.x(), rectangle.mHeight, rectangle.mBounds.mMax.y(), // vertex 2
                rectangle.mBounds.mMax.x(), rectangle.mHeight, rectangle.mBounds.mMin.y(), // vertex 3
            };

            if (!isSupportedCoordinates(vertices.begin(), vertices.end()))
                return false;

            const std::array indices{
                0, 1, 2, // triangle 0
                0, 2, 3, // triangle 1
            };

            const std::array<unsigned char, 2> areas{ areaType, areaType };

            return rcRasterizeTriangles(&context, vertices.data(), static_cast<int>(vertices.size() / 3),
                indices.data(), areas.data(), static_cast<int>(areas.size()), solid, params.mWalkableClimb);
        }

        [[nodiscard]] bool rasterizeTriangles(RecastContext& context, float agentHalfExtentsZ,
            const std::vector<CellWater>& water, const RecastSettings& settings, const RecastParams& params,
            const TileBounds& realTileBounds, rcHeightfield& solid)
        {
            for (const CellWater& cellWater : water)
            {
                const TileBounds cellTileBounds
                    = maxCellTileBounds(cellWater.mCellPosition, cellWater.mWater.mCellSize);
                if (auto intersection = getIntersection(realTileBounds, cellTileBounds))
                {
                    const Rectangle rectangle{ toNavMeshCoordinates(settings, *intersection),
                        toNavMeshCoordinates(
                            settings, getSwimLevel(settings, cellWater.mWater.mLevel, agentHalfExtentsZ)) };
                    if (!rasterizeTriangles(context, rectangle, AreaType_water, params, solid))
                        return false;
                }
            }
            return true;
        }

        [[nodiscard]] bool rasterizeTriangles(RecastContext& context, const TileBounds& realTileBounds,
            const std::vector<FlatHeightfield>& heightfields, const RecastSettings& settings,
            const RecastParams& params, rcHeightfield& solid)
        {
            for (const FlatHeightfield& heightfield : heightfields)
            {
                const TileBounds cellTileBounds = maxCellTileBounds(heightfield.mCellPosition, heightfield.mCellSize);
                if (auto intersection = getIntersection(realTileBounds, cellTileBounds))
                {
                    const Rectangle rectangle{ toNavMeshCoordinates(settings, *intersection),
                        toNavMeshCoordinates(settings, heightfield.mHeight) };
                    if (!rasterizeTriangles(context, rectangle, AreaType_ground, params, solid))
                        return false;
                }
            }
            return true;
        }

        [[nodiscard]] bool rasterizeTriangles(RecastContext& context, const std::vector<Heightfield>& heightfields,
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

        [[nodiscard]] bool rasterizeTriangles(RecastContext& context, const TilePosition& tilePosition,
            float agentHalfExtentsZ, const RecastMesh& recastMesh, const RecastSettings& settings,
            const RecastParams& params, rcHeightfield& solid)
        {
            const TileBounds realTileBounds = makeRealTileBoundsWithBorder(settings, tilePosition);
            return rasterizeTriangles(context, recastMesh.getMesh(), settings, params, solid)
                && rasterizeTriangles(
                    context, agentHalfExtentsZ, recastMesh.getWater(), settings, params, realTileBounds, solid)
                && rasterizeTriangles(context, recastMesh.getHeightfields(), settings, params, solid)
                && rasterizeTriangles(
                    context, realTileBounds, recastMesh.getFlatHeightfields(), settings, params, solid);
        }

        bool isValidWalkableHeight(int value)
        {
            return value >= 3;
        }

        [[nodiscard]] bool buildCompactHeightfield(RecastContext& context, const int walkableHeight,
            const int walkableClimb, rcHeightfield& solid, rcCompactHeightfield& compact)
        {
            if (!isValidWalkableHeight(walkableHeight))
            {
                Log(Debug::Warning) << context.getPrefix()
                                    << "Invalid walkableHeight to build compact heightfield: " << walkableHeight;
                return false;
            }

            if (walkableClimb < 0)
            {
                Log(Debug::Warning) << context.getPrefix()
                                    << "Invalid walkableClimb to build compact heightfield: " << walkableClimb;
                return false;
            }

            return rcBuildCompactHeightfield(&context, walkableHeight, walkableClimb, solid, compact);
        }

        bool isValidWalkableRadius(int value)
        {
            return 0 < value && value < walkableRadiusUpperLimit;
        }

        [[nodiscard]] bool erodeWalkableArea(RecastContext& context, int walkableRadius, rcCompactHeightfield& compact)
        {
            if (!isValidWalkableRadius(walkableRadius))
            {
                Log(Debug::Warning) << context.getPrefix()
                                    << "Invalid walkableRadius to erode walkable area: " << walkableRadius;
                return false;
            }

            return rcErodeWalkableArea(&context, walkableRadius, compact);
        }

        [[nodiscard]] bool buildDistanceField(RecastContext& context, rcCompactHeightfield& compact)
        {
            return rcBuildDistanceField(&context, compact);
        }

        [[nodiscard]] bool buildRegions(RecastContext& context, rcCompactHeightfield& compact, const int borderSize,
            const int minRegionArea, const int mergeRegionArea)
        {
            if (borderSize < 0)
            {
                Log(Debug::Warning) << context.getPrefix() << "Invalid borderSize to build regions: " << borderSize;
                return false;
            }

            if (minRegionArea < 0)
            {
                Log(Debug::Warning) << context.getPrefix()
                                    << "Invalid minRegionArea to build regions: " << minRegionArea;
                return false;
            }

            if (mergeRegionArea < 0)
            {
                Log(Debug::Warning) << context.getPrefix()
                                    << "Invalid mergeRegionArea to build regions: " << mergeRegionArea;
                return false;
            }

            return rcBuildRegions(&context, compact, borderSize, minRegionArea, mergeRegionArea);
        }

        [[nodiscard]] bool buildContours(RecastContext& context, rcCompactHeightfield& compact, const float maxError,
            const int maxEdgeLen, rcContourSet& contourSet, const int buildFlags = RC_CONTOUR_TESS_WALL_EDGES)
        {
            if (maxError < 0)
            {
                Log(Debug::Warning) << context.getPrefix() << "Invalid maxError to build contours: " << maxError;
                return false;
            }

            if (maxEdgeLen < 0)
            {
                Log(Debug::Warning) << context.getPrefix() << "Invalid maxEdgeLen to build contours: " << maxEdgeLen;
                return false;
            }

            return rcBuildContours(&context, compact, maxError, maxEdgeLen, contourSet, buildFlags);
        }

        [[nodiscard]] bool buildPolyMesh(
            RecastContext& context, rcContourSet& contourSet, const int maxVertsPerPoly, rcPolyMesh& polyMesh)
        {
            if (maxVertsPerPoly < 3)
            {
                Log(Debug::Warning) << context.getPrefix()
                                    << "Invalid maxVertsPerPoly to build poly mesh: " << maxVertsPerPoly;
                return false;
            }

            return rcBuildPolyMesh(&context, contourSet, maxVertsPerPoly, polyMesh);
        }

        [[nodiscard]] bool buildPolyMeshDetail(RecastContext& context, const rcPolyMesh& polyMesh,
            const rcCompactHeightfield& compact, const float sampleDist, const float sampleMaxError,
            rcPolyMeshDetail& polyMeshDetail)
        {
            if (sampleDist < 0)
            {
                Log(Debug::Warning) << context.getPrefix()
                                    << "Invalid sampleDist to build poly mesh detail: " << sampleDist;
                return false;
            }

            if (sampleMaxError < 0)
            {
                Log(Debug::Warning) << context.getPrefix()
                                    << "Invalid sampleMaxError to build poly mesh detail: " << sampleMaxError;
                return false;
            }

            return rcBuildPolyMeshDetail(&context, polyMesh, compact, sampleDist, sampleMaxError, polyMeshDetail);
        }

        void setPolyMeshFlags(rcPolyMesh& polyMesh)
        {
            for (int i = 0; i < polyMesh.npolys; ++i)
                polyMesh.flags[i] = getFlag(static_cast<AreaType>(polyMesh.areas[i]));
        }

        [[nodiscard]] bool fillPolyMesh(RecastContext& context, const RecastSettings& settings,
            const RecastParams& params, rcHeightfield& solid, rcPolyMesh& polyMesh, rcPolyMeshDetail& polyMeshDetail)
        {
            rcCompactHeightfield compact;
            if (!buildCompactHeightfield(context, params.mWalkableHeight, params.mWalkableClimb, solid, compact))
                return false;

            if (!erodeWalkableArea(context, params.mWalkableRadius, compact))
                return false;

            if (!buildDistanceField(context, compact))
                return false;

            if (!buildRegions(
                    context, compact, settings.mBorderSize, settings.mRegionMinArea, settings.mRegionMergeArea))
                return false;

            rcContourSet contourSet;
            if (!buildContours(context, compact, settings.mMaxSimplificationError, params.mMaxEdgeLen, contourSet))
                return false;

            if (contourSet.nconts == 0)
                return false;

            if (!buildPolyMesh(context, contourSet, settings.mMaxVertsPerPoly, polyMesh))
                return false;

            if (!buildPolyMeshDetail(
                    context, polyMesh, compact, params.mSampleDist, params.mSampleMaxError, polyMeshDetail))
                return false;

            setPolyMeshFlags(polyMesh);

            return true;
        }

        std::pair<float, float> getBoundsByZ(
            const RecastMesh& recastMesh, float agentHalfExtentsZ, const RecastSettings& settings)
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
                const auto [minHeight, maxHeight]
                    = std::minmax_element(heightfield.mHeights.begin(), heightfield.mHeights.end());
                minZ = std::min(minZ, *minHeight);
                maxZ = std::max(maxZ, *maxHeight);
            }

            for (const FlatHeightfield& heightfield : recastMesh.getFlatHeightfields())
            {
                minZ = std::min(minZ, heightfield.mHeight);
                maxZ = std::max(maxZ, heightfield.mHeight);
            }

            return { minZ, maxZ };
        }
    }

    std::unique_ptr<PreparedNavMeshData> prepareNavMeshTileData(const RecastMesh& recastMesh, ESM::RefId worldspace,
        const TilePosition& tilePosition, const AgentBounds& agentBounds, const RecastSettings& settings)
    {
        RecastContext context(worldspace, tilePosition, agentBounds, settings.mMaxLogLevel);

        const auto [minZ, maxZ] = getBoundsByZ(recastMesh, agentBounds.mHalfExtents.z(), settings);

        rcHeightfield solid;
        if (!initHeightfield(context, tilePosition, toNavMeshCoordinates(settings, minZ),
                toNavMeshCoordinates(settings, maxZ), settings, solid))
            return nullptr;

        const RecastParams params = makeRecastParams(settings, agentBounds);

        if (!rasterizeTriangles(
                context, tilePosition, agentBounds.mHalfExtents.z(), recastMesh, settings, params, solid))
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

    void initEmptyNavMesh(const Settings& settings, dtNavMesh& navMesh)
    {
        dtNavMeshParams params;
        std::fill_n(params.orig, 3, 0.0f);
        params.tileWidth = settings.mRecast.mTileSize * settings.mRecast.mCellSize;
        params.tileHeight = settings.mRecast.mTileSize * settings.mRecast.mCellSize;
        params.maxTiles = settings.mMaxTilesNumber;
        params.maxPolys = settings.mDetour.mMaxPolys;

        const auto status = navMesh.init(&params);

        if (!dtStatusSucceed(status))
            throw NavigatorException("Failed to init navmesh");
    }

    bool isSupportedAgentBounds(const RecastSettings& settings, const AgentBounds& agentBounds)
    {
        return isValidWalkableHeight(getWalkableHeight(settings, agentBounds))
            && isValidWalkableRadius(getWalkableRadius(settings, agentBounds));
    }
}
