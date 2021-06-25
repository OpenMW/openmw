#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_CACHEDRECASTMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_CACHEDRECASTMESHMANAGER_H

#include "recastmeshmanager.hpp"
#include "version.hpp"

namespace DetourNavigator
{
    class CachedRecastMeshManager
    {
    public:
        CachedRecastMeshManager(const Settings& settings, const TileBounds& bounds, std::size_t generation);

        bool addObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform,
                       const AreaType areaType);

        bool updateObject(const ObjectId id, const btTransform& transform, const AreaType areaType);

        bool addWater(const osg::Vec2i& cellPosition, const int cellSize, const btTransform& transform);

        std::optional<RecastMeshManager::Water> removeWater(const osg::Vec2i& cellPosition);

        std::optional<RemovedRecastMeshObject> removeObject(const ObjectId id);

        std::shared_ptr<RecastMesh> getMesh();

        bool isEmpty() const;

        void reportNavMeshChange(const Version& recastMeshVersion, const Version& navMeshVersion);

        Version getVersion() const;

    private:
        RecastMeshManager mImpl;
        std::shared_ptr<RecastMesh> mCached;
    };
}

#endif
