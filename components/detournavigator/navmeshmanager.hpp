#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHMANAGER_H

#include "asyncnavmeshupdater.hpp"
#include "cachedrecastmeshmanager.hpp"
#include "offmeshconnectionsmanager.hpp"
#include "sharednavmesh.hpp"
#include "recastmeshtiles.hpp"

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

#include <osg/Vec3f>

#include <map>
#include <memory>

class dtNavMesh;

namespace DetourNavigator
{
    class NavMeshManager
    {
    public:
        NavMeshManager(const Settings& settings);

        bool addObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform,
                       const AreaType areaType);

        bool updateObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform,
                          const AreaType areaType);

        bool removeObject(const ObjectId id);

        void addAgent(const osg::Vec3f& agentHalfExtents);

        bool addWater(const osg::Vec2i& cellPosition, const int cellSize, const btTransform& transform);

        bool removeWater(const osg::Vec2i& cellPosition);

        bool reset(const osg::Vec3f& agentHalfExtents);

        void addOffMeshConnection(const ObjectId id, const osg::Vec3f& start, const osg::Vec3f& end);

        void removeOffMeshConnection(const ObjectId id);

        void update(osg::Vec3f playerPosition, const osg::Vec3f& agentHalfExtents);

        void wait();

        SharedNavMeshCacheItem getNavMesh(const osg::Vec3f& agentHalfExtents) const;

        std::map<osg::Vec3f, SharedNavMeshCacheItem> getNavMeshes() const;

        void reportStats(unsigned int frameNumber, osg::Stats& stats) const;

        RecastMeshTiles getRecastMeshTiles();

    private:
        const Settings& mSettings;
        TileCachedRecastMeshManager mRecastMeshManager;
        OffMeshConnectionsManager mOffMeshConnectionsManager;
        AsyncNavMeshUpdater mAsyncNavMeshUpdater;
        std::map<osg::Vec3f, SharedNavMeshCacheItem> mCache;
        std::map<osg::Vec3f, std::map<TilePosition, ChangeType>> mChangedTiles;
        std::size_t mGenerationCounter = 0;
        std::map<osg::Vec3f, TilePosition> mPlayerTile;
        std::map<osg::Vec3f, std::size_t> mLastRecastMeshManagerRevision;

        void addChangedTiles(const btCollisionShape& shape, const btTransform& transform, const ChangeType changeType);

        void addChangedTiles(const int cellSize, const btTransform& transform, const ChangeType changeType);

        void addChangedTile(const TilePosition& tilePosition, const ChangeType changeType);

        SharedNavMeshCacheItem getCached(const osg::Vec3f& agentHalfExtents) const;
    };
}

#endif
