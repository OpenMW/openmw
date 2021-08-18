#include "pathfinding.hpp"

#include <iterator>
#include <limits>

#include <components/detournavigator/debug.hpp>
#include <components/detournavigator/navigator.hpp>
#include <components/debug/debuglog.hpp>
#include <components/misc/coordinateconverter.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwphysics/collisiontype.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"

#include "pathgrid.hpp"
#include "actorutil.hpp"

namespace
{
    // Chooses a reachable end pathgrid point.  start is assumed reachable.
    std::pair<int, bool> getClosestReachablePoint(const ESM::Pathgrid* grid,
                                                  const MWMechanics::PathgridGraph *graph,
                                                  const osg::Vec3f& pos, int start)
    {
        assert(grid && !grid->mPoints.empty());

        float closestDistanceBetween = std::numeric_limits<float>::max();
        float closestDistanceReachable = std::numeric_limits<float>::max();
        int closestIndex = 0;
        int closestReachableIndex = 0;
        // TODO: if this full scan causes performance problems mapping pathgrid
        //       points to a quadtree may help
        for(unsigned int counter = 0; counter < grid->mPoints.size(); counter++)
        {
            float potentialDistBetween = MWMechanics::PathFinder::distanceSquared(grid->mPoints[counter], pos);
            if (potentialDistBetween < closestDistanceReachable)
            {
                // found a closer one
                if (graph->isPointConnected(start, counter))
                {
                    closestDistanceReachable = potentialDistBetween;
                    closestReachableIndex = counter;
                }
                if (potentialDistBetween < closestDistanceBetween)
                {
                    closestDistanceBetween = potentialDistBetween;
                    closestIndex = counter;
                }
            }
        }

        // post-condition: start and endpoint must be connected
        assert(graph->isPointConnected(start, closestReachableIndex));

        // AiWander has logic that depends on whether a path was created, deleting
        // allowed nodes if not.  Hence a path needs to be created even if the start
        // and the end points are the same.

        return std::pair<int, bool>
            (closestReachableIndex, closestReachableIndex == closestIndex);
    }

    float sqrDistance(const osg::Vec2f& lhs, const osg::Vec2f& rhs)
    {
        return (lhs - rhs).length2();
    }

    float sqrDistanceIgnoreZ(const osg::Vec3f& lhs, const osg::Vec3f& rhs)
    {
        return sqrDistance(osg::Vec2f(lhs.x(), lhs.y()), osg::Vec2f(rhs.x(), rhs.y()));
    }

    float getPathStepSize(const MWWorld::ConstPtr& actor)
    {
        const auto world = MWBase::Environment::get().getWorld();
        const auto realHalfExtents = world->getHalfExtents(actor);
        return 2 * std::max(realHalfExtents.x(), realHalfExtents.y());
    }

    float getHeight(const MWWorld::ConstPtr& actor)
    {
        const auto world = MWBase::Environment::get().getWorld();
        const auto halfExtents = world->getHalfExtents(actor);
        return 2.0 * halfExtents.z();
    }

    // Returns true if turn in `p2` is less than 10 degrees and all the 3 points are almost on one line.
    bool isAlmostStraight(const osg::Vec3f& p1, const osg::Vec3f& p2, const osg::Vec3f& p3, float pointTolerance) {
        osg::Vec3f v1 = p1 - p2;
        osg::Vec3f v3 = p3 - p2;
        v1.z() = v3.z() = 0;
        float dotProduct = v1.x() * v3.x() + v1.y() * v3.y();
        float crossProduct = v1.x() * v3.y() - v1.y() * v3.x();

        // Check that the angle between v1 and v3 is less or equal than 5 degrees.
        static const float cos175 = std::cos(osg::PI * (175.0 / 180));
        bool checkAngle = dotProduct <= cos175 * v1.length() * v3.length();

        // Check that distance from p2 to the line (p1, p3) is less or equal than `pointTolerance`.
        bool checkDist = std::abs(crossProduct) <= pointTolerance * (p3 - p1).length();

        return checkAngle && checkDist;
    }

    struct IsValidShortcut
    {
        const DetourNavigator::Navigator* mNavigator;
        const osg::Vec3f mHalfExtents;
        const DetourNavigator::Flags mFlags;

