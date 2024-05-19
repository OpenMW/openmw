#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATORSTUB_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATORSTUB_H

#include "navigator.hpp"
#include "settings.hpp"
#include "stats.hpp"
#include "updateguard.hpp"

namespace Loading
{
    class Listener;
}

namespace DetourNavigator
{
    class NavigatorStub final : public Navigator
    {
    public:
        NavigatorStub() = default;

        ScopedUpdateGuard makeUpdateGuard() override { return nullptr; }

        bool addAgent(const AgentBounds& /*agentBounds*/) override { return true; }

        void removeAgent(const AgentBounds& /*agentBounds*/) override {}

        void updateBounds(
            ESM::RefId /*worldspace*/, const osg::Vec3f& /*playerPosition*/, const UpdateGuard* /*guard*/) override
        {
        }

        void addObject(const ObjectId /*id*/, const ObjectShapes& /*shapes*/, const btTransform& /*transform*/,
            const UpdateGuard* /*guard*/) override
        {
        }

        void addObject(const ObjectId /*id*/, const DoorShapes& /*shapes*/, const btTransform& /*transform*/,
            const UpdateGuard* /*guard*/) override
        {
        }

        void updateObject(const ObjectId /*id*/, const ObjectShapes& /*shapes*/, const btTransform& /*transform*/,
            const UpdateGuard* /*guard*/) override
        {
        }

        void updateObject(const ObjectId /*id*/, const DoorShapes& /*shapes*/, const btTransform& /*transform*/,
            const UpdateGuard* /*guard*/) override
        {
        }

        void removeObject(const ObjectId /*id*/, const UpdateGuard* /*guard*/) override {}

        void addWater(const osg::Vec2i& /*cellPosition*/, int /*cellSize*/, float /*level*/,
            const UpdateGuard* /*guard*/) override
        {
        }

        void removeWater(const osg::Vec2i& /*cellPosition*/, const UpdateGuard* /*guard*/) override {}

        void addHeightfield(const osg::Vec2i& /*cellPosition*/, int /*cellSize*/, const HeightfieldShape& /*height*/,
            const UpdateGuard* /*guard*/) override
        {
        }

        void removeHeightfield(const osg::Vec2i& /*cellPosition*/, const UpdateGuard* /*guard*/) override {}

        void addPathgrid(const ESM::Cell& /*cell*/, const ESM::Pathgrid& /*pathgrid*/) override {}

        void removePathgrid(const ESM::Pathgrid& /*pathgrid*/) override {}

        void update(const osg::Vec3f& /*playerPosition*/, const UpdateGuard* /*guard*/) override {}

        void wait(WaitConditionType /*waitConditionType*/, Loading::Listener* /*listener*/) override {}

        SharedNavMeshCacheItem getNavMesh(const AgentBounds& /*agentBounds*/) const override
        {
            return mEmptyNavMeshCacheItem;
        }

        std::map<AgentBounds, SharedNavMeshCacheItem> getNavMeshes() const override { return {}; }

        const Settings& getSettings() const override { return mDefaultSettings; }

        Stats getStats() const override { return Stats{}; }

        RecastMeshTiles getRecastMeshTiles() const override { return {}; }

        float getMaxNavmeshAreaRealRadius() const override { return std::numeric_limits<float>::max(); }

    private:
        Settings mDefaultSettings{};
        SharedNavMeshCacheItem mEmptyNavMeshCacheItem;
    };
}

#endif
