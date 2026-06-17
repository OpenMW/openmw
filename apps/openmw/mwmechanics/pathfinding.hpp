#ifndef GAME_MWMECHANICS_PATHFINDING_H
#define GAME_MWMECHANICS_PATHFINDING_H

#include <cassert>
#include <deque>
#include <iterator>
#include <span>

#include <osg/Vec3f>

#include <components/detournavigator/areatype.hpp>
#include <components/detournavigator/flags.hpp>
#include <components/detournavigator/status.hpp>

namespace MWWorld
{
    class CellStore;
    class ConstPtr;
    class Ptr;
}

namespace DetourNavigator
{
    struct AgentBounds;
}

namespace MWMechanics
{
    class PathgridGraph;

    template <class T>
    inline float distance(const T& lhs, const T& rhs)
    {
        static_assert(std::is_same<T, osg::Vec2f>::value || std::is_same<T, osg::Vec3f>::value, "T is not a position");
        return (lhs - rhs).length();
    }

    inline float distanceIgnoreZ(const osg::Vec3f& lhs, const osg::Vec3f& rhs)
    {
        return distance(osg::Vec2f(lhs.x(), lhs.y()), osg::Vec2f(rhs.x(), rhs.y()));
    }

    float getPathDistance(const MWWorld::Ptr& actor, const osg::Vec3f& lhs, const osg::Vec3f& rhs);

    inline float getZAngleToDir(const osg::Vec3f& dir)
    {
        return std::atan2(dir.x(), dir.y());
    }

    inline float getXAngleToDir(const osg::Vec3f& dir)
    {
        float dirLen = dir.length();
        return (dirLen != 0) ? -std::asin(dir.z() / dirLen) : 0;
    }

    inline float getZAngleToPoint(const osg::Vec3f& origin, const osg::Vec3f& dest)
    {
        return getZAngleToDir(dest - origin);
    }

    inline float getXAngleToPoint(const osg::Vec3f& origin, const osg::Vec3f& dest)
    {
        return getXAngleToDir(dest - origin);
    }

    const float PATHFIND_Z_REACH = 50.0f;
    // distance after which actor (failed previously to shortcut) will try again
    const float PATHFIND_SHORTCUT_RETRY_DIST = 300.0f;

    const float MIN_TOLERANCE = 1.0f;
    const float DEFAULT_TOLERANCE = 32.0f;

    // cast up-down ray with some offset from actor position to check for pits/obstacles on the way to target;
    // magnitude of pits/obstacles is defined by PATHFIND_Z_REACH
    bool checkWayIsClear(const osg::Vec3f& from, const osg::Vec3f& to, float offsetXY);

    enum class PathType
    {
        Full,
        Partial,
    };

    class PathFinder
    {
    public:
        using UpdateFlags = unsigned;

        enum UpdateFlag : UpdateFlags
        {
            UpdateFlag_CanMoveByZ = 1 << 0,
            UpdateFlag_ShortenIfAlmostStraight = 1 << 1,
            UpdateFlag_RemoveLoops = 1 << 2,
        };

        PathFinder() = default;

        void clearPath()
        {
            mConstructed = false;
            mPath.clear();
            mCell = nullptr;
        }

        void buildStraightPath(const osg::Vec3f& endPoint);

        void buildPathByNavMesh(const MWWorld::ConstPtr& actor, const osg::Vec3f& startPoint,
            const osg::Vec3f& endPoint, const DetourNavigator::AgentBounds& agentBounds,
            const DetourNavigator::Flags flags, const DetourNavigator::AreaCosts& areaCosts, float endTolerance,
            PathType pathType, std::span<const osg::Vec3f> checkpoints = {});

        void buildPath(const MWWorld::ConstPtr& actor, const osg::Vec3f& startPoint, const osg::Vec3f& endPoint,
            const PathgridGraph& pathgridGraph, const DetourNavigator::AgentBounds& agentBounds,
            const DetourNavigator::Flags flags, const DetourNavigator::AreaCosts& areaCosts, float endTolerance,
            PathType pathType, std::span<const osg::Vec3f> checkpoints = {});

        void buildLimitedPath(const MWWorld::ConstPtr& actor, const osg::Vec3f& startPoint, const osg::Vec3f& endPoint,
            const PathgridGraph& pathgridGraph, const DetourNavigator::AgentBounds& agentBounds,
            const DetourNavigator::Flags flags, const DetourNavigator::AreaCosts& areaCosts, float endTolerance,
            PathType pathType);

        /// Remove front point if exist and within tolerance
        void update(const osg::Vec3f& position, float pointTolerance, float destinationTolerance,
            UpdateFlags updateFlags, const DetourNavigator::AgentBounds& agentBounds, DetourNavigator::Flags pathFlags);

        bool checkPathCompleted() const { return mConstructed && mPath.empty(); }

        /// In radians
        float getZAngleToNext(float x, float y) const;

        float getXAngleToNext(float x, float y, float z) const;

        bool isPathConstructed() const { return mConstructed && !mPath.empty(); }

        std::size_t getPathSize() const { return mPath.size(); }

        const std::deque<osg::Vec3f>& getPath() const { return mPath; }

        const MWWorld::CellStore* getPathCell() const { return mCell; }

        void addPointToPath(const osg::Vec3f& point)
        {
            mConstructed = true;
            mPath.push_back(point);
        }

    private:
        bool mConstructed = false;
        std::deque<osg::Vec3f> mPath;
        const MWWorld::CellStore* mCell = nullptr;

        void buildPathByPathgridImpl(const osg::Vec3f& startPoint, const osg::Vec3f& endPoint,
            const PathgridGraph& pathgridGraph, std::back_insert_iterator<std::deque<osg::Vec3f>> out);

        [[nodiscard]] DetourNavigator::Status buildPathByNavigatorImpl(const MWWorld::ConstPtr& actor,
            const osg::Vec3f& startPoint, const osg::Vec3f& endPoint, const DetourNavigator::AgentBounds& agentBounds,
            const DetourNavigator::Flags flags, const DetourNavigator::AreaCosts& areaCosts, float endTolerance,
            PathType pathType, std::span<const osg::Vec3f> checkpoints,
            std::back_insert_iterator<std::deque<osg::Vec3f>> out);
    };
}

#endif