        bool operator()(const osg::Vec3f& start, const osg::Vec3f& end) const
        {
            const auto position = mNavigator->raycast(mHalfExtents, start, end, mFlags);
            return position.has_value() && std::abs((position.value() - start).length2() - (end - start).length2()) <= 1;
        }
    };
}

namespace MWMechanics
{
    float getPathDistance(const MWWorld::Ptr& actor, const osg::Vec3f& lhs, const osg::Vec3f& rhs)
    {
        if (std::abs(lhs.z() - rhs.z()) > getHeight(actor) || canActorMoveByZAxis(actor))
            return distance(lhs, rhs);
        return distanceIgnoreZ(lhs, rhs);
    }

    bool checkWayIsClear(const osg::Vec3f& from, const osg::Vec3f& to, float offsetXY)
    {
        osg::Vec3f dir = to - from;
        dir.z() = 0;
        dir.normalize();
        float verticalOffset = 200; // instead of '200' here we want the height of the actor
        osg::Vec3f _from = from + dir*offsetXY + osg::Z_AXIS * verticalOffset;

        // cast up-down ray and find height of hit in world space
        float h = _from.z() - MWBase::Environment::get().getWorld()->getDistToNearestRayHit(_from, -osg::Z_AXIS, verticalOffset + PATHFIND_Z_REACH + 1);

        return (std::abs(from.z() - h) <= PATHFIND_Z_REACH);
    }

