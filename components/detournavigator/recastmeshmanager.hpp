#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHMANAGER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHMANAGER_H

#include "oscillatingrecastmeshobject.hpp"
#include "objectid.hpp"
#include "version.hpp"
#include "recastmesh.hpp"
#include "heightfieldshape.hpp"

#include <LinearMath/btTransform.h>

#include <osg/Vec2i>

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

    struct SizedHeightfieldShape
    {
        int mCellSize;
        HeightfieldShape mShape;
    };

    class RecastMeshManager
    {
    public:
        explicit RecastMeshManager(const TileBounds& bounds, std::size_t generation);

        bool addObject(const ObjectId id, const CollisionShape& shape, const btTransform& transform,
                       const AreaType areaType);

        bool updateObject(const ObjectId id, const btTransform& transform, const AreaType areaType);

        std::optional<RemovedRecastMeshObject> removeObject(const ObjectId id);

        bool addWater(const osg::Vec2i& cellPosition, int cellSize, float level);

        std::optional<Water> removeWater(const osg::Vec2i& cellPosition);

        bool addHeightfield(const osg::Vec2i& cellPosition, int cellSize, const HeightfieldShape& shape);

        std::optional<SizedHeightfieldShape> removeHeightfield(const osg::Vec2i& cellPosition);

        std::shared_ptr<RecastMesh> getMesh() const;

        bool isEmpty() const;

        void reportNavMeshChange(const Version& recastMeshVersion, const Version& navMeshVersion);

        Version getVersion() const;

    private:
        struct Report
        {
            std::size_t mRevision;
            Version mNavMeshVersion;
        };

        const std::size_t mGeneration;
        const TileBounds mTileBounds;
        mutable std::mutex mMutex;
        std::size_t mRevision = 0;
        std::map<ObjectId, OscillatingRecastMeshObject> mObjects;
        std::map<osg::Vec2i, Water> mWater;
        std::map<osg::Vec2i, SizedHeightfieldShape> mHeightfields;
        std::optional<Report> mLastNavMeshReportedChange;
        std::optional<Report> mLastNavMeshReport;
    };
}

#endif
