#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHMANAGER_H

#include "recastmeshbuilder.hpp"
#include "recastmeshobject.hpp"
#include "objectid.hpp"

#include <LinearMath/btTransform.h>

#include <osg/Vec2i>

#include <boost/optional.hpp>

#include <map>
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
        struct Water
        {
            int mCellSize;
            btTransform mTransform;
        };

        RecastMeshManager(const Settings& settings, const TileBounds& bounds);

        bool addObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform,
                       const AreaType areaType);

        bool updateObject(const ObjectId id, const btTransform& transform, const AreaType areaType);

        bool addWater(const osg::Vec2i& cellPosition, const int cellSize, const btTransform& transform);

        boost::optional<Water> removeWater(const osg::Vec2i& cellPosition);

        boost::optional<RemovedRecastMeshObject> removeObject(const ObjectId id);

        std::shared_ptr<RecastMesh> getMesh();

        bool isEmpty() const;

    private:
        bool mShouldRebuild;
        RecastMeshBuilder mMeshBuilder;
        std::unordered_map<ObjectId, RecastMeshObject> mObjects;
        std::map<osg::Vec2i, Water> mWater;

        void rebuild();
    };
}

#endif
