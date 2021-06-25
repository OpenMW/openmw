#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHMANAGER_H

#include "recastmeshbuilder.hpp"
#include "oscillatingrecastmeshobject.hpp"
#include "objectid.hpp"
#include "version.hpp"

#include <LinearMath/btTransform.h>

#include <osg/Vec2i>

#include <list>
#include <map>
#include <optional>

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

        std::optional<Water> removeWater(const osg::Vec2i& cellPosition);

        std::optional<RemovedRecastMeshObject> removeObject(const ObjectId id);

        std::shared_ptr<RecastMesh> getMesh();

        bool isEmpty() const;

        void reportNavMeshChange(const Version& recastMeshVersion, const Version& navMeshVersion);

        Version getVersion() const;

    private:
        struct Report
        {
            std::size_t mRevision;
            Version mNavMeshVersion;
        };

        std::size_t mRevision = 0;
        std::size_t mGeneration;
        RecastMeshBuilder mMeshBuilder;
        TileBounds mTileBounds;
        std::list<OscillatingRecastMeshObject> mObjectsOrder;
        std::map<ObjectId, std::list<OscillatingRecastMeshObject>::iterator> mObjects;
        std::list<Water> mWaterOrder;
        std::map<osg::Vec2i, std::list<Water>::iterator> mWater;
        std::optional<Report> mLastNavMeshReportedChange;
        std::optional<Report> mLastNavMeshReport;

        void rebuild();
    };
}

#endif
