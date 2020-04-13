#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATORSTUB_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATORSTUB_H

#include "navigator.hpp"

namespace DetourNavigator
{
    class NavigatorStub final : public Navigator
    {
    public:
        NavigatorStub() = default;

        void addAgent(const osg::Vec3f& /*agentHalfExtents*/) override {}

        void removeAgent(const osg::Vec3f& /*agentHalfExtents*/) override {}

        bool addObject(const ObjectId /*id*/, const btCollisionShape& /*shape*/, const btTransform& /*transform*/) override
        {
            return false;
        }

        bool addObject(const ObjectId /*id*/, const ObjectShapes& /*shapes*/, const btTransform& /*transform*/) override
        {
            return false;
        }

        bool addObject(const ObjectId /*id*/, const DoorShapes& /*shapes*/, const btTransform& /*transform*/) override
        {
            return false;
        }

        bool updateObject(const ObjectId /*id*/, const btCollisionShape& /*shape*/, const btTransform& /*transform*/) override
        {
            return false;
        }

        bool updateObject(const ObjectId /*id*/, const ObjectShapes& /*shapes*/, const btTransform& /*transform*/) override
        {
            return false;
        }

        bool updateObject(const ObjectId /*id*/, const DoorShapes& /*shapes*/, const btTransform& /*transform*/) override
        {
            return false;
        }

        bool removeObject(const ObjectId /*id*/) override
        {
            return false;
        }

        bool addWater(const osg::Vec2i& /*cellPosition*/, const int /*cellSize*/, const btScalar /*level*/,
            const btTransform& /*transform*/) override
        {
            return false;
        }

        bool removeWater(const osg::Vec2i& /*cellPosition*/) override
        {
            return false;
        }

        void update(const osg::Vec3f& /*playerPosition*/) override {}

        void wait() override {}

        SharedNavMeshCacheItem getNavMesh(const osg::Vec3f& /*agentHalfExtents*/) const override
        {
            return mEmptyNavMeshCacheItem;
        }

        std::map<osg::Vec3f, SharedNavMeshCacheItem> getNavMeshes() const override
        {
            return std::map<osg::Vec3f, SharedNavMeshCacheItem>();
        }

        const Settings& getSettings() const override
        {
            return mDefaultSettings;
        }

        void reportStats(unsigned int /*frameNumber*/, osg::Stats& /*stats*/) const override {}

        RecastMeshTiles getRecastMeshTiles() override
        {
            return {};
        }

    private:
        Settings mDefaultSettings {};
        SharedNavMeshCacheItem mEmptyNavMeshCacheItem;
    };
}

#endif
