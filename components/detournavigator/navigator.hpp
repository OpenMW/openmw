#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATOR_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATOR_H

#include <cassert>
#include <filesystem>

#include "heightfieldshape.hpp"
#include "objectid.hpp"
#include "objecttransform.hpp"
#include "recastmeshtiles.hpp"
#include "sharednavmeshcacheitem.hpp"
#include "updateguard.hpp"
#include "waitconditiontype.hpp"

#include <components/esm/refid.hpp>
#include <components/resource/bulletshape.hpp>

namespace ESM
{
    struct Cell;
    struct Pathgrid;
    class RefId;
}

namespace Loading
{
    class Listener;
}

namespace DetourNavigator
{
    struct Settings;
    struct AgentBounds;
    struct Stats;

    struct ObjectShapes
    {
        osg::ref_ptr<const Resource::BulletShapeInstance> mShapeInstance;
        ObjectTransform mTransform;

        ObjectShapes(
            const osg::ref_ptr<const Resource::BulletShapeInstance>& shapeInstance, const ObjectTransform& transform)
            : mShapeInstance(shapeInstance)
            , mTransform(transform)
        {
            assert(mShapeInstance != nullptr);
        }
    };

    struct DoorShapes : ObjectShapes
    {
        osg::Vec3f mConnectionStart;
        osg::Vec3f mConnectionEnd;

        DoorShapes(const osg::ref_ptr<const Resource::BulletShapeInstance>& shapeInstance,
            const ObjectTransform& transform, const osg::Vec3f& connectionStart, const osg::Vec3f& connectionEnd)
            : ObjectShapes(shapeInstance, transform)
            , mConnectionStart(connectionStart)
            , mConnectionEnd(connectionEnd)
        {
        }
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

        virtual ScopedUpdateGuard makeUpdateGuard() = 0;

        /**
         * @brief addAgent should be called for each agent even if all of them has same half extents.
         * @param agentBounds allows to setup bounding cylinder for each agent, for each different half extents
         * there is different navmesh.
         * @return true if agent is successfully added or false if agent bounds are not supported.
         */
        virtual bool addAgent(const AgentBounds& agentBounds) = 0;

        /**
         * @brief removeAgent should be called for each agent even if all of them has same half extents
         * @param agentBounds allows determine which agent to remove
         */
        virtual void removeAgent(const AgentBounds& agentBounds) = 0;

        // Updates bounds for recast mesh and navmesh tiles, removes tiles outside the range.
        virtual void updateBounds(ESM::RefId worldspace, const osg::Vec3f& playerPosition, const UpdateGuard* guard)
            = 0;

        /**
         * @brief addObject is used to add complex object with allowed to walk and avoided to walk shapes
         * @param id is used to distinguish different objects
         * @param shape members must live until object is updated by another shape removed from Navigator
         * @param transform allows to setup objects geometry according to its world state
         */
        virtual void addObject(
            const ObjectId id, const ObjectShapes& shapes, const btTransform& transform, const UpdateGuard* guard)
            = 0;

        /**
         * @brief addObject is used to add doors.
         * @param id is used to distinguish different objects.
         * @param shape members must live until object is updated by another shape or removed from Navigator.
         * @param transform allows to setup objects geometry according to its world state.
         */
        virtual void addObject(
            const ObjectId id, const DoorShapes& shapes, const btTransform& transform, const UpdateGuard* guard)
            = 0;

        /**
         * @brief updateObject replace object geometry by given data.
         * @param id is used to find object.
         * @param shape members must live until object is updated by another shape removed from Navigator.
         * @param transform allows to setup objects geometry according to its world state.
         */
        virtual void updateObject(
            const ObjectId id, const ObjectShapes& shapes, const btTransform& transform, const UpdateGuard* guard)
            = 0;

        /**
         * @brief updateObject replace object geometry by given data.
         * @param id is used to find object.
         * @param shape members must live until object is updated by another shape removed from Navigator.
         * @param transform allows to setup objects geometry according to its world state.
         */
        virtual void updateObject(
            const ObjectId id, const DoorShapes& shapes, const btTransform& transform, const UpdateGuard* guard)
            = 0;

        /**
         * @brief removeObject to make it no more available at the scene.
         * @param id is used to find object.
         */
        virtual void removeObject(const ObjectId id, const UpdateGuard* guard) = 0;

        /**
         * @brief addWater is used to set water level at given world cell.
         * @param cellPosition allows to distinguish cells if there is many in current world.
         * @param cellSize set cell borders. std::numeric_limits<int>::max() disables cell borders.
         * @param shift set global shift of cell center.
         */
        virtual void addWater(const osg::Vec2i& cellPosition, int cellSize, float level, const UpdateGuard* guard) = 0;

        /**
         * @brief removeWater to make it no more available at the scene.
         * @param cellPosition allows to find cell.
         */
        virtual void removeWater(const osg::Vec2i& cellPosition, const UpdateGuard* guard) = 0;

        virtual void addHeightfield(
            const osg::Vec2i& cellPosition, int cellSize, const HeightfieldShape& shape, const UpdateGuard* guard)
            = 0;

        virtual void removeHeightfield(const osg::Vec2i& cellPosition, const UpdateGuard* guard) = 0;

        virtual void addPathgrid(const ESM::Cell& cell, const ESM::Pathgrid& pathgrid) = 0;

        virtual void removePathgrid(const ESM::Pathgrid& pathgrid) = 0;

        /**
         * @brief update starts background navmesh update using current scene state.
         * @param playerPosition setup initial point to order build tiles of navmesh.
         */
        virtual void update(const osg::Vec3f& playerPosition, const UpdateGuard* guard) = 0;

        /**
         * @brief wait locks thread until tiles are updated from last update call based on passed condition type.
         * @param waitConditionType defines when waiting will stop
         * @param listener optional listener for a progress bar
         */
        virtual void wait(WaitConditionType waitConditionType, Loading::Listener* listener) = 0;

        /**
         * @brief getNavMesh returns navmesh for specific agent half extents
         * @return navmesh
         */
        virtual SharedNavMeshCacheItem getNavMesh(const AgentBounds& agentBounds) const = 0;

        /**
         * @brief getNavMeshes returns all current navmeshes
         * @return map of agent half extents to navmesh
         */
        virtual std::map<AgentBounds, SharedNavMeshCacheItem> getNavMeshes() const = 0;

        virtual const Settings& getSettings() const = 0;

        virtual Stats getStats() const = 0;

        virtual RecastMeshTiles getRecastMeshTiles() const = 0;

        virtual float getMaxNavmeshAreaRealRadius() const = 0;
    };

    std::unique_ptr<Navigator> makeNavigator(const Settings& settings, const std::filesystem::path& userDataPath);

    std::unique_ptr<Navigator> makeNavigatorStub();
}

#endif
