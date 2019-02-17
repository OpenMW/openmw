#include "navigatorimpl.hpp"
#include "debug.hpp"
#include "settingsutils.hpp"

#include <components/debug/debuglog.hpp>

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
        const ::ProfileScope profile("Navigator::addAgent");
        ++mAgents[agentHalfExtents];
        mNavMeshManager.addAgent(agentHalfExtents);
    }

    void NavigatorImpl::removeAgent(const osg::Vec3f& agentHalfExtents)
    {
        const ::ProfileScope profile("Navigator::removeAgent");
        const auto it = mAgents.find(agentHalfExtents);
        if (it == mAgents.end() || --it->second)
            return;
        mAgents.erase(it);
        mNavMeshManager.reset(agentHalfExtents);
    }

    bool NavigatorImpl::addObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform)
    {
        const ::ProfileScope profile("Navigator::addObject1");
        return mNavMeshManager.addObject(id, shape, transform, AreaType_ground);
    }

    bool NavigatorImpl::addObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform)
    {
        const ::ProfileScope profile("Navigator::addObject2");
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
        const ::ProfileScope profile("Navigator::addObject3");
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
        const ::ProfileScope profile("Navigator::updateObject1");
        return mNavMeshManager.updateObject(id, shape, transform, AreaType_ground);
    }

    bool NavigatorImpl::updateObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform)
    {
        const ::ProfileScope profile("Navigator::updateObject2");
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
        const ::ProfileScope profile("Navigator::updateObject3");
        return updateObject(id, static_cast<const ObjectShapes&>(shapes), transform);
    }

    bool NavigatorImpl::removeObject(const ObjectId id)
    {
        const ::ProfileScope profile("Navigator::removeObject");
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
        const ::ProfileScope profile("Navigator::addWater");
        return mNavMeshManager.addWater(cellPosition, cellSize,
            btTransform(transform.getBasis(), btVector3(transform.getOrigin().x(), transform.getOrigin().y(), level)));
    }

    bool NavigatorImpl::removeWater(const osg::Vec2i& cellPosition)
    {
        const ::ProfileScope profile("Navigator::removeWater");
        return mNavMeshManager.removeWater(cellPosition);
    }

    void NavigatorImpl::update(const osg::Vec3f& playerPosition)
    {
        const ::ProfileScope profile("Navigator::update");
        for (const auto& v : mAgents)
            mNavMeshManager.update(playerPosition, v.first);
    }

    void NavigatorImpl::wait()
    {
        const ::ProfileScope profile("Navigator::wait");
        mNavMeshManager.wait();
    }

    SharedNavMeshCacheItem NavigatorImpl::getNavMesh(const osg::Vec3f& agentHalfExtents) const
    {
        return mNavMeshManager.getNavMesh(agentHalfExtents);
    }

    std::map<osg::Vec3f, SharedNavMeshCacheItem> NavigatorImpl::getNavMeshes() const
    {
        const ::ProfileScope profile("Navigator::getNavMeshes");
        return mNavMeshManager.getNavMeshes();
    }

    const Settings& NavigatorImpl::getSettings() const
    {
        return mSettings;
    }

    void NavigatorImpl::updateAvoidShapeId(const ObjectId id, const ObjectId avoidId)
    {
        const ::ProfileScope profile("Navigator::updateAvoidShapeId");
        updateId(id, avoidId, mWaterIds);
    }

    void NavigatorImpl::updateWaterShapeId(const ObjectId id, const ObjectId waterId)
    {
        const ::ProfileScope profile("Navigator::updateWaterShapeId");
        updateId(id, waterId, mWaterIds);
    }

    void NavigatorImpl::updateId(const ObjectId id, const ObjectId updateId, std::unordered_map<ObjectId, ObjectId>& ids)
    {
        const ::ProfileScope profile("Navigator::updateId");
        auto inserted = ids.insert(std::make_pair(id, updateId));
        if (!inserted.second)
        {
            mNavMeshManager.removeObject(inserted.first->second);
            inserted.first->second = updateId;
        }
    }
}
