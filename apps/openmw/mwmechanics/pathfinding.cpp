#include "pathfinding.hpp"

#include <limits>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwphysics/collisiontype.hpp"

#include "../mwworld/cellstore.hpp"

#include "pathgrid.hpp"
#include "coordinateconverter.hpp"

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
            float potentialDistBetween = MWMechanics::PathFinder::DistanceSquared(grid->mPoints[counter], pos);
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

}

namespace MWMechanics
{
    float sqrDistanceIgnoreZ(const ESM::Pathgrid::Point& point, float x, float y)
    {
        x -= point.mX;
        y -= point.mY;
        return (x * x + y * y);
    }

    float distance(const ESM::Pathgrid::Point& point, float x, float y, float z)
    {
        x -= point.mX;
        y -= point.mY;
        z -= point.mZ;
        return sqrt(x * x + y * y + z * z);
    }

    float distance(const ESM::Pathgrid::Point& a, const ESM::Pathgrid::Point& b)
    {
        float x = static_cast<float>(a.mX - b.mX);
        float y = static_cast<float>(a.mY - b.mY);
        float z = static_cast<float>(a.mZ - b.mZ);
        return sqrt(x * x + y * y + z * z);
    }

    float getZAngleToDir(const osg::Vec3f& dir)
    {
        return std::atan2(dir.x(), dir.y());
    }

    float getXAngleToDir(const osg::Vec3f& dir)
    {
        float dirLen = dir.length();
        return (dirLen != 0) ? -std::asin(dir.z() / dirLen) : 0;
    }

    float getZAngleToPoint(const ESM::Pathgrid::Point &origin, const ESM::Pathgrid::Point &dest)
    {
        osg::Vec3f dir = PathFinder::MakeOsgVec3(dest) - PathFinder::MakeOsgVec3(origin);
        return getZAngleToDir(dir);
    }

