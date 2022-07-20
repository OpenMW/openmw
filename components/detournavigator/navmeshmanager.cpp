#include "navmeshmanager.hpp"
#include "debug.hpp"
#include "exceptions.hpp"
#include "gettilespositions.hpp"
#include "makenavmesh.hpp"
#include "navmeshcacheitem.hpp"
#include "settings.hpp"
#include "waitconditiontype.hpp"

#include <components/debug/debuglog.hpp>
#include <components/bullethelpers/heightfield.hpp>
#include <components/misc/convert.hpp>

#include <osg/io_utils>

#include <DetourNavMesh.h>

#include <iterator>

namespace
{
    using DetourNavigator::ChangeType;

    ChangeType addChangeType(const ChangeType current, const ChangeType add)
    {
        return current == add ? current : ChangeType::mixed;
    }

    /// Safely reset shared_ptr with definite underlying object destrutor call.
    /// Assuming there is another thread holding copy of this shared_ptr or weak_ptr to this shared_ptr.
    template <class T>
    bool resetIfUnique(std::shared_ptr<T>& ptr)
    {
        const std::weak_ptr<T> weak(ptr);
        ptr.reset();
        if (auto shared = weak.lock())
        {
            ptr = std::move(shared);
            return false;
        }
        return true;
    }
}

namespace DetourNavigator
{
    namespace
    {
        TileBounds makeBounds(const RecastSettings& settings, const osg::Vec2f& center, int maxTiles)
        {
            const float radius = fromNavMeshCoordinates(settings, std::ceil(std::sqrt(static_cast<float>(maxTiles) / osg::PIf) + 1) * getTileSize(settings));
            TileBounds result;
            result.mMin = center - osg::Vec2f(radius, radius);
            result.mMax = center + osg::Vec2f(radius, radius);
            return result;
        }
    }

    NavMeshManager::NavMeshManager(const Settings& settings, std::unique_ptr<NavMeshDb>&& db)
        : mSettings(settings)
        , mRecastMeshManager(settings.mRecast)
        , mOffMeshConnectionsManager(settings.mRecast)
        , mAsyncNavMeshUpdater(settings, mRecastMeshManager, mOffMeshConnectionsManager, std::move(db))
    {}

    void NavMeshManager::setWorldspace(std::string_view worldspace)
    {
        if (worldspace == mWorldspace)
            return;
        mRecastMeshManager.setWorldspace(worldspace);
        for (auto& [agent, cache] : mCache)
            cache = std::make_shared<GuardedNavMeshCacheItem>(makeEmptyNavMesh(mSettings), ++mGenerationCounter);
        mWorldspace = worldspace;
    }

    void NavMeshManager::updateBounds(const osg::Vec3f& playerPosition)
    {
        const TileBounds bounds = makeBounds(mSettings.mRecast, osg::Vec2f(playerPosition.x(), playerPosition.y()),
                                             mSettings.mMaxTilesNumber);
        const auto changedTiles = mRecastMeshManager.setBounds(bounds);
        for (const auto& [agent, cache] : mCache)
        {
            auto& tiles = mChangedTiles[agent];
            for (const auto& [tilePosition, changeType] : changedTiles)
            {
                auto tile = tiles.find(tilePosition);
                if (tile == tiles.end())
                    tiles.emplace_hint(tile, tilePosition, changeType);
                else
                    tile->second = addChangeType(tile->second, changeType);
            }
        }
    }

    bool NavMeshManager::addObject(const ObjectId id, const CollisionShape& shape, const btTransform& transform,
                                   const AreaType areaType)
    {
        return mRecastMeshManager.addObject(id, shape, transform, areaType,
            [&] (const TilePosition& tile) { addChangedTile(tile, ChangeType::add); });
    }

    bool NavMeshManager::updateObject(const ObjectId id, const CollisionShape& shape, const btTransform& transform,
                                      const AreaType areaType)
    {
        return mRecastMeshManager.updateObject(id, shape, transform, areaType,
            [&] (const TilePosition& tile, ChangeType changeType) { addChangedTile(tile, changeType); });
    }

    bool NavMeshManager::removeObject(const ObjectId id)
    {
        const auto object = mRecastMeshManager.removeObject(id);
        if (!object)
            return false;
        addChangedTiles(object->mShape, object->mTransform, ChangeType::remove);
        return true;
    }

    bool NavMeshManager::addWater(const osg::Vec2i& cellPosition, int cellSize, float level)
    {
        if (!mRecastMeshManager.addWater(cellPosition, cellSize, level))
            return false;
        const btVector3 shift = Misc::Convert::toBullet(getWaterShift3d(cellPosition, cellSize, level));
        addChangedTiles(cellSize, shift, ChangeType::add);
        return true;
    }

