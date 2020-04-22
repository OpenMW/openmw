#include "navigatorimpl.hpp"
#include "debug.hpp"
#include "settingsutils.hpp"

#include <Recast.h>

namespace DetourNavigator
{
    NavigatorImpl::NavigatorImpl(const Settings& settings)
        : mSettings(settings)
        , mNavMeshManager(mSettings)
    {
    }

    void NavigatorImpl::addAgent(const osg::Vec3f& agentHalfExtents)
    {
        ++mAgents[agentHalfExtents];
        mNavMeshManager.addAgent(agentHalfExtents);
    }

    void NavigatorImpl::removeAgent(const osg::Vec3f& agentHalfExtents)
    {
        const auto it = mAgents.find(agentHalfExtents);
        if (it == mAgents.end())
            return;
        if (it->second > 0)
            --it->second;
    }

    bool NavigatorImpl::addObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform)
    {
        return mNavMeshManager.addObject(id, shape, transform, AreaType_ground);
    }

    bool NavigatorImpl::addObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform)
    {
        bool result = addObject(id, shapes.mShape, transform);
        if (shapes.mAvoid)
        {
            const ObjectId avoidId(shapes.mAvoid);
            if (mNavMeshManager.addObject(avoidId, *shapes.mAvoid, transform, AreaType_null))
            {
                updateAvoidShapeId(id, avoidId);
                result = true;
            }
        }
        return result;
    }

    bool NavigatorImpl::addObject(const ObjectId id, const DoorShapes& shapes, const btTransform& transform)
    {
        if (addObject(id, static_cast<const ObjectShapes&>(shapes), transform))
        {
            mNavMeshManager.addOffMeshConnection(
                id,
                toNavMeshCoordinates(mSettings, shapes.mConnectionStart),
                toNavMeshCoordinates(mSettings, shapes.mConnectionEnd)
            );
            return true;
        }
        return false;
    }

    bool NavigatorImpl::updateObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform)
    {
        return mNavMeshManager.updateObject(id, shape, transform, AreaType_ground);
    }

    bool NavigatorImpl::updateObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform)
    {
        bool result = updateObject(id, shapes.mShape, transform);
        if (shapes.mAvoid)
        {
            const ObjectId avoidId(shapes.mAvoid);
            if (mNavMeshManager.updateObject(avoidId, *shapes.mAvoid, transform, AreaType_null))
            {
                updateAvoidShapeId(id, avoidId);
                result = true;
            }
        }
        return result;
    }

    bool NavigatorImpl::updateObject(const ObjectId id, const DoorShapes& shapes, const btTransform& transform)
    {
        return updateObject(id, static_cast<const ObjectShapes&>(shapes), transform);
    }

    bool NavigatorImpl::removeObject(const ObjectId id)
    {
        bool result = mNavMeshManager.removeObject(id);
        const auto avoid = mAvoidIds.find(id);
        if (avoid != mAvoidIds.end())
            result = mNavMeshManager.removeObject(avoid->second) || result;
        const auto water = mWaterIds.find(id);
        if (water != mWaterIds.end())
            result = mNavMeshManager.removeObject(water->second) || result;
        mNavMeshManager.removeOffMeshConnection(id);
        return result;
    }

    bool NavigatorImpl::addWater(const osg::Vec2i& cellPosition, const int cellSize, const btScalar level,
        const btTransform& transform)
    {
        return mNavMeshManager.addWater(cellPosition, cellSize,
            btTransform(transform.getBasis(), btVector3(transform.getOrigin().x(), transform.getOrigin().y(), level)));
    }

    bool NavigatorImpl::removeWater(const osg::Vec2i& cellPosition)
    {
        return mNavMeshManager.removeWater(cellPosition);
    }

    void NavigatorImpl::update(const osg::Vec3f& playerPosition)
    {
        removeUnusedNavMeshes();
        for (const auto& v : mAgents)
            mNavMeshManager.update(playerPosition, v.first);
    }

    void NavigatorImpl::wait()
    {
        mNavMeshManager.wait();
    }

    SharedNavMeshCacheItem NavigatorImpl::getNavMesh(const osg::Vec3f& agentHalfExtents) const
    {
        return mNavMeshManager.getNavMesh(agentHalfExtents);
    }

    std::map<osg::Vec3f, SharedNavMeshCacheItem> NavigatorImpl::getNavMeshes() const
    {
        return mNavMeshManager.getNavMeshes();
    }

    const Settings& NavigatorImpl::getSettings() const
    {
        return mSettings;
    }

    void NavigatorImpl::reportStats(unsigned int frameNumber, osg::Stats& stats) const
    {
        mNavMeshManager.reportStats(frameNumber, stats);
    }

    RecastMeshTiles NavigatorImpl::getRecastMeshTiles()
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
}
