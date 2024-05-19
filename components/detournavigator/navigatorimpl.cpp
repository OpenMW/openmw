#include "navigatorimpl.hpp"
#include "makenavmesh.hpp"
#include "settingsutils.hpp"
#include "stats.hpp"

#include <components/esm3/loadpgrd.hpp>
#include <components/misc/convert.hpp>
#include <components/misc/coordinateconverter.hpp>

namespace DetourNavigator
{
    NavigatorImpl::NavigatorImpl(const Settings& settings, std::unique_ptr<NavMeshDb>&& db)
        : mSettings(settings)
        , mNavMeshManager(mSettings, std::move(db))
    {
    }

    bool NavigatorImpl::addAgent(const AgentBounds& agentBounds)
    {
        if (!isSupportedAgentBounds(mSettings.mRecast, agentBounds))
            return false;
        ++mAgents[agentBounds];
        mNavMeshManager.addAgent(agentBounds);
        return true;
    }

    void NavigatorImpl::removeAgent(const AgentBounds& agentBounds)
    {
        const auto it = mAgents.find(agentBounds);
        if (it == mAgents.end())
            return;
        if (it->second > 0)
            --it->second;
    }

    void NavigatorImpl::setWorldspace(ESM::RefId worldspace, const UpdateGuard* guard)
    {
        mNavMeshManager.setWorldspace(worldspace, guard);
    }

    void NavigatorImpl::updateBounds(const osg::Vec3f& playerPosition, const UpdateGuard* guard)
    {
        mNavMeshManager.updateBounds(playerPosition, guard);
    }

    void NavigatorImpl::addObject(
        const ObjectId id, const ObjectShapes& shapes, const btTransform& transform, const UpdateGuard* guard)
    {
        addObjectImpl(id, shapes, transform, guard);
    }

    bool NavigatorImpl::addObjectImpl(
        const ObjectId id, const ObjectShapes& shapes, const btTransform& transform, const UpdateGuard* guard)
    {
        const CollisionShape collisionShape(
            shapes.mShapeInstance, *shapes.mShapeInstance->mCollisionShape, shapes.mTransform);
        bool result = mNavMeshManager.addObject(id, collisionShape, transform, AreaType_ground, guard);
        if (const btCollisionShape* const avoidShape = shapes.mShapeInstance->mAvoidCollisionShape.get())
        {
            const ObjectId avoidId(avoidShape);
            const CollisionShape avoidCollisionShape(shapes.mShapeInstance, *avoidShape, shapes.mTransform);
            if (mNavMeshManager.addObject(avoidId, avoidCollisionShape, transform, AreaType_null, guard))
            {
                updateAvoidShapeId(id, avoidId, guard);
                result = true;
            }
        }
        return result;
    }

    void NavigatorImpl::addObject(
        const ObjectId id, const DoorShapes& shapes, const btTransform& transform, const UpdateGuard* guard)
    {
        if (addObjectImpl(id, static_cast<const ObjectShapes&>(shapes), transform, guard))
        {
            const osg::Vec3f start = toNavMeshCoordinates(mSettings.mRecast, shapes.mConnectionStart);
            const osg::Vec3f end = toNavMeshCoordinates(mSettings.mRecast, shapes.mConnectionEnd);
            mNavMeshManager.addOffMeshConnection(id, start, end, AreaType_door);
            mNavMeshManager.addOffMeshConnection(id, end, start, AreaType_door);
        }
    }

    void NavigatorImpl::updateObject(
        const ObjectId id, const ObjectShapes& shapes, const btTransform& transform, const UpdateGuard* guard)
    {
        mNavMeshManager.updateObject(id, transform, AreaType_ground, guard);
        if (const btCollisionShape* const avoidShape = shapes.mShapeInstance->mAvoidCollisionShape.get())
        {
            const ObjectId avoidId(avoidShape);
            if (mNavMeshManager.updateObject(avoidId, transform, AreaType_null, guard))
                updateAvoidShapeId(id, avoidId, guard);
        }
    }

    void NavigatorImpl::updateObject(
        const ObjectId id, const DoorShapes& shapes, const btTransform& transform, const UpdateGuard* guard)
    {
        return updateObject(id, static_cast<const ObjectShapes&>(shapes), transform, guard);
    }