    /*
     * NOTE: This method may fail to find a path.  The caller must check the
     * result before using it.  If there is no path the AI routies need to
     * implement some other heuristics to reach the target.
     *
     * NOTE: It may be desirable to simply go directly to the endPoint if for
     *       example there are no pathgrids in this cell.
     *
     * NOTE: startPoint & endPoint are in world coordinates
     *
     * Updates mPath using aStarSearch() or ray test (if shortcut allowed).
     * mPath consists of pathgrid points, except the last element which is
     * endPoint.  This may be useful where the endPoint is not on a pathgrid
     * point (e.g. combat).  However, if the caller has already chosen a
     * pathgrid point (e.g. wander) then it may be worth while to call
     * pop_back() to remove the redundant entry.
     *
     * NOTE: coordinates must be converted prior to calling getClosestPoint()
     *
     *    |
     *    |       cell
     *    |     +-----------+
     *    |     |           |
     *    |     |           |
     *    |     |      @    |
     *    |  i  |   j       |
     *    |<--->|<---->|    |
     *    |     +-----------+
     *    |   k
     *    |<---------->|         world
     *    +-----------------------------
     *
     *    i = x value of cell itself (multiply by ESM::Land::REAL_SIZE to convert)
     *    j = @.x in local coordinates (i.e. within the cell)
     *    k = @.x in world coordinates
     */
    void PathFinder::buildPathByPathgridImpl(const osg::Vec3f& startPoint, const osg::Vec3f& endPoint,
        const PathgridGraph& pathgridGraph, std::back_insert_iterator<std::deque<osg::Vec3f>> out)
    {
        const auto pathgrid = pathgridGraph.getPathgrid();

        // Refer to AiWander reseach topic on openmw forums for some background.
        // Maybe there is no pathgrid for this cell.  Just go to destination and let
        // physics take care of any blockages.
        if(!pathgrid || pathgrid->mPoints.empty())
            return;

        // NOTE: getClosestPoint expects local coordinates
        Misc::CoordinateConverter converter(mCell->getCell());

        // NOTE: It is possible that getClosestPoint returns a pathgrind point index
        //       that is unreachable in some situations. e.g. actor is standing
        //       outside an area enclosed by walls, but there is a pathgrid
        //       point right behind the wall that is closer than any pathgrid
        //       point outside the wall
        osg::Vec3f startPointInLocalCoords(converter.toLocalVec3(startPoint));
        int startNode = getClosestPoint(pathgrid, startPointInLocalCoords);

        osg::Vec3f endPointInLocalCoords(converter.toLocalVec3(endPoint));
        std::pair<int, bool> endNode = getClosestReachablePoint(pathgrid, &pathgridGraph,
            endPointInLocalCoords,
                startNode);

        // if it's shorter for actor to travel from start to end, than to travel from either
        // start or end to nearest pathgrid point, just travel from start to end.
        float startToEndLength2 = (endPointInLocalCoords - startPointInLocalCoords).length2();
        float endTolastNodeLength2 = distanceSquared(pathgrid->mPoints[endNode.first], endPointInLocalCoords);
        float startTo1stNodeLength2 = distanceSquared(pathgrid->mPoints[startNode], startPointInLocalCoords);
        if ((startToEndLength2 < startTo1stNodeLength2) || (startToEndLength2 < endTolastNodeLength2))
        {
            *out++ = endPoint;
            return;
        }

        // AiWander has logic that depends on whether a path was created,
        // deleting allowed nodes if not.  Hence a path needs to be created
        // even if the start and the end points are the same.
        // NOTE: aStarSearch will return an empty path if the start and end
        //       nodes are the same
        if(startNode == endNode.first)
        {
            ESM::Pathgrid::Point temp(pathgrid->mPoints[startNode]);
            converter.toWorld(temp);
            *out++ = makeOsgVec3(temp);
        }
        else
        {
            auto path = pathgridGraph.aStarSearch(startNode, endNode.first);

            // If nearest path node is in opposite direction from second, remove it from path.
            // Especially useful for wandering actors, if the nearest node is blocked for some reason.
            if (path.size() > 1)
            {
                ESM::Pathgrid::Point secondNode = *(++path.begin());
                osg::Vec3f firstNodeVec3f = makeOsgVec3(pathgrid->mPoints[startNode]);
                osg::Vec3f secondNodeVec3f = makeOsgVec3(secondNode);
                osg::Vec3f toSecondNodeVec3f = secondNodeVec3f - firstNodeVec3f;
                osg::Vec3f toStartPointVec3f = startPointInLocalCoords - firstNodeVec3f;
                if (toSecondNodeVec3f * toStartPointVec3f > 0)
                {
                    ESM::Pathgrid::Point temp(secondNode);
                    converter.toWorld(temp);
                    // Add Z offset since path node can overlap with other objects.
                    // Also ignore doors in raytesting.
                    const int mask = MWPhysics::CollisionType_World;
                    bool isPathClear = !MWBase::Environment::get().getWorld()->castRay(
                        startPoint.x(), startPoint.y(), startPoint.z() + 16, temp.mX, temp.mY, temp.mZ + 16, mask);
                    if (isPathClear)
                        path.pop_front();
                }
            }

            // convert supplied path to world coordinates
            std::transform(path.begin(), path.end(), out,
                [&] (ESM::Pathgrid::Point& point)
                {
                    converter.toWorld(point);
                    return makeOsgVec3(point);
                });
        }

        // If endNode found is NOT the closest PathGrid point to the endPoint,
        // assume endPoint is not reachable from endNode. In which case,
        // path ends at endNode.
        //
        // So only add the destination (which may be different to the closest
        // pathgrid point) when endNode was the closest point to endPoint.
        //
        // This logic can fail in the opposite situate, e.g. endPoint may
        // have been reachable but happened to be very close to an
        // unreachable pathgrid point.
        //
        // The AI routines will have to deal with such situations.
        if (endNode.second)
            *out++ = endPoint;
    }

    float PathFinder::getZAngleToNext(float x, float y) const
    {
        // This should never happen (programmers should have an if statement checking
        // isPathConstructed that prevents this call if otherwise).
        if(mPath.empty())
            return 0.;

        const auto& nextPoint = mPath.front();
        const float directionX = nextPoint.x() - x;
        const float directionY = nextPoint.y() - y;

        return std::atan2(directionX, directionY);
    }

    float PathFinder::getXAngleToNext(float x, float y, float z) const
    {
        // This should never happen (programmers should have an if statement checking
        // isPathConstructed that prevents this call if otherwise).
        if(mPath.empty())
            return 0.;

        const osg::Vec3f dir = mPath.front() - osg::Vec3f(x, y, z);

        return getXAngleToDir(dir);
    }

