#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVMESHMANAGER_H

#include "asyncnavmeshupdater.hpp"
#include "cachedrecastmeshmanager.hpp"
#include "offmeshconnectionsmanager.hpp"
#include "sharednavmesh.hpp"

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

        bool addObject(std::size_t id, const btCollisionShape& shape, const btTransform& transform,
                       const AreaType areaType);

        bool updateObject(std::size_t id, const btCollisionShape& shape, const btTransform& transform,
                          const AreaType areaType);

        bool removeObject(std::size_t id);

        void addAgent(const osg::Vec3f& agentHalfExtents);

        bool addWater(const osg::Vec2i& cellPosition, const int cellSize, const btTransform& transform);

        bool removeWater(const osg::Vec2i& cellPosition);

        void reset(const osg::Vec3f& agentHalfExtents);

        void addOffMeshConnection(std::size_t id, const osg::Vec3f& start, const osg::Vec3f& end);

        void removeOffMeshConnection(std::size_t id);

        void update(osg::Vec3f playerPosition, const osg::Vec3f& agentHalfExtents);

        void wait();

        SharedNavMesh getNavMesh(const osg::Vec3f& agentHalfExtents) const;

        std::map<osg::Vec3f, std::shared_ptr<NavMeshCacheItem>> getNavMeshes() const;

    private:
        const Settings& mSettings;
        TileCachedRecastMeshManager mRecastMeshManager;
        OffMeshConnectionsManager mOffMeshConnectionsManager;
        std::map<osg::Vec3f, std::shared_ptr<NavMeshCacheItem>> mCache;
        std::map<osg::Vec3f, std::map<TilePosition, ChangeType>> mChangedTiles;
        AsyncNavMeshUpdater mAsyncNavMeshUpdater;
        std::size_t mGenerationCounter = 0;
        std::map<osg::Vec3f, TilePosition> mPlayerTile;
        std::map<osg::Vec3f, std::size_t> mLastRecastMeshManagerRevision;

        void addChangedTiles(const btCollisionShape& shape, const btTransform& transform, const ChangeType changeType);

        void addChangedTiles(const int cellSize, const btTransform& transform, const ChangeType changeType);

        void addChangedTile(const TilePosition& tilePosition, const ChangeType changeType);

        const std::shared_ptr<NavMeshCacheItem>& getCached(const osg::Vec3f& agentHalfExtents) const;
    };
}

#endif