    void NavigatorImpl::removeObject(const ObjectId id, const UpdateGuard* guard)
    {
        mNavMeshManager.removeObject(id, guard);
        const auto avoid = mAvoidIds.find(id);
        if (avoid != mAvoidIds.end())
            mNavMeshManager.removeObject(avoid->second, guard);
        const auto water = mWaterIds.find(id);
        if (water != mWaterIds.end())
            mNavMeshManager.removeObject(water->second, guard);
        mNavMeshManager.removeOffMeshConnections(id);
    }

    void NavigatorImpl::addWater(const osg::Vec2i& cellPosition, int cellSize, float level, const UpdateGuard* guard)
    {
        mNavMeshManager.addWater(cellPosition, cellSize, level, guard);
    }

    void NavigatorImpl::removeWater(const osg::Vec2i& cellPosition, const UpdateGuard* guard)
    {
        mNavMeshManager.removeWater(cellPosition, guard);
    }

    void NavigatorImpl::addHeightfield(
        const osg::Vec2i& cellPosition, int cellSize, const HeightfieldShape& shape, const UpdateGuard* guard)
    {
        mNavMeshManager.addHeightfield(cellPosition, cellSize, shape, guard);
    }

    void NavigatorImpl::removeHeightfield(const osg::Vec2i& cellPosition, const UpdateGuard* guard)
    {
        mNavMeshManager.removeHeightfield(cellPosition, guard);
    }

    void NavigatorImpl::addPathgrid(const ESM::Cell& cell, const ESM::Pathgrid& pathgrid)
    {
        const Misc::CoordinateConverter converter = Misc::makeCoordinateConverter(cell);
        for (const auto& edge : pathgrid.mEdges)
        {
            const auto src = Misc::Convert::makeOsgVec3f(converter.toWorldPoint(pathgrid.mPoints[edge.mV0]));
            const auto dst = Misc::Convert::makeOsgVec3f(converter.toWorldPoint(pathgrid.mPoints[edge.mV1]));
            mNavMeshManager.addOffMeshConnection(ObjectId(&pathgrid), toNavMeshCoordinates(mSettings.mRecast, src),
                toNavMeshCoordinates(mSettings.mRecast, dst), AreaType_pathgrid);
        }
    }

    void NavigatorImpl::removePathgrid(const ESM::Pathgrid& pathgrid)
    {
        mNavMeshManager.removeOffMeshConnections(ObjectId(&pathgrid));
    }

    void NavigatorImpl::update(const osg::Vec3f& playerPosition, const UpdateGuard* guard)
    {
        removeUnusedNavMeshes();
        mNavMeshManager.update(playerPosition, guard);
    }

    void NavigatorImpl::wait(WaitConditionType waitConditionType, Loading::Listener* listener)
    {
        mNavMeshManager.wait(waitConditionType, listener);
    }

    SharedNavMeshCacheItem NavigatorImpl::getNavMesh(const AgentBounds& agentBounds) const
    {
        return mNavMeshManager.getNavMesh(agentBounds);
    }

    std::map<AgentBounds, SharedNavMeshCacheItem> NavigatorImpl::getNavMeshes() const
    {
        return mNavMeshManager.getNavMeshes();
    }

    const Settings& NavigatorImpl::getSettings() const
    {
        return mSettings;
    }

    Stats NavigatorImpl::getStats() const
    {
        return mNavMeshManager.getStats();
    }

    RecastMeshTiles NavigatorImpl::getRecastMeshTiles() const
    {
        return mNavMeshManager.getRecastMeshTiles();
    }

    void NavigatorImpl::updateAvoidShapeId(const ObjectId id, const ObjectId avoidId, const UpdateGuard* guard)
    {
        updateId(id, avoidId, mWaterIds, guard);
    }

    void NavigatorImpl::updateId(const ObjectId id, const ObjectId updateId,
        std::unordered_map<ObjectId, ObjectId>& ids, const UpdateGuard* guard)
    {
        auto inserted = ids.insert(std::make_pair(id, updateId));
        if (!inserted.second)
        {
            mNavMeshManager.removeObject(inserted.first->second, guard);
            inserted.first->second = updateId;
        }
    }

    void NavigatorImpl::removeUnusedNavMeshes()
    {
        for (auto it = mAgents.begin(); it != mAgents.end();)
        {
            if (it->second == 0 && mNavMeshManager.reset(it->first))
                it = mAgents.erase(it);
            else
                ++it;
        }
    }

    float NavigatorImpl::getMaxNavmeshAreaRealRadius() const
    {
        const auto& settings = getSettings();
        return getRealTileSize(settings.mRecast) * getMaxNavmeshAreaRadius(settings);
    }
}
