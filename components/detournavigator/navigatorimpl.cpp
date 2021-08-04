#include "navigatorimpl.hpp"
#include "debug.hpp"
#include "settingsutils.hpp"

#include <components/esm/loadpgrd.hpp>
#include <components/misc/coordinateconverter.hpp>

namespace DetourNavigator
{
    NavigatorImpl::NavigatorImpl(const Settings& settings)
        : mSettings(settings)
        , mNavMeshManager(mSettings)
        , mUpdatesEnabled(true)
    {
    }

    void NavigatorImpl::addAgent(const osg::Vec3f& agentHalfExtents)
    {
        if(agentHalfExtents.length2() <= 0)
            return;
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

    bool NavigatorImpl::addObject(const ObjectId id, const osg::ref_ptr<const osg::Object>& holder,
        const btHeightfieldTerrainShape& shape, const btTransform& transform)
    {
        const CollisionShape collisionShape {holder, shape};
        return mNavMeshManager.addObject(id, collisionShape, transform, AreaType_ground);
    }

    bool NavigatorImpl::addObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform)
    {
        const CollisionShape collisionShape {shapes.mShapeInstance, *shapes.mShapeInstance->getCollisionShape()};
        bool result = mNavMeshManager.addObject(id, collisionShape, transform, AreaType_ground);
        if (const btCollisionShape* const avoidShape = shapes.mShapeInstance->getAvoidCollisionShape())
        {
            const ObjectId avoidId(avoidShape);
            const CollisionShape collisionShape {shapes.mShapeInstance, *avoidShape};
            if (mNavMeshManager.addObject(avoidId, collisionShape, transform, AreaType_null))
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
            const osg::Vec3f start = toNavMeshCoordinates(mSettings, shapes.mConnectionStart);
            const osg::Vec3f end = toNavMeshCoordinates(mSettings, shapes.mConnectionEnd);
            mNavMeshManager.addOffMeshConnection(id, start, end, AreaType_door);
            mNavMeshManager.addOffMeshConnection(id, end, start, AreaType_door);
            return true;
        }
        return false;
    }

    bool NavigatorImpl::updateObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform)
    {
        const CollisionShape collisionShape {shapes.mShapeInstance, *shapes.mShapeInstance->getCollisionShape()};
        bool result = mNavMeshManager.updateObject(id, collisionShape, transform, AreaType_ground);
        if (const btCollisionShape* const avoidShape = shapes.mShapeInstance->getAvoidCollisionShape())
        {
            const ObjectId avoidId(avoidShape);
            const CollisionShape collisionShape {shapes.mShapeInstance, *avoidShape};
            if (mNavMeshManager.updateObject(avoidId, collisionShape, transform, AreaType_null))
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
        mNavMeshManager.removeOffMeshConnections(id);
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

    void NavigatorImpl::addPathgrid(const ESM::Cell& cell, const ESM::Pathgrid& pathgrid)
    {
        Misc::CoordinateConverter converter(&cell);
        for (auto edge : pathgrid.mEdges)
        {
            const auto src = Misc::Convert::makeOsgVec3f(converter.toWorldPoint(pathgrid.mPoints[edge.mV0]));
            const auto dst = Misc::Convert::makeOsgVec3f(converter.toWorldPoint(pathgrid.mPoints[edge.mV1]));
            mNavMeshManager.addOffMeshConnection(
                ObjectId(&pathgrid),
                toNavMeshCoordinates(mSettings, src),
                toNavMeshCoordinates(mSettings, dst),
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
        if (!mUpdatesEnabled)
            return;
        removeUnusedNavMeshes();
        for (const auto& v : mAgents)
            mNavMeshManager.update(playerPosition, v.first);
    }

    void NavigatorImpl::updatePlayerPosition(const osg::Vec3f& playerPosition)
    {
        const TilePosition tilePosition = getTilePosition(mSettings, toNavMeshCoordinates(mSettings, playerPosition));
        if (mLastPlayerPosition.has_value() && *mLastPlayerPosition == tilePosition)
            return;
        update(playerPosition);
        mLastPlayerPosition = tilePosition;
    }

    void NavigatorImpl::setUpdatesEnabled(bool enabled)
    {
        mUpdatesEnabled = enabled;
    }

    void NavigatorImpl::wait(Loading::Listener& listener, WaitConditionType waitConditionType)
    {
        mNavMeshManager.wait(listener, waitConditionType);
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

    float NavigatorImpl::getMaxNavmeshAreaRealRadius() const
    {
        const auto& settings = getSettings();
        return getRealTileSize(settings) * getMaxNavmeshAreaRadius(settings);
    }
}
