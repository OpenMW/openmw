#include "pathfinding.hpp"

#include "OgreMath.h"
#include "OgreVector3.h"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"

namespace
{
    // Slightly cheaper version for comparisons.
    // Caller needs to be careful for very short distances (i.e. less than 1)
    // or when accumuating the results i.e. (a + b)^2 != a^2 + b^2
    //
    float distanceSquared(ESM::Pathgrid::Point point, Ogre::Vector3 pos)
    {
        return MWMechanics::PathFinder::MakeOgreVector3(point).squaredDistance(pos);
    }

    // Return the closest pathgrid point index from the specified position co
    // -ordinates.  NOTE: Does not check if there is a sensible way to get there
    // (e.g. a cliff in front).
    //
    // NOTE: pos is expected to be in local co-ordinates, as is grid->mPoints
    //
    int getClosestPoint(const ESM::Pathgrid* grid, Ogre::Vector3 pos)
    {
        if(!grid || grid->mPoints.empty())
            return -1;

        float distanceBetween = distanceSquared(grid->mPoints[0], pos);
        int closestIndex = 0;

        // TODO: if this full scan causes performance problems mapping pathgrid
        //       points to a quadtree may help
        for(unsigned int counter = 1; counter < grid->mPoints.size(); counter++)
        {
            float potentialDistBetween = distanceSquared(grid->mPoints[counter], pos);
            if(potentialDistBetween < distanceBetween)
            {
                distanceBetween = potentialDistBetween;
                closestIndex = counter;
            }
        }

        return closestIndex;
    }

    // Chooses a reachable end pathgrid point.  start is assumed reachable.
    std::pair<int, bool> getClosestReachablePoint(const ESM::Pathgrid* grid,
                                                  const MWWorld::CellStore *cell,
                                                  Ogre::Vector3 pos, int start)
    {
        if(!grid || grid->mPoints.empty())
            return std::pair<int, bool> (-1, false);

        float distanceBetween = distanceSquared(grid->mPoints[0], pos);
        int closestIndex = 0;
        int closestReachableIndex = 0;
        // TODO: if this full scan causes performance problems mapping pathgrid
        //       points to a quadtree may help
        for(unsigned int counter = 1; counter < grid->mPoints.size(); counter++)
        {
            float potentialDistBetween = distanceSquared(grid->mPoints[counter], pos);
            if(potentialDistBetween < distanceBetween)
            {
                // found a closer one
                distanceBetween = potentialDistBetween;
                closestIndex = counter;
                if (cell->isPointConnected(start, counter))
                {
                    closestReachableIndex = counter;
                }
            }
        }
        // AiWander has logic that depends on whether a path was created, deleting
        // allowed nodes if not.  Hence a path needs to be created even if the start
        // and the end points are the same.
        //if(start == closestReachableIndex)
            //closestReachableIndex = -1; // couldn't find anyting other than start

        return std::pair<int, bool>
            (closestReachableIndex, closestReachableIndex == closestIndex);
    }

}

namespace MWMechanics
{
    float sqrDistanceIgnoreZ(ESM::Pathgrid::Point point, float x, float y)
    {
        x -= point.mX;
        y -= point.mY;
        return (x * x + y * y);
    }

    float distance(ESM::Pathgrid::Point point, float x, float y, float z)
    {
        x -= point.mX;
        y -= point.mY;
        z -= point.mZ;
        return sqrt(x * x + y * y + z * z);
    }

    float distance(ESM::Pathgrid::Point a, ESM::Pathgrid::Point b)
    {
        float x = static_cast<float>(a.mX - b.mX);
        float y = static_cast<float>(a.mY - b.mY);
        float z = static_cast<float>(a.mZ - b.mZ);
        return sqrt(x * x + y * y + z * z);
    }

    PathFinder::PathFinder()
        : mIsPathConstructed(false),
          mPathgrid(NULL),
          mCell(NULL)
    {
    }

    void PathFinder::clearPath()
    {
        if(!mPath.empty())
            mPath.clear();
        mIsPathConstructed = false;
    }

