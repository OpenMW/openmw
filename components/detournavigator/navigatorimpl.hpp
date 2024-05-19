#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATORIMPL_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATORIMPL_H

#include "navigator.hpp"
#include "navmeshmanager.hpp"
#include "updateguard.hpp"

#include <map>
#include <memory>
#include <optional>
#include <unordered_map>

namespace DetourNavigator
{
    class NavigatorImpl final : public Navigator
    {
    public:
        /**
         * @brief Navigator constructor initializes all internal data. Constructed object is ready to build a scene.
         * @param settings allows to customize navigator work. Constructor is only place to set navigator settings.
         */
        explicit NavigatorImpl(const Settings& settings, std::unique_ptr<NavMeshDb>&& db);

        ScopedUpdateGuard makeUpdateGuard() override { return mNavMeshManager.makeUpdateGuard(); }

        bool addAgent(const AgentBounds& agentBounds) override;

        void removeAgent(const AgentBounds& agentBounds) override;

        void updateBounds(ESM::RefId worldspace, const osg::Vec3f& playerPosition, const UpdateGuard* guard) override;

        void addObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform,
            const UpdateGuard* guard) override;

        void addObject(const ObjectId id, const DoorShapes& shapes, const btTransform& transform,
            const UpdateGuard* guard) override;

        void updateObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform,
            const UpdateGuard* guard) override;

        void updateObject(const ObjectId id, const DoorShapes& shapes, const btTransform& transform,
            const UpdateGuard* guard) override;

        void removeObject(const ObjectId id, const UpdateGuard* guard) override;

        void addWater(const osg::Vec2i& cellPosition, int cellSize, float level, const UpdateGuard* guard) override;

        void removeWater(const osg::Vec2i& cellPosition, const UpdateGuard* guard) override;

        void addHeightfield(const osg::Vec2i& cellPosition, int cellSize, const HeightfieldShape& shape,
            const UpdateGuard* guard) override;

        void removeHeightfield(const osg::Vec2i& cellPosition, const UpdateGuard* guard) override;

        void addPathgrid(const ESM::Cell& cell, const ESM::Pathgrid& pathgrid) override;

        void removePathgrid(const ESM::Pathgrid& pathgrid) override;

        void update(const osg::Vec3f& playerPosition, const UpdateGuard* guard) override;

        void wait(WaitConditionType waitConditionType, Loading::Listener* listener) override;

        SharedNavMeshCacheItem getNavMesh(const AgentBounds& agentBounds) const override;

        std::map<AgentBounds, SharedNavMeshCacheItem> getNavMeshes() const override;

        const Settings& getSettings() const override;

        Stats getStats() const override;

        RecastMeshTiles getRecastMeshTiles() const override;

        float getMaxNavmeshAreaRealRadius() const override;

    private:
        Settings mSettings;
        NavMeshManager mNavMeshManager;
        std::optional<TilePosition> mLastPlayerPosition;
        std::map<AgentBounds, std::size_t> mAgents;
        std::unordered_map<ObjectId, ObjectId> mAvoidIds;
        std::unordered_map<ObjectId, ObjectId> mWaterIds;

        inline bool addObjectImpl(
            const ObjectId id, const ObjectShapes& shapes, const btTransform& transform, const UpdateGuard* guard);

        inline void updateAvoidShapeId(const ObjectId id, const ObjectId avoidId, const UpdateGuard* guard);

        inline void updateId(const ObjectId id, const ObjectId waterId, std::unordered_map<ObjectId, ObjectId>& ids,
            const UpdateGuard* guard);

        inline void removeUnusedNavMeshes();

        friend class UpdateGuard;
    };
}

#endif
