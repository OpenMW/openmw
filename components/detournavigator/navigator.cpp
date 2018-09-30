#include "navigator.hpp"
#include "debug.hpp"
#include "settingsutils.hpp"

#include <Recast.h>

namespace DetourNavigator
{
    Navigator::Navigator(const Settings& settings)
        : mSettings(settings)
        , mNavMeshManager(mSettings)
    {
    }

    void Navigator::addAgent(const osg::Vec3f& agentHalfExtents)
    {
        ++mAgents[agentHalfExtents];
        mNavMeshManager.addAgent(agentHalfExtents);
    }

    void Navigator::removeAgent(const osg::Vec3f& agentHalfExtents)
    {
        const auto it = mAgents.find(agentHalfExtents);
        if (it == mAgents.end() || --it->second)
            return;
        mAgents.erase(it);
        mNavMeshManager.reset(agentHalfExtents);
    }

    bool Navigator::addObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform)
    {
        return mNavMeshManager.addObject(id, shape, transform, AreaType_ground);
    }

    bool Navigator::addObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform)
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

    bool Navigator::addObject(const ObjectId id, const DoorShapes& shapes, const btTransform& transform)
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

    bool Navigator::updateObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform)
    {
        return mNavMeshManager.updateObject(id, shape, transform, AreaType_ground);
    }

    bool Navigator::updateObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform)
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

    bool Navigator::updateObject(const ObjectId id, const DoorShapes& shapes, const btTransform& transform)
    {
        return updateObject(id, static_cast<const ObjectShapes&>(shapes), transform);
    }

    bool Navigator::removeObject(const ObjectId id)
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

    bool Navigator::addWater(const osg::Vec2i& cellPosition, const int cellSize, const btScalar level,
        const btTransform& transform)
    {
        return mNavMeshManager.addWater(cellPosition, cellSize,
            btTransform(transform.getBasis(), btVector3(transform.getOrigin().x(), transform.getOrigin().y(), level)));
    }

    bool Navigator::removeWater(const osg::Vec2i& cellPosition)
    {
        return mNavMeshManager.removeWater(cellPosition);
    }

    void Navigator::update(const osg::Vec3f& playerPosition)
    {
        for (const auto& v : mAgents)
            mNavMeshManager.update(playerPosition, v.first);
    }

    void Navigator::wait()
    {
        mNavMeshManager.wait();
    }

    std::map<osg::Vec3f, SharedNavMeshCacheItem> Navigator::getNavMeshes() const
    {
        return mNavMeshManager.getNavMeshes();
    }

    const Settings& Navigator::getSettings() const
    {
        return mSettings;
    }

    void Navigator::updateAvoidShapeId(const ObjectId id, const ObjectId avoidId)
    {
        updateId(id, avoidId, mWaterIds);
    }

    void Navigator::updateWaterShapeId(const ObjectId id, const ObjectId waterId)
    {
        updateId(id, waterId, mWaterIds);
    }

    void Navigator::updateId(const ObjectId id, const ObjectId updateId, std::unordered_map<ObjectId, ObjectId>& ids)
    {
        auto inserted = ids.insert(std::make_pair(id, updateId));
        if (!inserted.second)
        {
            mNavMeshManager.removeObject(inserted.first->second);
            inserted.first->second = updateId;
        }
    }
}
