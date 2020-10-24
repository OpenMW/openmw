#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_CACHEDRECASTMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_CACHEDRECASTMESHMANAGER_H

#include "recastmeshmanager.hpp"

namespace DetourNavigator
{
    class CachedRecastMeshManager
    {
    public:
        CachedRecastMeshManager(const Settings& settings, const TileBounds& bounds, std::size_t generation);

        bool addObject(ObjectId id, const btCollisionShape& shape, const btTransform& transform,
                       AreaType areaType);

        bool updateObject(ObjectId id, const btTransform& transform, AreaType areaType);

        bool addWater(const osg::Vec2i& cellPosition, int cellSize, const btTransform& transform);

        std::optional<RecastMeshManager::Water> removeWater(const osg::Vec2i& cellPosition);

        std::optional<RemovedRecastMeshObject> removeObject(ObjectId id);

        std::shared_ptr<RecastMesh> getMesh();

        bool isEmpty() const;

    private:
        RecastMeshManager mImpl;
        std::shared_ptr<RecastMesh> mCached;
    };
}

#endif
