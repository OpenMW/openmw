#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHMANAGER_H

#include "agentbounds.hpp"
#include "asyncnavmeshupdater.hpp"
#include "heightfieldshape.hpp"
#include "offmeshconnectionsmanager.hpp"
#include "recastmeshtiles.hpp"
#include "waitconditiontype.hpp"

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

        ScopedUpdateGuard makeUpdateGuard() { return mRecastMeshManager.makeUpdateGuard(); }

        void updateBounds(ESM::RefId worldspace, const osg::Vec3f& playerPosition, const UpdateGuard* guard);

        bool addObject(const ObjectId id, const CollisionShape& shape, const btTransform& transform,
            const AreaType areaType, const UpdateGuard* guard);

        bool updateObject(ObjectId id, const btTransform& transform, AreaType areaType, const UpdateGuard* guard);

        void removeObject(const ObjectId id, const UpdateGuard* guard);

        void addAgent(const AgentBounds& agentBounds);

        void addWater(const osg::Vec2i& cellPosition, int cellSize, float level, const UpdateGuard* guard);

        void removeWater(const osg::Vec2i& cellPosition, const UpdateGuard* guard);

        void addHeightfield(
            const osg::Vec2i& cellPosition, int cellSize, const HeightfieldShape& shape, const UpdateGuard* guard);

        void removeHeightfield(const osg::Vec2i& cellPosition, const UpdateGuard* guard);

        bool reset(const AgentBounds& agentBounds);

        void addOffMeshConnection(
            const ObjectId id, const osg::Vec3f& start, const osg::Vec3f& end, const AreaType areaType);

        void removeOffMeshConnections(const ObjectId id);

        void update(const osg::Vec3f& playerPosition, const UpdateGuard* guard);

        void wait(WaitConditionType waitConditionType, Loading::Listener* listener);

        SharedNavMeshCacheItem getNavMesh(const AgentBounds& agentBounds) const;

        std::map<AgentBounds, SharedNavMeshCacheItem> getNavMeshes() const;

        Stats getStats() const;

        RecastMeshTiles getRecastMeshTiles() const;

    private:
        const Settings& mSettings;
        ESM::RefId mWorldspace;
        TileCachedRecastMeshManager mRecastMeshManager;
        OffMeshConnectionsManager mOffMeshConnectionsManager;
        AsyncNavMeshUpdater mAsyncNavMeshUpdater;
        std::map<AgentBounds, SharedNavMeshCacheItem> mCache;
        std::size_t mGenerationCounter = 0;
        std::optional<TilePosition> mPlayerTile;
        std::size_t mLastRecastMeshManagerRevision = 0;

        inline SharedNavMeshCacheItem getCached(const AgentBounds& agentBounds) const;

        inline void update(const AgentBounds& agentBounds, const TilePosition& playerTile,
            const TilesPositionsRange& range, const SharedNavMeshCacheItem& cached,
            const std::map<osg::Vec2i, ChangeType>& changedTiles);
    };
}

#endif