    void PathFinder::update(const osg::Vec3f& position, float pointTolerance, float destinationTolerance,
                            bool shortenIfAlmostStraight, bool canMoveByZ, const osg::Vec3f& halfExtents,
                            const DetourNavigator::Flags flags)
    {
        if (mPath.empty())
            return;

        while (mPath.size() > 1 && sqrDistanceIgnoreZ(mPath.front(), position) < pointTolerance * pointTolerance)
            mPath.pop_front();

        if (shortenIfAlmostStraight)
        {
            const IsValidShortcut isValidShortcut {
                MWBase::Environment::get().getWorld()->getNavigator(),
                halfExtents, flags
            };
            while (mPath.size() > 2 && isAlmostStraight(mPath[0], mPath[1], mPath[2], pointTolerance)
                   && isValidShortcut(mPath[0], mPath[2]))
                mPath.erase(mPath.begin() + 1);
            if (mPath.size() > 1 && isAlmostStraight(position, mPath[0], mPath[1], pointTolerance)
                    && isValidShortcut(position, mPath[1]))
                mPath.pop_front();
        }

        if (mPath.size() == 1)
        {
            float distSqr;
            if (canMoveByZ)
                distSqr = (mPath.front() - position).length2();
            else
                distSqr = sqrDistanceIgnoreZ(mPath.front(), position);
            if (distSqr < destinationTolerance * destinationTolerance)
                mPath.pop_front();
        }
    }

    void PathFinder::buildStraightPath(const osg::Vec3f& endPoint)
    {
        mPath.clear();
        mPath.push_back(endPoint);
        mConstructed = true;
    }

    void PathFinder::buildPathByPathgrid(const osg::Vec3f& startPoint, const osg::Vec3f& endPoint,
        const MWWorld::CellStore* cell, const PathgridGraph& pathgridGraph)
    {
        mPath.clear();
        mCell = cell;

        buildPathByPathgridImpl(startPoint, endPoint, pathgridGraph, std::back_inserter(mPath));

        mConstructed = !mPath.empty();
    }

    void PathFinder::buildPathByNavMesh(const MWWorld::ConstPtr& actor, const osg::Vec3f& startPoint,
        const osg::Vec3f& endPoint, const osg::Vec3f& halfExtents, const DetourNavigator::Flags flags,
        const DetourNavigator::AreaCosts& areaCosts)
    {
        mPath.clear();

        // If it's not possible to build path over navmesh due to disabled navmesh generation fallback to straight path
        DetourNavigator::Status status = buildPathByNavigatorImpl(actor, startPoint, endPoint, halfExtents, flags,
                                                                  areaCosts, std::back_inserter(mPath));

        if (status != DetourNavigator::Status::Success)
            mPath.clear();

        if (status == DetourNavigator::Status::NavMeshNotFound)
            mPath.push_back(endPoint);

        mConstructed = !mPath.empty();
    }

    void PathFinder::buildPath(const MWWorld::ConstPtr& actor, const osg::Vec3f& startPoint, const osg::Vec3f& endPoint,
        const MWWorld::CellStore* cell, const PathgridGraph& pathgridGraph, const osg::Vec3f& halfExtents,
        const DetourNavigator::Flags flags, const DetourNavigator::AreaCosts& areaCosts)
    {
        mPath.clear();
        mCell = cell;

        DetourNavigator::Status status = DetourNavigator::Status::NavMeshNotFound;

        if (!actor.getClass().isPureWaterCreature(actor) && !actor.getClass().isPureFlyingCreature(actor))
        {
            status = buildPathByNavigatorImpl(actor, startPoint, endPoint, halfExtents, flags, areaCosts, std::back_inserter(mPath));
            if (status != DetourNavigator::Status::Success)
                mPath.clear();
        }

        if (status != DetourNavigator::Status::NavMeshNotFound && mPath.empty())
        {
            status = buildPathByNavigatorImpl(actor, startPoint, endPoint, halfExtents,
                                     flags | DetourNavigator::Flag_usePathgrid, areaCosts, std::back_inserter(mPath));
            if (status != DetourNavigator::Status::Success)
                mPath.clear();
        }

        if (mPath.empty())
            buildPathByPathgridImpl(startPoint, endPoint, pathgridGraph, std::back_inserter(mPath));

        if (status == DetourNavigator::Status::NavMeshNotFound && mPath.empty())
            mPath.push_back(endPoint);

        mConstructed = !mPath.empty();
    }

