#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHMANAGER_H

#include "oscillatingrecastmeshobject.hpp"
#include "objectid.hpp"
#include "version.hpp"
#include "recastmesh.hpp"

#include <LinearMath/btTransform.h>

#include <osg/Vec2i>

#include <map>
#include <optional>
#include <memory>

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
        RecastMeshManager(const Settings& settings, const TileBounds& bounds, std::size_t generation);

        bool addObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform,
                       const AreaType areaType);

        bool updateObject(const ObjectId id, const btTransform& transform, const AreaType areaType);

        bool addWater(const osg::Vec2i& cellPosition, const int cellSize, const osg::Vec3f& shift);

        std::optional<Cell> removeWater(const osg::Vec2i& cellPosition);

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

        const Settings& mSettings;
        std::size_t mRevision = 0;
        std::size_t mGeneration;
        TileBounds mTileBounds;
        std::map<ObjectId, OscillatingRecastMeshObject> mObjects;
        std::map<osg::Vec2i, Cell> mWater;
        std::optional<Report> mLastNavMeshReportedChange;
        std::optional<Report> mLastNavMeshReport;
    };
}

#endif
