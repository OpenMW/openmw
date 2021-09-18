#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATORIMPL_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATORIMPL_H

#include "navigator.hpp"
#include "navmeshmanager.hpp"

#include <set>
#include <memory>

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

        void addAgent(const AgentBounds& agentBounds) override;

        void removeAgent(const AgentBounds& agentBounds) override;

        void setWorldspace(std::string_view worldspace) override;

        void updateBounds(const osg::Vec3f& playerPosition) override;

        void addObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform) override;

        void addObject(const ObjectId id, const DoorShapes& shapes, const btTransform& transform) override;

        void updateObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform) override;

        void updateObject(const ObjectId id, const DoorShapes& shapes, const btTransform& transform) override;

        void removeObject(const ObjectId id) override;

        void addWater(const osg::Vec2i& cellPosition, int cellSize, float level) override;

        void removeWater(const osg::Vec2i& cellPosition) override;

        void addHeightfield(const osg::Vec2i& cellPosition, int cellSize, const HeightfieldShape& shape) override;

        void removeHeightfield(const osg::Vec2i& cellPosition) override;

        void addPathgrid(const ESM::Cell& cell, const ESM::Pathgrid& pathgrid) override;

        void removePathgrid(const ESM::Pathgrid& pathgrid) override;

        void update(const osg::Vec3f& playerPosition) override;

        void wait(Loading::Listener& listener, WaitConditionType waitConditionType) override;

        SharedNavMeshCacheItem getNavMesh(const AgentBounds& agentBounds) const override;

        std::map<AgentBounds, SharedNavMeshCacheItem> getNavMeshes() const override;

        const Settings& getSettings() const override;

        void reportStats(unsigned int frameNumber, osg::Stats& stats) const override;

        RecastMeshTiles getRecastMeshTiles() const override;

        float getMaxNavmeshAreaRealRadius() const override;

    private:
        Settings mSettings;
        NavMeshManager mNavMeshManager;
        std::optional<TilePosition> mLastPlayerPosition;
        std::map<AgentBounds, std::size_t> mAgents;
        std::unordered_map<ObjectId, ObjectId> mAvoidIds;
        std::unordered_map<ObjectId, ObjectId> mWaterIds;

        inline bool addObjectImpl(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform);
        void updateAvoidShapeId(const ObjectId id, const ObjectId avoidId);
        void updateWaterShapeId(const ObjectId id, const ObjectId waterId);
        void updateId(const ObjectId id, const ObjectId waterId, std::unordered_map<ObjectId, ObjectId>& ids);
        void removeUnusedNavMeshes();
    };
}

#endif