    float getXAngleToPoint(const ESM::Pathgrid::Point &origin, const ESM::Pathgrid::Point &dest)
    {
        osg::Vec3f dir = PathFinder::MakeOsgVec3(dest) - PathFinder::MakeOsgVec3(origin);
        return getXAngleToDir(dir);
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

    PathFinder::PathFinder()
        : mPathgrid(nullptr)
        , mCell(nullptr)
    {
    }

    void PathFinder::clearPath()
    {
        if(!mPath.empty())
            mPath.clear();
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
     * NOTE: coordinates must be converted prior to calling GetClosestPoint()
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
    void PathFinder::buildPath(const ESM::Pathgrid::Point &startPoint,
                               const ESM::Pathgrid::Point &endPoint,
                               const MWWorld::CellStore* cell, const PathgridGraph& pathgridGraph)
    {
        mPath.clear();

        // TODO: consider removing mCell / mPathgrid in favor of mPathgridGraph
        if(mCell != cell || !mPathgrid)
        {
            mCell = cell;
            mPathgrid = pathgridGraph.getPathgrid();
        }

        // Refer to AiWander reseach topic on openmw forums for some background.
        // Maybe there is no pathgrid for this cell.  Just go to destination and let
        // physics take care of any blockages.
        if(!mPathgrid || mPathgrid->mPoints.empty())
        {
            mPath.push_back(endPoint);
            return;
        }

        // NOTE: GetClosestPoint expects local coordinates
        CoordinateConverter converter(mCell->getCell());

        // NOTE: It is possible that GetClosestPoint returns a pathgrind point index
        //       that is unreachable in some situations. e.g. actor is standing
        //       outside an area enclosed by walls, but there is a pathgrid
        //       point right behind the wall that is closer than any pathgrid
        //       point outside the wall
        osg::Vec3f startPointInLocalCoords(converter.toLocalVec3(startPoint));
        int startNode = GetClosestPoint(mPathgrid, startPointInLocalCoords);

        osg::Vec3f endPointInLocalCoords(converter.toLocalVec3(endPoint));
        std::pair<int, bool> endNode = getClosestReachablePoint(mPathgrid, &pathgridGraph,
            endPointInLocalCoords,
                startNode);

        // if it's shorter for actor to travel from start to end, than to travel from either
        // start or end to nearest pathgrid point, just travel from start to end.
        float startToEndLength2 = (endPointInLocalCoords - startPointInLocalCoords).length2();
        float endTolastNodeLength2 = DistanceSquared(mPathgrid->mPoints[endNode.first], endPointInLocalCoords);
        float startTo1stNodeLength2 = DistanceSquared(mPathgrid->mPoints[startNode], startPointInLocalCoords);
        if ((startToEndLength2 < startTo1stNodeLength2) || (startToEndLength2 < endTolastNodeLength2))
        {
            mPath.push_back(endPoint);
            return;
        }

        // AiWander has logic that depends on whether a path was created,
        // deleting allowed nodes if not.  Hence a path needs to be created
        // even if the start and the end points are the same.
        // NOTE: aStarSearch will return an empty path if the start and end
        //       nodes are the same
        if(startNode == endNode.first)
        {
            ESM::Pathgrid::Point temp(mPathgrid->mPoints[startNode]);
            converter.toWorld(temp);
            mPath.push_back(temp);
        }
        else
        {
            mPath = pathgridGraph.aStarSearch(startNode, endNode.first);

            // If nearest path node is in opposite direction from second, remove it from path.
            // Especially useful for wandering actors, if the nearest node is blocked for some reason.
            if (mPath.size() > 1)
            {
                ESM::Pathgrid::Point secondNode = *(++mPath.begin());
                osg::Vec3f firstNodeVec3f = MakeOsgVec3(mPathgrid->mPoints[startNode]);
                osg::Vec3f secondNodeVec3f = MakeOsgVec3(secondNode);
                osg::Vec3f toSecondNodeVec3f = secondNodeVec3f - firstNodeVec3f;
                osg::Vec3f toStartPointVec3f = startPointInLocalCoords - firstNodeVec3f;
                if (toSecondNodeVec3f * toStartPointVec3f > 0)
                {
                    ESM::Pathgrid::Point temp(secondNode);
                    converter.toWorld(temp);
                    // Add Z offset since path node can overlap with other objects.
                    // Also ignore doors in raytesting.
                    int mask = MWPhysics::CollisionType_World;
                    bool isPathClear = !MWBase::Environment::get().getWorld()->castRay(
                        startPoint.mX, startPoint.mY, startPoint.mZ+16, temp.mX, temp.mY, temp.mZ+16, mask);
                    if (isPathClear)
                        mPath.pop_front();
                }
            }

            // convert supplied path to world coordinates
            for (std::list<ESM::Pathgrid::Point>::iterator iter(mPath.begin()); iter != mPath.end(); ++iter)
            {
                converter.toWorld(*iter);
            }
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
        if(endNode.second)
            mPath.push_back(endPoint);
    }

    float PathFinder::getZAngleToNext(float x, float y) const
    {
        // This should never happen (programmers should have an if statement checking
        // isPathConstructed that prevents this call if otherwise).
        if(mPath.empty())
            return 0.;

        const ESM::Pathgrid::Point &nextPoint = *mPath.begin();
        float directionX = nextPoint.mX - x;
        float directionY = nextPoint.mY - y;

        return std::atan2(directionX, directionY);
    }

    float PathFinder::getXAngleToNext(float x, float y, float z) const
    {
        // This should never happen (programmers should have an if statement checking
        // isPathConstructed that prevents this call if otherwise).
        if(mPath.empty())
            return 0.;

        const ESM::Pathgrid::Point &nextPoint = *mPath.begin();
        osg::Vec3f dir = MakeOsgVec3(nextPoint) - osg::Vec3f(x,y,z);

        return getXAngleToDir(dir);
    }

    bool PathFinder::checkPathCompleted(float x, float y, float tolerance)
    {
        if(mPath.empty())
            return true;

        const ESM::Pathgrid::Point& nextPoint = *mPath.begin();
        if (sqrDistanceIgnoreZ(nextPoint, x, y) < tolerance*tolerance)
        {
            mPath.pop_front();
            if(mPath.empty())
            {
                return true;
            }
        }

        return false;
    }

    // see header for the rationale
    void PathFinder::buildSyncedPath(const ESM::Pathgrid::Point &startPoint,
        const ESM::Pathgrid::Point &endPoint,
        const MWWorld::CellStore* cell, const MWMechanics::PathgridGraph& pathgridGraph)
    {
        if (mPath.size() < 2)
        {
            // if path has one point, then it's the destination.
            // don't need to worry about bad path for this case
            buildPath(startPoint, endPoint, cell, pathgridGraph);
        }
        else
        {
            const ESM::Pathgrid::Point oldStart(*getPath().begin());
            buildPath(startPoint, endPoint, cell, pathgridGraph);
            if (mPath.size() >= 2)
            {
                // if 2nd waypoint of new path == 1st waypoint of old,
                // delete 1st waypoint of new path.
                std::list<ESM::Pathgrid::Point>::iterator iter = ++mPath.begin();
                if (iter->mX == oldStart.mX
                    && iter->mY == oldStart.mY
                    && iter->mZ == oldStart.mZ)
                {
                    mPath.pop_front();
                }
            }
        }
    }

    const MWWorld::CellStore* PathFinder::getPathCell() const
    {
        return mCell;
    }
}