    DetourNavigator::Status PathFinder::buildPathByNavigatorImpl(const MWWorld::ConstPtr& actor, const osg::Vec3f& startPoint,
        const osg::Vec3f& endPoint, const osg::Vec3f& halfExtents, const DetourNavigator::Flags flags,
        const DetourNavigator::AreaCosts& areaCosts, std::back_insert_iterator<std::deque<osg::Vec3f>> out)
    {
        const auto world = MWBase::Environment::get().getWorld();
        const auto stepSize = getPathStepSize(actor);
        const auto navigator = world->getNavigator();
        const auto status = navigator->findPath(halfExtents, stepSize, startPoint, endPoint, flags, areaCosts, out);

        if (status != DetourNavigator::Status::Success)
        {
            Log(Debug::Debug) << "Build path by navigator error: \"" << DetourNavigator::getMessage(status)
                << "\" for \"" << actor.getClass().getName(actor) << "\" (" << actor.getBase()
                << ") from " << startPoint << " to " << endPoint << " with flags ("
                << DetourNavigator::WriteFlags {flags} << ")";
        }

        return status;
    }

    void PathFinder::buildPathByNavMeshToNextPoint(const MWWorld::ConstPtr& actor, const osg::Vec3f& halfExtents,
        const DetourNavigator::Flags flags, const DetourNavigator::AreaCosts& areaCosts)
    {
        if (mPath.empty())
            return;

        const auto stepSize = getPathStepSize(actor);
        const auto startPoint = actor.getRefData().getPosition().asVec3();

        if (sqrDistanceIgnoreZ(mPath.front(), startPoint) <= 4 * stepSize * stepSize)
            return;

        const auto navigator = MWBase::Environment::get().getWorld()->getNavigator();
        std::deque<osg::Vec3f> prePath;
        auto prePathInserter = std::back_inserter(prePath);
        const auto status = navigator->findPath(halfExtents, stepSize, startPoint, mPath.front(), flags, areaCosts,
                                                prePathInserter);

        if (status == DetourNavigator::Status::NavMeshNotFound)
            return;

        if (status != DetourNavigator::Status::Success)
        {
            Log(Debug::Debug) << "Build path by navigator error: \"" << DetourNavigator::getMessage(status)
                << "\" for \"" << actor.getClass().getName(actor) << "\" (" << actor.getBase()
                << ") from " << startPoint << " to " << mPath.front() << " with flags ("
                << DetourNavigator::WriteFlags {flags} << ")";
            return;
        }

        while (!prePath.empty() && sqrDistanceIgnoreZ(prePath.front(), startPoint) < stepSize * stepSize)
            prePath.pop_front();

        while (!prePath.empty() && sqrDistanceIgnoreZ(prePath.back(), mPath.front()) < stepSize * stepSize)
            prePath.pop_back();

        std::copy(prePath.rbegin(), prePath.rend(), std::front_inserter(mPath));
    }

    void PathFinder::buildLimitedPath(const MWWorld::ConstPtr& actor, const osg::Vec3f& startPoint, const osg::Vec3f& endPoint,
        const MWWorld::CellStore* cell, const PathgridGraph& pathgridGraph, const osg::Vec3f& halfExtents,
        const DetourNavigator::Flags flags, const DetourNavigator::AreaCosts& areaCosts)
    {
        const auto navigator = MWBase::Environment::get().getWorld()->getNavigator();
        const auto maxDistance = std::min(
            navigator->getMaxNavmeshAreaRealRadius(),
            static_cast<float>(Constants::CellSizeInUnits)
        );
        const auto startToEnd = endPoint - startPoint;
        const auto distance = startToEnd.length();
        if (distance <= maxDistance)
            return buildPath(actor, startPoint, endPoint, cell, pathgridGraph, halfExtents, flags, areaCosts);
        const auto end = startPoint + startToEnd * maxDistance / distance;
        buildPath(actor, startPoint, end, cell, pathgridGraph, halfExtents, flags, areaCosts);
    }
}
