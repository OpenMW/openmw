﻿#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATOR_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATOR_H

#include "findsmoothpath.hpp"
#include "flags.hpp"
#include "settings.hpp"
#include "objectid.hpp"
#include "navmeshcacheitem.hpp"
#include "recastmeshtiles.hpp"
#include "waitconditiontype.hpp"
#include "heightfieldshape.hpp"

#include <components/resource/bulletshape.hpp>

#include <variant>

namespace ESM
{
    struct Cell;
    struct Pathgrid;
}

namespace Loading
{
    class Listener;
}

namespace DetourNavigator
{
    struct ObjectShapes
    {
        osg::ref_ptr<const Resource::BulletShapeInstance> mShapeInstance;

        ObjectShapes(const osg::ref_ptr<const Resource::BulletShapeInstance>& shapeInstance)
            : mShapeInstance(shapeInstance)
        {}
    };

    struct DoorShapes : ObjectShapes
    {
        osg::Vec3f mConnectionStart;
        osg::Vec3f mConnectionEnd;

        DoorShapes(const osg::ref_ptr<const Resource::BulletShapeInstance>& shapeInstance,
                   const osg::Vec3f& connectionStart,const osg::Vec3f& connectionEnd)
            : ObjectShapes(shapeInstance)
            , mConnectionStart(connectionStart)
            , mConnectionEnd(connectionEnd)
        {}
    };

    /**
     * @brief Top level interface of detournavigator component. Navigator allows to build a scene with navmesh and find
     * a path for an agent there. Scene contains agents, geometry objects and water. Agent are distinguished only by
     * half extents. Each object has unique identifier and could be added, updated or removed. Water could be added once
     * for each world cell at given level of height. Navmesh builds asynchronously in separate threads. To start build
     * navmesh call update method.
     */
    struct Navigator
    {
        virtual ~Navigator() = default;

        /**
         * @brief addAgent should be called for each agent even if all of them has same half extents.
         * @param agentHalfExtents allows to setup bounding cylinder for each agent, for each different half extents
         * there is different navmesh.
         */
        virtual void addAgent(const osg::Vec3f& agentHalfExtents) = 0;

        /**
         * @brief removeAgent should be called for each agent even if all of them has same half extents
         * @param agentHalfExtents allows determine which agent to remove
         */
        virtual void removeAgent(const osg::Vec3f& agentHalfExtents) = 0;

        /**
         * @brief addObject is used to add complex object with allowed to walk and avoided to walk shapes
         * @param id is used to distinguish different objects
         * @param shape members must live until object is updated by another shape removed from Navigator
         * @param transform allows to setup objects geometry according to its world state
         * @return true if object is added, false if there is already object with given id
         */
        virtual bool addObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform) = 0;

        /**
         * @brief addObject is used to add doors.
         * @param id is used to distinguish different objects.
         * @param shape members must live until object is updated by another shape or removed from Navigator.
         * @param transform allows to setup objects geometry according to its world state.
         * @return true if object is added, false if there is already object with given id.
         */
        virtual bool addObject(const ObjectId id, const DoorShapes& shapes, const btTransform& transform) = 0;