    /*
     * NOTE: This method may fail to find a path.  The caller must check the
     * result before using it.  If there is no path the AI routies need to
     * implement some other heuristics to reach the target.
     *
     * NOTE: It may be desirable to simply go directly to the endPoint if for
     *       example there are no pathgrids in this cell.
     *
     * NOTE: startPoint & endPoint are in world co-ordinates
     *
     * Updates mPath using aStarSearch() or ray test (if shortcut allowed).
     * mPath consists of pathgrid points, except the last element which is
     * endPoint.  This may be useful where the endPoint is not on a pathgrid
     * point (e.g. combat).  However, if the caller has already chosen a
     * pathgrid point (e.g. wander) then it may be worth while to call
     * pop_back() to remove the redundant entry.
     *
     * mPathConstructed is set true if successful, false if not
     *
     * NOTE: co-ordinates must be converted prior to calling getClosestPoint()
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
     *    j = @.x in local co-ordinates (i.e. within the cell)
     *    k = @.x in world co-ordinates
     */
    void PathFinder::buildPath(const ESM::Pathgrid::Point &startPoint,
                               const ESM::Pathgrid::Point &endPoint,
                               const MWWorld::CellStore* cell,
                               bool allowShortcuts)
    {
        mPath.clear();

        if(allowShortcuts)
        {
            // if there's a ray cast hit, can't take a direct path
            if (!MWBase::Environment::get().getWorld()->castRay(
                static_cast<float>(startPoint.mX), static_cast<float>(startPoint.mY), static_cast<float>(startPoint.mZ),
                static_cast<float>(endPoint.mX), static_cast<float>(endPoint.mY), static_cast<float>(endPoint.mZ)))
            {
                mPath.push_back(endPoint);
                mIsPathConstructed = true;
                return;
            }
        }

        if(mCell != cell || !mPathgrid)
        {
            mCell = cell;
            mPathgrid = MWBase::Environment::get().getWorld()->getStore().get<ESM::Pathgrid>().search(*mCell->getCell());
        }

        // Refer to AiWander reseach topic on openmw forums for some background.
        // Maybe there is no pathgrid for this cell.  Just go to destination and let
        // physics take care of any blockages.
        if(!mPathgrid || mPathgrid->mPoints.empty())
        {
            mPath.push_back(endPoint);
            mIsPathConstructed = true;
            return;
        }

        // NOTE: getClosestPoint expects local co-ordinates
        float xCell = 0;
        float yCell = 0;
        if (mCell->isExterior())
        {
            xCell = static_cast<float>(mCell->getCell()->mData.mX * ESM::Land::REAL_SIZE);
            yCell = static_cast<float>(mCell->getCell()->mData.mY * ESM::Land::REAL_SIZE);
        }

        // NOTE: It is possible that getClosestPoint returns a pathgrind point index
        //       that is unreachable in some situations. e.g. actor is standing
        //       outside an area enclosed by walls, but there is a pathgrid
        //       point right behind the wall that is closer than any pathgrid
        //       point outside the wall
        int startNode = getClosestPoint(mPathgrid,
                Ogre::Vector3(startPoint.mX - xCell, startPoint.mY - yCell, static_cast<float>(startPoint.mZ)));
        // Some cells don't have any pathgrids at all
        if(startNode != -1)
        {
            std::pair<int, bool> endNode = getClosestReachablePoint(mPathgrid, cell,
                Ogre::Vector3(endPoint.mX - xCell, endPoint.mY - yCell, static_cast<float>(endPoint.mZ)),
                    startNode);

            // this shouldn't really happen, but just in case
            if(endNode.first != -1)
            {
                // AiWander has logic that depends on whether a path was created,
                // deleting allowed nodes if not.  Hence a path needs to be created
                // even if the start and the end points are the same.
                // NOTE: aStarSearch will return an empty path if the start and end
                //       nodes are the same
                if(startNode == endNode.first)
                {
                    mPath.push_back(endPoint);
                    mIsPathConstructed = true;
                    return;
                }

                mPath = mCell->aStarSearch(startNode, endNode.first);

                if(!mPath.empty())
                {
                    mIsPathConstructed = true;
                    // Add the destination (which may be different to the closest
                    // pathgrid point).  However only add if endNode was the closest
                    // point to endPoint.
                    //
                    // This logic can fail in the opposite situate, e.g. endPoint may
                    // have been reachable but happened to be very close to an
                    // unreachable pathgrid point.
                    //
                    // The AI routines will have to deal with such situations.
                    if(endNode.second)
                        mPath.push_back(endPoint);
                }
                else
                    mIsPathConstructed = false;
            }
            else
                mIsPathConstructed = false;
        }
        else
            mIsPathConstructed = false;

        return;
    }

    float PathFinder::getZAngleToNext(float x, float y) const
    {
        // This should never happen (programmers should have an if statement checking
        // mIsPathConstructed that prevents this call if otherwise).
        if(mPath.empty())
            return 0.;

        const ESM::Pathgrid::Point &nextPoint = *mPath.begin();
        float directionX = nextPoint.mX - x;
        float directionY = nextPoint.mY - y;

        return Ogre::Math::ATan2(directionX,directionY).valueDegrees();
    }

    bool PathFinder::checkPathCompleted(float x, float y, float tolerance)
    {
        if(mPath.empty())
            return true;

        ESM::Pathgrid::Point nextPoint = *mPath.begin();
        if (sqrDistanceIgnoreZ(nextPoint, x, y) < tolerance*tolerance)
        {
            mPath.pop_front();
            if(mPath.empty())
            {
                mIsPathConstructed = false;
                return true;
            }
        }

        return false;
    }

    // used by AiCombat, see header for the rationale
    bool PathFinder::syncStart(const std::list<ESM::Pathgrid::Point> &path)
    {
        if (mPath.size() < 2)
            return false; //nothing to pop

        std::list<ESM::Pathgrid::Point>::const_iterator oldStart = path.begin();
        std::list<ESM::Pathgrid::Point>::iterator iter = ++mPath.begin();

        if(    (*iter).mX == oldStart->mX
            && (*iter).mY == oldStart->mY
            && (*iter).mZ == oldStart->mZ)
        {
            mPath.pop_front();
            return true;
        }
        return false;
    }

}
