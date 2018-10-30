#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATOR_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_NAVIGATOR_H

#include "findsmoothpath.hpp"
#include "flags.hpp"
#include "navmeshmanager.hpp"
#include "settings.hpp"
#include "settingsutils.hpp"

namespace DetourNavigator
{
    struct ObjectShapes
    {
        const btCollisionShape& mShape;
        const btCollisionShape* mAvoid;

        ObjectShapes(const btCollisionShape& shape, const btCollisionShape* avoid)
            : mShape(shape), mAvoid(avoid)
        {}
    };

    struct DoorShapes : ObjectShapes
    {
        osg::Vec3f mConnectionStart;
        osg::Vec3f mConnectionEnd;

        DoorShapes(const btCollisionShape& shape, const btCollisionShape* avoid,
                   const osg::Vec3f& connectionStart,const osg::Vec3f& connectionEnd)
            : ObjectShapes(shape, avoid)
            , mConnectionStart(connectionStart)
            , mConnectionEnd(connectionEnd)
        {}
    };

    /**
     * @brief Top level class of detournavigator componenet. Navigator allows to build a scene with navmesh and find
     * a path for an agent there. Scene contains agents, geometry objects and water. Agent are distinguished only by
     * half extents. Each object has unique identifier and could be added, updated or removed. Water could be added once
     * for each world cell at given level of height. Navmesh builds asynchronously in separate threads. To start build
     * navmesh call update method.
     */
    class Navigator
    {
    public:
        /**
         * @brief Navigator constructor initializes all internal data. Constructed object is ready to build a scene.
         * @param settings allows to customize navigator work. Constructor is only place to set navigator settings.
         */
        Navigator(const Settings& settings);

        /**
         * @brief addAgent should be called for each agent even if all of them has same half extents.
         * @param agentHalfExtents allows to setup bounding cylinder for each agent, for each different half extents
         * there is different navmesh.
         */
        void addAgent(const osg::Vec3f& agentHalfExtents);

        /**
         * @brief removeAgent should be called for each agent even if all of them has same half extents
         * @param agentHalfExtents allows determine which agent to remove
         */
        void removeAgent(const osg::Vec3f& agentHalfExtents);

        /**
         * @brief addObject is used to add object represented by single btCollisionShape and btTransform.
         * @param id is used to distinguish different objects.
         * @param shape must live until object is updated by another shape removed from Navigator.
         * @param transform allows to setup object geometry according to its world state.
         * @return true if object is added, false if there is already object with given id.
         */
        bool addObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform);

