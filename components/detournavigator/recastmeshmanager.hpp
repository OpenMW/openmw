#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHMANAGER_H

#include "recastmeshbuilder.hpp"
#include "recastmeshobject.hpp"

#include <LinearMath/btTransform.h>

#include <boost/optional.hpp>

#include <unordered_map>

class btCollisionShape;

namespace DetourNavigator
{
    struct RemovedRecastMeshObject
    {
        std::reference_wrapper<const btCollisionShape> mShape;
        btTransform mTransform;
    };

    class RecastMeshManager
    {
    public:
        RecastMeshManager(const Settings& settings, const TileBounds& bounds);

        bool addObject(std::size_t id, const btCollisionShape& shape, const btTransform& transform,
                       const AreaType areaType);

        bool updateObject(std::size_t id, const btTransform& transform, const AreaType areaType);

        boost::optional<RemovedRecastMeshObject> removeObject(std::size_t id);

        std::shared_ptr<RecastMesh> getMesh();

        bool isEmpty() const;

    private:
        bool mShouldRebuild;
        RecastMeshBuilder mMeshBuilder;
        std::unordered_map<std::size_t, RecastMeshObject> mObjects;

        void rebuild();
    };
}

#endif
