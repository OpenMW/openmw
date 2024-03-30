#ifndef GAME_MWMECHANICS_PATHFINDING_H
#define GAME_MWMECHANICS_PATHFINDING_H

#include <cassert>
#include <deque>
#include <iterator>

#include <components/detournavigator/areatype.hpp>
#include <components/detournavigator/flags.hpp>
#include <components/detournavigator/status.hpp>
#include <components/esm/position.hpp>
#include <components/esm3/loadpgrd.hpp>

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

        void buildPathByPathgrid(const osg::Vec3f& startPoint, const osg::Vec3f& endPoint,
            const MWWorld::CellStore* cell, const PathgridGraph& pathgridGraph);

        void buildPathByNavMesh(const MWWorld::ConstPtr& actor, const osg::Vec3f& startPoint,
            const osg::Vec3f& endPoint, const DetourNavigator::AgentBounds& agentBounds,
            const DetourNavigator::Flags flags, const DetourNavigator::AreaCosts& areaCosts, float endTolerance,
            PathType pathType);

        void buildPath(const MWWorld::ConstPtr& actor, const osg::Vec3f& startPoint, const osg::Vec3f& endPoint,
            const MWWorld::CellStore* cell, const PathgridGraph& pathgridGraph,
            const DetourNavigator::AgentBounds& agentBounds, const DetourNavigator::Flags flags,
            const DetourNavigator::AreaCosts& areaCosts, float endTolerance, PathType pathType);

        void buildLimitedPath(const MWWorld::ConstPtr& actor, const osg::Vec3f& startPoint, const osg::Vec3f& endPoint,
            const MWWorld::CellStore* cell, const PathgridGraph& pathgridGraph,
            const DetourNavigator::AgentBounds& agentBounds, const DetourNavigator::Flags flags,
            const DetourNavigator::AreaCosts& areaCosts, float endTolerance, PathType pathType);

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

        /// utility function to convert a osg::Vec3f to a Pathgrid::Point
        static ESM::Pathgrid::Point makePathgridPoint(const osg::Vec3f& v)
        {
            return ESM::Pathgrid::Point(static_cast<int>(v[0]), static_cast<int>(v[1]), static_cast<int>(v[2]));
        }

        /// utility function to convert an ESM::Position to a Pathgrid::Point
        static ESM::Pathgrid::Point makePathgridPoint(const ESM::Position& p)
        {
            return ESM::Pathgrid::Point(
                static_cast<int>(p.pos[0]), static_cast<int>(p.pos[1]), static_cast<int>(p.pos[2]));
        }

        static osg::Vec3f makeOsgVec3(const ESM::Pathgrid::Point& p)
        {
            return osg::Vec3f(static_cast<float>(p.mX), static_cast<float>(p.mY), static_cast<float>(p.mZ));
        }

        // Slightly cheaper version for comparisons.
        // Caller needs to be careful for very short distances (i.e. less than 1)
        // or when accumuating the results i.e. (a + b)^2 != a^2 + b^2
        //
        static float distanceSquared(const ESM::Pathgrid::Point& point, const osg::Vec3f& pos)
        {
            return (MWMechanics::PathFinder::makeOsgVec3(point) - pos).length2();
        }

        // Return the closest pathgrid point index from the specified position
        // coordinates.  NOTE: Does not check if there is a sensible way to get there
        // (e.g. a cliff in front).
        //
        // NOTE: pos is expected to be in local coordinates, as is grid->mPoints
        //
        static int getClosestPoint(const ESM::Pathgrid* grid, const osg::Vec3f& pos)
        {
            assert(grid && !grid->mPoints.empty());

            float distanceBetween = distanceSquared(grid->mPoints[0], pos);
            int closestIndex = 0;

            // TODO: if this full scan causes performance problems mapping pathgrid
            //       points to a quadtree may help
            for (unsigned int counter = 1; counter < grid->mPoints.size(); counter++)
            {
                float potentialDistBetween = distanceSquared(grid->mPoints[counter], pos);
                if (potentialDistBetween < distanceBetween)
                {
                    distanceBetween = potentialDistBetween;
                    closestIndex = counter;
                }
            }

            return closestIndex;
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
            PathType pathType, std::back_insert_iterator<std::deque<osg::Vec3f>> out);
    };
}

#endif
