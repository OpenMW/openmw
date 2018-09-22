#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATOR_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATOR_H

#include "findsmoothpath.hpp"
#include "flags.hpp"
#include "navmeshmanager.hpp"
#include "settings.hpp"
#include "settingsutils.hpp"

namespace DetourNavigator
{
    struct ObjectShapes
    {
        const btCollisionShape& mShape;
        const btCollisionShape* mAvoid;

        ObjectShapes(const btCollisionShape& shape, const btCollisionShape* avoid)
            : mShape(shape), mAvoid(avoid)
        {}
    };

    struct DoorShapes : ObjectShapes
    {
        osg::Vec3f mConnectionStart;
        osg::Vec3f mConnectionEnd;

        DoorShapes(const btCollisionShape& shape, const btCollisionShape* avoid,
                   const osg::Vec3f& connectionStart,const osg::Vec3f& connectionEnd)
            : ObjectShapes(shape, avoid)
            , mConnectionStart(connectionStart)
            , mConnectionEnd(connectionEnd)
        {}
    };

    class Navigator
    {
    public:
        Navigator(const Settings& settings);

        void addAgent(const osg::Vec3f& agentHalfExtents);

        void removeAgent(const osg::Vec3f& agentHalfExtents);

        bool addObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform);

        bool addObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform);

        bool addObject(const ObjectId id, const DoorShapes& shapes, const btTransform& transform);

        bool updateObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform);

        bool updateObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform);

        bool updateObject(const ObjectId id, const DoorShapes& shapes, const btTransform& transform);

        bool removeObject(const ObjectId id);

        bool addWater(const osg::Vec2i& cellPosition, const int cellSize, const btScalar level,
            const btTransform& transform);

        bool removeWater(const osg::Vec2i& cellPosition);

        void update(const osg::Vec3f& playerPosition);

        void wait();

        template <class OutputIterator>
        OutputIterator findPath(const osg::Vec3f& agentHalfExtents, const osg::Vec3f& start,
            const osg::Vec3f& end, const Flags includeFlags, OutputIterator out) const
        {
            const auto navMesh = mNavMeshManager.getNavMesh(agentHalfExtents);
            return findSmoothPath(*navMesh.lock(), toNavMeshCoordinates(mSettings, agentHalfExtents),
                toNavMeshCoordinates(mSettings, start), toNavMeshCoordinates(mSettings, end), includeFlags,
                mSettings, out);
        }

        std::map<osg::Vec3f, std::shared_ptr<NavMeshCacheItem>> getNavMeshes() const;

        const Settings& getSettings() const;

    private:
        Settings mSettings;
        NavMeshManager mNavMeshManager;
        std::map<osg::Vec3f, std::size_t> mAgents;
        std::unordered_map<ObjectId, ObjectId> mAvoidIds;
        std::unordered_map<ObjectId, ObjectId> mWaterIds;

        void updateAvoidShapeId(const ObjectId id, const ObjectId avoidId);
        void updateWaterShapeId(const ObjectId id, const ObjectId waterId);
        void updateId(const ObjectId id, const ObjectId waterId, std::unordered_map<ObjectId, ObjectId>& ids);
    };
}

#endif