        /**
         * @brief addObject is used to add complex object with allowed to walk and avoided to walk shapes
         * @param id is used to distinguish different objects
         * @param shape members must live until object is updated by another shape removed from Navigator
         * @param transform allows to setup objects geometry according to its world state
         * @return true if object is added, false if there is already object with given id
         */
        bool addObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform);

        /**
         * @brief addObject is used to add doors.
         * @param id is used to distinguish different objects.
         * @param shape members must live until object is updated by another shape or removed from Navigator.
         * @param transform allows to setup objects geometry according to its world state.
         * @return true if object is added, false if there is already object with given id.
         */
        bool addObject(const ObjectId id, const DoorShapes& shapes, const btTransform& transform);

        /**
         * @brief updateObject replace object geometry by given data.
         * @param id is used to find object.
         * @param shape must live until object is updated by another shape removed from Navigator.
         * @param transform allows to setup objects geometry according to its world state.
         * @return true if object is updated, false if there is no object with given id.
         */
        bool updateObject(const ObjectId id, const btCollisionShape& shape, const btTransform& transform);

        /**
         * @brief updateObject replace object geometry by given data.
         * @param id is used to find object.
         * @param shape members must live until object is updated by another shape removed from Navigator.
         * @param transform allows to setup objects geometry according to its world state.
         * @return true if object is updated, false if there is no object with given id.
         */
        bool updateObject(const ObjectId id, const ObjectShapes& shapes, const btTransform& transform);

        /**
         * @brief updateObject replace object geometry by given data.
         * @param id is used to find object.
         * @param shape members must live until object is updated by another shape removed from Navigator.
         * @param transform allows to setup objects geometry according to its world state.
         * @return true if object is updated, false if there is no object with given id.
         */
        bool updateObject(const ObjectId id, const DoorShapes& shapes, const btTransform& transform);

        /**
         * @brief removeObject to make it no more available at the scene.
         * @param id is used to find object.
         * @return true if object is removed, false if there is no object with given id.
         */
        bool removeObject(const ObjectId id);

        /**
         * @brief addWater is used to set water level at given world cell.
         * @param cellPosition allows to distinguish cells if there is many in current world.
         * @param cellSize set cell borders. std::numeric_limits<int>::max() disables cell borders.
         * @param level set z coordinate of water surface at the scene.
         * @param transform set global shift of cell center.
         * @return true if there was no water at given cell if cellSize != std::numeric_limits<int>::max() or there is
         * at least single object is added to the scene, false if there is already water for given cell or there is no
         * any other objects.
         */
        bool addWater(const osg::Vec2i& cellPosition, const int cellSize, const btScalar level,
            const btTransform& transform);

        /**
         * @brief removeWater to make it no more available at the scene.
         * @param cellPosition allows to find cell.
         * @return true if there was water at given cell.
         */
        bool removeWater(const osg::Vec2i& cellPosition);

        /**
         * @brief update start background navmesh update using current scene state.
         * @param playerPosition setup initial point to order build tiles of navmesh.
         */
        void update(const osg::Vec3f& playerPosition);

        /**
         * @brief wait locks thread until all tiles are updated from last update call.
         */
        void wait();

        /**
         * @brief findPath fills output iterator with points of scene surfaces to be used for actor to walk through.
         * @param agentHalfExtents allows to find navmesh for given actor.
         * @param start path from given point.
         * @param end path at given point.
         * @param includeFlags setup allowed surfaces for actor to walk.
         * @param out the beginning of the destination range.
         * @return Output iterator to the element in the destination range, one past the last element of found path.
         * Equal to out if no path is found.
         * @throws InvalidArgument if there is no navmesh for given agentHalfExtents.
         */
        template <class OutputIterator>
        OutputIterator findPath(const osg::Vec3f& agentHalfExtents, const osg::Vec3f& start,
            const osg::Vec3f& end, const Flags includeFlags, OutputIterator out) const
        {
            static_assert(
                std::is_same<
                    typename std::iterator_traits<OutputIterator>::iterator_category,
                    std::output_iterator_tag
                >::value,
                "out is not an OutputIterator"
            );
            const auto navMesh = mNavMeshManager.getNavMesh(agentHalfExtents);
            return findSmoothPath(navMesh.lock()->getValue(), toNavMeshCoordinates(mSettings, agentHalfExtents),
                toNavMeshCoordinates(mSettings, start), toNavMeshCoordinates(mSettings, end), includeFlags,
                mSettings, out);
        }

        /**
         * @brief getNavMeshes returns all current navmeshes
         * @return map of agent half extents to navmesh
         */
        std::map<osg::Vec3f, SharedNavMeshCacheItem> getNavMeshes() const;

        const Settings& getSettings() const;

    private:
        Settings mSettings;
        NavMeshManager mNavMeshManager;
        std::map<osg::Vec3f, std::size_t> mAgents;
        std::unordered_map<ObjectId, ObjectId> mAvoidIds;
        std::unordered_map<ObjectId, ObjectId> mWaterIds;

        void updateAvoidShapeId(const ObjectId id, const ObjectId avoidId);
        void updateWaterShapeId(const ObjectId id, const ObjectId waterId);
        void updateId(const ObjectId id, const ObjectId waterId, std::unordered_map<ObjectId, ObjectId>& ids);
    };
}

#endif