    bool NavMeshManager::removeWater(const osg::Vec2i& cellPosition)
    {
        const auto water = mRecastMeshManager.removeWater(cellPosition);
        if (!water)
            return false;
        const btVector3 shift = Misc::Convert::toBullet(getWaterShift3d(cellPosition, water->mCellSize, water->mLevel));
        addChangedTiles(water->mCellSize, shift, ChangeType::remove);
        return true;
    }

    bool NavMeshManager::addHeightfield(const osg::Vec2i& cellPosition, int cellSize, const HeightfieldShape& shape)
    {
        if (!mRecastMeshManager.addHeightfield(cellPosition, cellSize, shape))
            return false;
        const btVector3 shift = getHeightfieldShift(shape, cellPosition, cellSize);
        addChangedTiles(cellSize, shift, ChangeType::add);
        return true;
    }

    bool NavMeshManager::removeHeightfield(const osg::Vec2i& cellPosition)
    {
        const auto heightfield = mRecastMeshManager.removeHeightfield(cellPosition);
        if (!heightfield)
            return false;
        const btVector3 shift = getHeightfieldShift(heightfield->mShape, cellPosition, heightfield->mCellSize);
        addChangedTiles(heightfield->mCellSize, shift, ChangeType::remove);
        return true;
    }

    void NavMeshManager::addAgent(const AgentBounds& agentBounds)
    {
        auto cached = mCache.find(agentBounds);
        if (cached != mCache.end())
            return;
        mCache.insert(std::make_pair(agentBounds,
            std::make_shared<GuardedNavMeshCacheItem>(makeEmptyNavMesh(mSettings), ++mGenerationCounter)));
        Log(Debug::Debug) << "cache add for agent=" << agentBounds;
    }

    bool NavMeshManager::reset(const AgentBounds& agentBounds)
    {
        const auto it = mCache.find(agentBounds);
        if (it == mCache.end())
            return true;
        if (!resetIfUnique(it->second))
            return false;
        mCache.erase(agentBounds);
        mChangedTiles.erase(agentBounds);
        mPlayerTile.erase(agentBounds);
        mLastRecastMeshManagerRevision.erase(agentBounds);
        return true;
    }

    void NavMeshManager::addOffMeshConnection(const ObjectId id, const osg::Vec3f& start, const osg::Vec3f& end, const AreaType areaType)
    {
        mOffMeshConnectionsManager.add(id, OffMeshConnection {start, end, areaType});

        const auto startTilePosition = getTilePosition(mSettings.mRecast, start);
        const auto endTilePosition = getTilePosition(mSettings.mRecast, end);

        addChangedTile(startTilePosition, ChangeType::add);

        if (startTilePosition != endTilePosition)
            addChangedTile(endTilePosition, ChangeType::add);
    }

    void NavMeshManager::removeOffMeshConnections(const ObjectId id)
    {
        const auto changedTiles = mOffMeshConnectionsManager.remove(id);
        for (const auto& tile : changedTiles)
            addChangedTile(tile, ChangeType::update);
    }