        /**
         * @brief updateObject replace object geometry by given data.
         * @param id is used to find object.
         * @param shape members must live until object is updated by another shape removed from Navigator.
         * @param transform allows to setup objects geometry according to its world state.
         * @return true if object is updated, false if there is no object with given id.
         */
        virtual bool updateObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform) = 0;

        /**
         * @brief updateObject replace object geometry by given data.
         * @param id is used to find object.
         * @param shape members must live until object is updated by another shape removed from Navigator.
         * @param transform allows to setup objects geometry according to its world state.
         * @return true if object is updated, false if there is no object with given id.
         */
        virtual bool updateObject(const ObjectId id, const DoorShapes& shapes, const btTransform& transform) = 0;

        /**
         * @brief removeObject to make it no more available at the scene.
         * @param id is used to find object.
         * @return true if object is removed, false if there is no object with given id.
         */
        virtual bool removeObject(const ObjectId id) = 0;

        /**
         * @brief addWater is used to set water level at given world cell.
         * @param cellPosition allows to distinguish cells if there is many in current world.
         * @param cellSize set cell borders. std::numeric_limits<int>::max() disables cell borders.
         * @param shift set global shift of cell center.
         * @return true if there was no water at given cell if cellSize != std::numeric_limits<int>::max() or there is
         * at least single object is added to the scene, false if there is already water for given cell or there is no
         * any other objects.
         */
        virtual bool addWater(const osg::Vec2i& cellPosition, int cellSize, const osg::Vec3f& shift) = 0;

        /**
         * @brief removeWater to make it no more available at the scene.
         * @param cellPosition allows to find cell.
         * @return true if there was water at given cell.
         */
        virtual bool removeWater(const osg::Vec2i& cellPosition) = 0;

        virtual bool addHeightfield(const osg::Vec2i& cellPosition, int cellSize, const osg::Vec3f& shift,
            const HeightfieldShape& shape) = 0;

        virtual bool removeHeightfield(const osg::Vec2i& cellPosition) = 0;

        virtual void addPathgrid(const ESM::Cell& cell, const ESM::Pathgrid& pathgrid) = 0;

        virtual void removePathgrid(const ESM::Pathgrid& pathgrid) = 0;

        /**
         * @brief update starts background navmesh update using current scene state.
         * @param playerPosition setup initial point to order build tiles of navmesh.
         */
        virtual void update(const osg::Vec3f& playerPosition) = 0;

        /**
         * @brief updatePlayerPosition starts background navmesh update using current scene state only when player position has been changed.
         * @param playerPosition setup initial point to order build tiles of navmesh.
         */
        virtual void updatePlayerPosition(const osg::Vec3f& playerPosition) = 0;

        /**
         * @brief disable navigator updates
         */
        virtual void setUpdatesEnabled(bool enabled) = 0;

        /**
         * @brief wait locks thread until tiles are updated from last update call based on passed condition type.
         * @param waitConditionType defines when waiting will stop
         */
        virtual void wait(Loading::Listener& listener, WaitConditionType waitConditionType) = 0;

        /**
         * @brief findPath fills output iterator with points of scene surfaces to be used for actor to walk through.
         * @param agentHalfExtents allows to find navmesh for given actor.
         * @param start path from given point.
         * @param end path at given point.
         * @param includeFlags setup allowed surfaces for actor to walk.
         * @param out the beginning of the destination range.
         * @param endTolerance defines maximum allowed distance to end path point in addition to agentHalfExtents
         * @return Output iterator to the element in the destination range, one past the last element of found path.
         * Equal to out if no path is found.
         */
        template <class OutputIterator>
        Status findPath(const osg::Vec3f& agentHalfExtents, const float stepSize, const osg::Vec3f& start,
            const osg::Vec3f& end, const Flags includeFlags, const DetourNavigator::AreaCosts& areaCosts,
            float endTolerance, OutputIterator& out) const
        {
            static_assert(
                std::is_same<
                    typename std::iterator_traits<OutputIterator>::iterator_category,
                    std::output_iterator_tag
                >::value,
                "out is not an OutputIterator"
            );
            const auto navMesh = getNavMesh(agentHalfExtents);
            if (!navMesh)
                return Status::NavMeshNotFound;
            const auto settings = getSettings();
            return findSmoothPath(navMesh->lockConst()->getImpl(), toNavMeshCoordinates(settings, agentHalfExtents),
                toNavMeshCoordinates(settings, stepSize), toNavMeshCoordinates(settings, start),
                toNavMeshCoordinates(settings, end), includeFlags, areaCosts, settings, endTolerance, out);
        }

        /**
         * @brief getNavMesh returns navmesh for specific agent half extents
         * @return navmesh
         */
        virtual SharedNavMeshCacheItem getNavMesh(const osg::Vec3f& agentHalfExtents) const = 0;

        /**
         * @brief getNavMeshes returns all current navmeshes
         * @return map of agent half extents to navmesh
         */
        virtual std::map<osg::Vec3f, SharedNavMeshCacheItem> getNavMeshes() const = 0;

        virtual const Settings& getSettings() const = 0;

        virtual void reportStats(unsigned int frameNumber, osg::Stats& stats) const = 0;

        /**
         * @brief findRandomPointAroundCircle returns random location on navmesh within the reach of specified location.
         * @param agentHalfExtents allows to find navmesh for given actor.
         * @param start path from given point.
         * @param maxRadius limit maximum distance from start.
         * @param includeFlags setup allowed surfaces for actor to walk.
         * @return not empty optional with position if point is found and empty optional if point is not found.
         */
        std::optional<osg::Vec3f> findRandomPointAroundCircle(const osg::Vec3f& agentHalfExtents,
            const osg::Vec3f& start, const float maxRadius, const Flags includeFlags) const;

        /**
         * @brief raycast finds farest navmesh point from start on a line from start to end that has path from start.
         * @param agentHalfExtents allows to find navmesh for given actor.
         * @param start of the line
         * @param end of the line
         * @param includeFlags setup allowed surfaces for actor to walk.
         * @return not empty optional with position if point is found and empty optional if point is not found.
         */
        std::optional<osg::Vec3f> raycast(const osg::Vec3f& agentHalfExtents, const osg::Vec3f& start,
            const osg::Vec3f& end, const Flags includeFlags) const;

        virtual RecastMeshTiles getRecastMeshTiles() const = 0;

        virtual float getMaxNavmeshAreaRealRadius() const = 0;
    };
}

#endif
