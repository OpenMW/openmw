#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHMANAGER_H

#include "oscillatingrecastmeshobject.hpp"
#include "objectid.hpp"
#include "version.hpp"

#include <LinearMath/btTransform.h>

#include <osg/Vec2i>

#include <list>
#include <map>
#include <optional>
#include <memory>
#include <mutex>

class btCollisionShape;

namespace DetourNavigator
{
    struct Settings;
    class RecastMesh;

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

        bool addObject(const ObjectId id, const CollisionShape& shape, const btTransform& transform,
                       const AreaType areaType);

        bool updateObject(const ObjectId id, const btTransform& transform, const AreaType areaType);

        bool addWater(const osg::Vec2i& cellPosition, const int cellSize, const btTransform& transform);

        std::optional<Water> removeWater(const osg::Vec2i& cellPosition);

        std::optional<RemovedRecastMeshObject> removeObject(const ObjectId id);

        std::shared_ptr<RecastMesh> getMesh();

        bool isEmpty() const;

        void reportNavMeshChange(Version recastMeshVersion, Version navMeshVersion);

        Version getVersion() const;

    private:
        struct Report
        {
            std::size_t mRevision;
            Version mNavMeshVersion;
        };

        const Settings& mSettings;
        const std::size_t mGeneration;
        const TileBounds mTileBounds;
        mutable std::mutex mMutex;
        std::size_t mRevision = 0;
        std::list<OscillatingRecastMeshObject> mObjectsOrder;
        std::map<ObjectId, std::list<OscillatingRecastMeshObject>::iterator> mObjects;
        std::list<Water> mWaterOrder;
        std::map<osg::Vec2i, std::list<Water>::iterator> mWater;
        std::optional<Report> mLastNavMeshReportedChange;
        std::optional<Report> mLastNavMeshReport;
    };
}

#endif
