#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_CACHEDRECASTMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_CACHEDRECASTMESHMANAGER_H

#include "recastmeshmanager.hpp"
#include "version.hpp"
#include "heightfieldshape.hpp"

#include <components/misc/guarded.hpp>

namespace DetourNavigator
{
    class CachedRecastMeshManager
    {
    public:
        CachedRecastMeshManager(const Settings& settings, const TileBounds& bounds, std::size_t generation);

        bool addObject(const ObjectId id, const CollisionShape& shape, const btTransform& transform,
                       const AreaType areaType);

        bool updateObject(const ObjectId id, const btTransform& transform, const AreaType areaType);

        std::optional<RemovedRecastMeshObject> removeObject(const ObjectId id);

        bool addWater(const osg::Vec2i& cellPosition, const int cellSize, const osg::Vec3f& shift);

        std::optional<Cell> removeWater(const osg::Vec2i& cellPosition);

        bool addHeightfield(const osg::Vec2i& cellPosition, int cellSize, const osg::Vec3f& shift,
            const HeightfieldShape& shape);

        std::optional<Cell> removeHeightfield(const osg::Vec2i& cellPosition);

        std::shared_ptr<RecastMesh> getMesh();

        bool isEmpty() const;

        void reportNavMeshChange(const Version& recastMeshVersion, const Version& navMeshVersion);

        Version getVersion() const;

    private:
        RecastMeshManager mImpl;
        Misc::ScopeGuarded<std::shared_ptr<RecastMesh>> mCached;
    };
}

#endif
