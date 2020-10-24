#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATORIMPL_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATORIMPL_H

#include "navigator.hpp"
#include "navmeshmanager.hpp"

#include <set>

namespace DetourNavigator
{
    class NavigatorImpl final : public Navigator
    {
    public:
        /**
         * @brief Navigator constructor initializes all internal data. Constructed object is ready to build a scene.
         * @param settings allows to customize navigator work. Constructor is only place to set navigator settings.
         */
        explicit NavigatorImpl(Settings  settings);

        void addAgent(const osg::Vec3f& agentHalfExtents) override;

        void removeAgent(const osg::Vec3f& agentHalfExtents) override;

        bool addObject(ObjectId id, const btCollisionShape& shape, const btTransform& transform) override;

        bool addObject(ObjectId id, const ObjectShapes& shapes, const btTransform& transform) override;

        bool addObject(ObjectId id, const DoorShapes& shapes, const btTransform& transform) override;

        bool updateObject(ObjectId id, const btCollisionShape& shape, const btTransform& transform) override;

        bool updateObject(ObjectId id, const ObjectShapes& shapes, const btTransform& transform) override;

        bool updateObject(ObjectId id, const DoorShapes& shapes, const btTransform& transform) override;

        bool removeObject(ObjectId id) override;

        bool addWater(const osg::Vec2i& cellPosition, int cellSize, btScalar level,
            const btTransform& transform) override;

        bool removeWater(const osg::Vec2i& cellPosition) override;

        void addPathgrid(const ESM::Cell& cell, const ESM::Pathgrid& pathgrid) override;

        void removePathgrid(const ESM::Pathgrid& pathgrid) override;

        void update(const osg::Vec3f& playerPosition) override;

        void setUpdatesEnabled(bool enabled) override;

        void wait() override;

        SharedNavMeshCacheItem getNavMesh(const osg::Vec3f& agentHalfExtents) const override;

        std::map<osg::Vec3f, SharedNavMeshCacheItem> getNavMeshes() const override;

        const Settings& getSettings() const override;

        void reportStats(unsigned int frameNumber, osg::Stats& stats) const override;

        RecastMeshTiles getRecastMeshTiles() override;

    private:
        Settings mSettings;
        NavMeshManager mNavMeshManager;
        bool mUpdatesEnabled;
        std::map<osg::Vec3f, std::size_t> mAgents;
        std::unordered_map<ObjectId, ObjectId> mAvoidIds;
        std::unordered_map<ObjectId, ObjectId> mWaterIds;

        void updateAvoidShapeId(ObjectId id, ObjectId avoidId);
        void updateWaterShapeId(ObjectId id, ObjectId waterId);
        void updateId(ObjectId id, ObjectId waterId, std::unordered_map<ObjectId, ObjectId>& ids);
        void removeUnusedNavMeshes();
    };
}

#endif
