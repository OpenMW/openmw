#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATORIMPL_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATORIMPL_H

#include "navigator.hpp"
#include "navmeshmanager.hpp"

namespace DetourNavigator
{
    class NavigatorImpl final : public Navigator
    {
    public:
        /**
         * @brief Navigator constructor initializes all internal data. Constructed object is ready to build a scene.
         * @param settings allows to customize navigator work. Constructor is only place to set navigator settings.
         */
        NavigatorImpl(const Settings& settings);

        void addAgent(const osg::Vec3f& agentHalfExtents) override;

        void removeAgent(const osg::Vec3f& agentHalfExtents) override;

        bool addObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform) override;

        bool addObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform) override;

        bool addObject(const ObjectId id, const DoorShapes& shapes, const btTransform& transform) override;

        bool updateObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform) override;

        bool updateObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform) override;

        bool updateObject(const ObjectId id, const DoorShapes& shapes, const btTransform& transform) override;

        bool removeObject(const ObjectId id) override;

        bool addWater(const osg::Vec2i& cellPosition, const int cellSize, const btScalar level,
            const btTransform& transform) override;

        bool removeWater(const osg::Vec2i& cellPosition) override;

        void update(const osg::Vec3f& playerPosition) override;

        void wait() override;

        SharedNavMeshCacheItem getNavMesh(const osg::Vec3f& agentHalfExtents) const override;

        std::map<osg::Vec3f, SharedNavMeshCacheItem> getNavMeshes() const override;

        const Settings& getSettings() const override;

        void reportStats(unsigned int frameNumber, osg::Stats& stats) const override;

        RecastMeshTiles getRecastMeshTiles() override;

    private:
        Settings mSettings;
        NavMeshManager mNavMeshManager;
        std::map<osg::Vec3f, std::size_t> mAgents;
        std::unordered_map<ObjectId, ObjectId> mAvoidIds;
        std::unordered_map<ObjectId, ObjectId> mWaterIds;

        void updateAvoidShapeId(const ObjectId id, const ObjectId avoidId);
        void updateWaterShapeId(const ObjectId id, const ObjectId waterId);
        void updateId(const ObjectId id, const ObjectId waterId, std::unordered_map<ObjectId, ObjectId>& ids);
        void removeUnusedNavMeshes();
    };
}

#endif
