#include "navigatorimpl.hpp"
#include "settingsutils.hpp"
#include "stats.hpp"

#include <components/debug/debuglog.hpp>
#include <components/esm3/loadpgrd.hpp>
#include <components/misc/coordinateconverter.hpp>
#include <components/misc/convert.hpp>

namespace DetourNavigator
{
    NavigatorImpl::NavigatorImpl(const Settings& settings, std::unique_ptr<NavMeshDb>&& db)
        : mSettings(settings)
        , mNavMeshManager(mSettings, std::move(db))
    {
    }

    void NavigatorImpl::addAgent(const AgentBounds& agentBounds)
    {
        if(agentBounds.mHalfExtents.length2() <= 0)
            return;
        ++mAgents[agentBounds];
        mNavMeshManager.addAgent(agentBounds);
    }

    void NavigatorImpl::removeAgent(const AgentBounds& agentBounds)
    {
        const auto it = mAgents.find(agentBounds);
        if (it == mAgents.end())
            return;
        if (it->second > 0)
            --it->second;
    }

    void NavigatorImpl::setWorldspace(std::string_view worldspace)
    {
        mNavMeshManager.setWorldspace(worldspace);
    }

    void NavigatorImpl::updateBounds(const osg::Vec3f& playerPosition)
    {
        mNavMeshManager.updateBounds(playerPosition);
    }

    void NavigatorImpl::addObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform)
    {
        addObjectImpl(id, shapes, transform);
    }

    bool NavigatorImpl::addObjectImpl(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform)
    {
        const CollisionShape collisionShape(shapes.mShapeInstance, *shapes.mShapeInstance->mCollisionShape, shapes.mTransform);
        bool result = mNavMeshManager.addObject(id, collisionShape, transform, AreaType_ground);
        if (const btCollisionShape* const avoidShape = shapes.mShapeInstance->mAvoidCollisionShape.get())
        {
            const ObjectId avoidId(avoidShape);
            const CollisionShape avoidCollisionShape(shapes.mShapeInstance, *avoidShape, shapes.mTransform);
            if (mNavMeshManager.addObject(avoidId, avoidCollisionShape, transform, AreaType_null))
            {
                updateAvoidShapeId(id, avoidId);
                result = true;
            }
        }
        return result;
    }

    void NavigatorImpl::addObject(const ObjectId id, const DoorShapes& shapes, const btTransform& transform)
    {
        if (addObjectImpl(id, static_cast<const ObjectShapes&>(shapes), transform))
        {
            const osg::Vec3f start = toNavMeshCoordinates(mSettings.mRecast, shapes.mConnectionStart);
            const osg::Vec3f end = toNavMeshCoordinates(mSettings.mRecast, shapes.mConnectionEnd);
            mNavMeshManager.addOffMeshConnection(id, start, end, AreaType_door);
            mNavMeshManager.addOffMeshConnection(id, end, start, AreaType_door);
        }
    }

    void NavigatorImpl::updateObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform)
    {
        mNavMeshManager.updateObject(id, transform, AreaType_ground);
        if (const btCollisionShape* const avoidShape = shapes.mShapeInstance->mAvoidCollisionShape.get())
        {
            const ObjectId avoidId(avoidShape);
            if (mNavMeshManager.updateObject(avoidId, transform, AreaType_null))
                updateAvoidShapeId(id, avoidId);
        }
    }

    void NavigatorImpl::updateObject(const ObjectId id, const DoorShapes& shapes, const btTransform& transform)
    {
        return updateObject(id, static_cast<const ObjectShapes&>(shapes), transform);
    }

    void NavigatorImpl::removeObject(const ObjectId id)
    {
        mNavMeshManager.removeObject(id);
        const auto avoid = mAvoidIds.find(id);
        if (avoid != mAvoidIds.end())
            mNavMeshManager.removeObject(avoid->second);
        const auto water = mWaterIds.find(id);
        if (water != mWaterIds.end())
            mNavMeshManager.removeObject(water->second);
        mNavMeshManager.removeOffMeshConnections(id);
    }

    void NavigatorImpl::addWater(const osg::Vec2i& cellPosition, int cellSize, float level)
    {
        mNavMeshManager.addWater(cellPosition, cellSize, level);
    }

    void NavigatorImpl::removeWater(const osg::Vec2i& cellPosition)
    {
        mNavMeshManager.removeWater(cellPosition);
    }

    void NavigatorImpl::addHeightfield(const osg::Vec2i& cellPosition, int cellSize, const HeightfieldShape& shape)
    {
        mNavMeshManager.addHeightfield(cellPosition, cellSize, shape);
    }

    void NavigatorImpl::removeHeightfield(const osg::Vec2i& cellPosition)
    {
        mNavMeshManager.removeHeightfield(cellPosition);
    }

    void NavigatorImpl::addPathgrid(const ESM::Cell& cell, const ESM::Pathgrid& pathgrid)
    {
        Misc::CoordinateConverter converter(&cell);
        for (const auto& edge : pathgrid.mEdges)
        {
            const auto src = Misc::Convert::makeOsgVec3f(converter.toWorldPoint(pathgrid.mPoints[edge.mV0]));
            const auto dst = Misc::Convert::makeOsgVec3f(converter.toWorldPoint(pathgrid.mPoints[edge.mV1]));
            mNavMeshManager.addOffMeshConnection(
                ObjectId(&pathgrid),
                toNavMeshCoordinates(mSettings.mRecast, src),
                toNavMeshCoordinates(mSettings.mRecast, dst),
                AreaType_pathgrid
            );
        }
    }

    void NavigatorImpl::removePathgrid(const ESM::Pathgrid& pathgrid)
    {
        mNavMeshManager.removeOffMeshConnections(ObjectId(&pathgrid));
    }

    void NavigatorImpl::update(const osg::Vec3f& playerPosition)
    {
        removeUnusedNavMeshes();
        mNavMeshManager.update(playerPosition);
    }

    void NavigatorImpl::wait(Loading::Listener& listener, WaitConditionType waitConditionType)
    {
        mNavMeshManager.wait(listener, waitConditionType);
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

    void NavigatorImpl::updateAvoidShapeId(const ObjectId id, const ObjectId avoidId)
    {
        updateId(id, avoidId, mWaterIds);
    }

    void NavigatorImpl::updateWaterShapeId(const ObjectId id, const ObjectId waterId)
    {
        updateId(id, waterId, mWaterIds);
    }

    void NavigatorImpl::updateId(const ObjectId id, const ObjectId updateId, std::unordered_map<ObjectId, ObjectId>& ids)
    {
        auto inserted = ids.insert(std::make_pair(id, updateId));
        if (!inserted.second)
        {
            mNavMeshManager.removeObject(inserted.first->second);
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
