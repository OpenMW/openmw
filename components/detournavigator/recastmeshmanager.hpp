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
#include <list>

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
            int mCellSize = 0;
            btTransform mTransform;
        };

        RecastMeshManager(const Settings& settings, const TileBounds& bounds, std::size_t generation);

        bool addObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform,
                       const AreaType areaType);

        bool updateObject(const ObjectId id, const btTransform& transform, const AreaType areaType);

        bool addWater(const osg::Vec2i& cellPosition, const int cellSize, const btTransform& transform);

        boost::optional<Water> removeWater(const osg::Vec2i& cellPosition);

        boost::optional<RemovedRecastMeshObject> removeObject(const ObjectId id);

        std::shared_ptr<RecastMesh> getMesh();

        bool isEmpty() const;

    private:
        std::size_t mRevision = 0;
        std::size_t mLastBuildRevision = 0;
        std::size_t mGeneration;
        RecastMeshBuilder mMeshBuilder;
        std::list<RecastMeshObject> mObjectsOrder;
        std::unordered_map<ObjectId, std::list<RecastMeshObject>::iterator> mObjects;
        std::list<Water> mWaterOrder;
        std::map<osg::Vec2i, std::list<Water>::iterator> mWater;

        void rebuild();
    };
}

#endif