    void NavMeshManager::update(const osg::Vec3f& playerPosition, const AgentBounds& agentBounds)
    {
        const auto playerTile = getTilePosition(mSettings.mRecast, toNavMeshCoordinates(mSettings.mRecast, playerPosition));
        auto& lastRevision = mLastRecastMeshManagerRevision[agentBounds];
        auto lastPlayerTile = mPlayerTile.find(agentBounds);
        if (lastRevision == mRecastMeshManager.getRevision() && lastPlayerTile != mPlayerTile.end()
                && lastPlayerTile->second == playerTile)
            return;
        lastRevision = mRecastMeshManager.getRevision();
        if (lastPlayerTile == mPlayerTile.end())
            lastPlayerTile = mPlayerTile.insert(std::make_pair(agentBounds, playerTile)).first;
        else
            lastPlayerTile->second = playerTile;
        std::map<TilePosition, ChangeType> tilesToPost;
        const auto cached = getCached(agentBounds);
        if (!cached)
        {
            std::ostringstream stream;
            stream << "Agent with half extents is not found: " << agentBounds;
            throw InvalidArgument(stream.str());
        }
        const auto changedTiles = mChangedTiles.find(agentBounds);
        {
            const auto locked = cached->lockConst();
            const auto& navMesh = locked->getImpl();
            if (changedTiles != mChangedTiles.end())
            {
                for (const auto& tile : changedTiles->second)
                    if (navMesh.getTileAt(tile.first.x(), tile.first.y(), 0))
                    {
                        auto tileToPost = tilesToPost.find(tile.first);
                        if (tileToPost == tilesToPost.end())
                            tilesToPost.insert(tile);
                        else
                            tileToPost->second = addChangeType(tileToPost->second, tile.second);
                    }
            }
            const auto maxTiles = std::min(mSettings.mMaxTilesNumber, navMesh.getParams()->maxTiles);
            mRecastMeshManager.forEachTile([&] (const TilePosition& tile, CachedRecastMeshManager& recastMeshManager)
            {
                if (tilesToPost.count(tile))
                    return;
                const auto shouldAdd = shouldAddTile(tile, playerTile, maxTiles);
                const auto presentInNavMesh = bool(navMesh.getTileAt(tile.x(), tile.y(), 0));
                if (shouldAdd && !presentInNavMesh)
                    tilesToPost.insert(std::make_pair(tile, locked->isEmptyTile(tile) ? ChangeType::update : ChangeType::add));
                else if (!shouldAdd && presentInNavMesh)
                    tilesToPost.insert(std::make_pair(tile, ChangeType::mixed));
                else
                    recastMeshManager.reportNavMeshChange(recastMeshManager.getVersion(), Version {0, 0});
            });
        }
        mAsyncNavMeshUpdater.post(agentBounds, cached, playerTile, mRecastMeshManager.getWorldspace(), tilesToPost);
        if (changedTiles != mChangedTiles.end())
            changedTiles->second.clear();
        Log(Debug::Debug) << "Cache update posted for agent=" << agentBounds <<
            " playerTile=" << lastPlayerTile->second <<
            " recastMeshManagerRevision=" << lastRevision;
    }

    void NavMeshManager::wait(Loading::Listener& listener, WaitConditionType waitConditionType)
    {
        mAsyncNavMeshUpdater.wait(listener, waitConditionType);
    }

    SharedNavMeshCacheItem NavMeshManager::getNavMesh(const AgentBounds& agentBounds) const
    {
        return getCached(agentBounds);
    }

    std::map<AgentBounds, SharedNavMeshCacheItem> NavMeshManager::getNavMeshes() const
    {
        return mCache;
    }

    void NavMeshManager::reportStats(unsigned int frameNumber, osg::Stats& stats) const
    {
        DetourNavigator::reportStats(mAsyncNavMeshUpdater.getStats(), frameNumber, stats);
    }

    RecastMeshTiles NavMeshManager::getRecastMeshTiles() const
    {
        std::vector<TilePosition> tiles;
        mRecastMeshManager.forEachTile(
            [&tiles] (const TilePosition& tile, const CachedRecastMeshManager&) { tiles.push_back(tile); });
        const std::string worldspace = mRecastMeshManager.getWorldspace();
        RecastMeshTiles result;
        for (const TilePosition& tile : tiles)
            if (auto mesh = mRecastMeshManager.getCachedMesh(worldspace, tile))
                result.emplace(tile, std::move(mesh));
        return result;
    }

    void NavMeshManager::addChangedTiles(const btCollisionShape& shape, const btTransform& transform,
            const ChangeType changeType)
    {
        const auto bounds = mRecastMeshManager.getBounds();
        getTilesPositions(makeTilesPositionsRange(shape, transform, bounds, mSettings.mRecast),
            [&] (const TilePosition& v) { addChangedTile(v, changeType); });
    }

    void NavMeshManager::addChangedTiles(const int cellSize, const btVector3& shift,
            const ChangeType changeType)
    {
        if (cellSize == std::numeric_limits<int>::max())
            return;

        getTilesPositions(makeTilesPositionsRange(cellSize, shift, mSettings.mRecast),
            [&] (const TilePosition& v) { addChangedTile(v, changeType); });
    }

    void NavMeshManager::addChangedTile(const TilePosition& tilePosition, const ChangeType changeType)
    {
        for (const auto& cached : mCache)
        {
            auto& tiles = mChangedTiles[cached.first];
            auto tile = tiles.find(tilePosition);
            if (tile == tiles.end())
                tiles.insert(std::make_pair(tilePosition, changeType));
            else
                tile->second = addChangeType(tile->second, changeType);
        }
    }

    SharedNavMeshCacheItem NavMeshManager::getCached(const AgentBounds& agentBounds) const
    {
        const auto cached = mCache.find(agentBounds);
        if (cached != mCache.end())
            return cached->second;
        return SharedNavMeshCacheItem();
    }
}
