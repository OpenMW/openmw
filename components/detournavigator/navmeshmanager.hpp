#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHMANAGER_H

#include "asyncnavmeshupdater.hpp"
#include "cachedrecastmeshmanager.hpp"
#include "offmeshconnectionsmanager.hpp"
#include "recastmeshtiles.hpp"
#include "waitconditiontype.hpp"
#include "heightfieldshape.hpp"
#include "agentbounds.hpp"

#include <osg/Vec3f>

#include <map>
#include <memory>

class dtNavMesh;

namespace DetourNavigator
{
    class NavMeshManager
    {
    public:
        explicit NavMeshManager(const Settings& settings, std::unique_ptr<NavMeshDb>&& db);

        void setWorldspace(std::string_view worldspace);

        void updateBounds(const osg::Vec3f& playerPosition);

        bool addObject(const ObjectId id, const CollisionShape& shape, const btTransform& transform,
                       const AreaType areaType);

        bool updateObject(const ObjectId id, const CollisionShape& shape, const btTransform& transform,
                          const AreaType areaType);

        bool removeObject(const ObjectId id);

        void addAgent(const AgentBounds& agentBounds);

        bool addWater(const osg::Vec2i& cellPosition, int cellSize, float level);

        bool removeWater(const osg::Vec2i& cellPosition);

        bool addHeightfield(const osg::Vec2i& cellPosition, int cellSize, const HeightfieldShape& shape);

        bool removeHeightfield(const osg::Vec2i& cellPosition);

        bool reset(const AgentBounds& agentBounds);

        void addOffMeshConnection(const ObjectId id, const osg::Vec3f& start, const osg::Vec3f& end, const AreaType areaType);

        void removeOffMeshConnections(const ObjectId id);

        void update(const osg::Vec3f& playerPosition, const AgentBounds& agentBounds);

        void wait(Loading::Listener& listener, WaitConditionType waitConditionType);

        SharedNavMeshCacheItem getNavMesh(const AgentBounds& agentBounds) const;

        std::map<AgentBounds, SharedNavMeshCacheItem> getNavMeshes() const;

        void reportStats(unsigned int frameNumber, osg::Stats& stats) const;

        RecastMeshTiles getRecastMeshTiles() const;

    private:
        const Settings& mSettings;
        std::string mWorldspace;
        TileCachedRecastMeshManager mRecastMeshManager;
        OffMeshConnectionsManager mOffMeshConnectionsManager;
        AsyncNavMeshUpdater mAsyncNavMeshUpdater;
        std::map<AgentBounds, SharedNavMeshCacheItem> mCache;
        std::map<AgentBounds, std::map<TilePosition, ChangeType>> mChangedTiles;
        std::size_t mGenerationCounter = 0;
        std::map<AgentBounds, TilePosition> mPlayerTile;
        std::map<AgentBounds, std::size_t> mLastRecastMeshManagerRevision;

        void addChangedTiles(const btCollisionShape& shape, const btTransform& transform, const ChangeType changeType);

        void addChangedTiles(const int cellSize, const btVector3& shift, const ChangeType changeType);

        void addChangedTile(const TilePosition& tilePosition, const ChangeType changeType);

        SharedNavMeshCacheItem getCached(const AgentBounds& agentBounds) const;
    };
}

#endif
