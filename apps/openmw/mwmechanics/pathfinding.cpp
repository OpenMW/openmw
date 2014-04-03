#include "pathfinding.hpp"

#include <map>

#include "OgreMath.h"
#include "OgreVector3.h"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"

namespace
{
    float distanceZCorrected(ESM::Pathgrid::Point point, float x, float y, float z)
    {
        x -= point.mX;
        y -= point.mY;
        z -= point.mZ;
        return sqrt(x * x + y * y + 0.1 * z * z);
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
        float x = a.mX - b.mX;
        float y = a.mY - b.mY;
        float z = a.mZ - b.mZ;
        return sqrt(x * x + y * y + z * z);
    }

    // See http://theory.stanford.edu/~amitp/GameProgramming/Heuristics.html
    //
    // One of the smallest cost in Seyda Neen is between points 77 & 78:
    // pt      x     y
    // 77 = 8026, 4480
    // 78 = 7986, 4218
    //
    // Euclidean distance is about 262 (ignoring z) and Manhattan distance is 300
    // (again ignoring z).  Using a value of about 300 for D seems like a reasonable
    // starting point for experiments. If in doubt, just use value 1.
    //
    // The distance between 3 & 4 are pretty small, too.
    // 3 = 5435, 223
    // 4 = 5948, 193
    //
    // Approx. 514 Euclidean distance and 533 Manhattan distance.
    //
    float manhattan(const ESM::Pathgrid::Point a, const ESM::Pathgrid::Point b)
    {
        return 300 * (abs(a.mX - b.mX) + abs(a.mY - b.mY) + abs(a.mZ - b.mZ));
    }

    // Choose a heuristics - Note that these may not be the best for directed
    // graphs with non-uniform edge costs.
    //
    //   distance:
    //   - sqrt((curr.x - goal.x)^2 + (curr.y - goal.y)^2 + (curr.z - goal.z)^2)
    //   - slower but more accurate
    //
    //   Manhattan:
    //   - |curr.x - goal.x| + |curr.y - goal.y| + |curr.z - goal.z|
    //   - faster but not the shortest path
    float costAStar(const ESM::Pathgrid::Point a, const ESM::Pathgrid::Point b)
    {
        //return distance(a, b);
        return manhattan(a, b);
    }

    // Slightly cheaper version for comparisons.
    // Caller needs to be careful for very short distances (i.e. less than 1)
    // or when accumuating the results i.e. (a + b)^2 != a^2 + b^2
    //
    float distanceSquared(ESM::Pathgrid::Point point, Ogre::Vector3 pos)
    {
        return Ogre::Vector3(point.mX, point.mY, point.mZ).squaredDistance(pos);
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
        if(start == closestReachableIndex)
            closestReachableIndex = -1; // couldn't find anyting other than start

        return std::pair<int, bool>
            (closestReachableIndex, closestReachableIndex == closestIndex);
    }

}

namespace MWMechanics
{
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
            if(!MWBase::Environment::get().getWorld()->castRay(startPoint.mX, startPoint.mY, startPoint.mZ,
                                                               endPoint.mX, endPoint.mY, endPoint.mZ))
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
            xCell = mCell->getCell()->mData.mX * ESM::Land::REAL_SIZE;
            yCell = mCell->getCell()->mData.mY * ESM::Land::REAL_SIZE;
        }

        // NOTE: It is possible that getClosestPoint returns a pathgrind point index
        //       that is unreachable in some situations. e.g. actor is standing
        //       outside an area enclosed by walls, but there is a pathgrid
        //       point right behind the wall that is closer than any pathgrid
        //       point outside the wall
        int startNode = getClosestPoint(mPathgrid,
                Ogre::Vector3(startPoint.mX - xCell, startPoint.mY - yCell, startPoint.mZ));
        // Some cells don't have any pathgrids at all
        if(startNode != -1)
        {
            std::pair<int, bool> endNode = getClosestReachablePoint(mPathgrid, cell,
                    Ogre::Vector3(endPoint.mX - xCell, endPoint.mY - yCell, endPoint.mZ),
                    startNode);

            // this shouldn't really happen, but just in case
            if(endNode.first != -1)
            {
                mPath = mCell->aStarSearch(startNode, endNode.first, mCell->isExterior());

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
        float directionResult = sqrt(directionX * directionX + directionY * directionY);

        return Ogre::Radian(Ogre::Math::ACos(directionY / directionResult) * sgn(Ogre::Math::ASin(directionX / directionResult))).valueDegrees();
    }

    // Used by AiCombat, use Euclidean distance
    float PathFinder::getDistToNext(float x, float y, float z)
    {
        ESM::Pathgrid::Point nextPoint = *mPath.begin();
        return distance(nextPoint, x, y, z);
    }

    bool PathFinder::checkWaypoint(float x, float y, float z)
    {
        if(mPath.empty())
            return true;

        ESM::Pathgrid::Point nextPoint = *mPath.begin();
        if(distanceZCorrected(nextPoint, x, y, z) < 64)
        {
            mPath.pop_front();
            if(mPath.empty()) mIsPathConstructed = false;
            return true;
        }
        return false;
    }

    bool PathFinder::checkPathCompleted(float x, float y, float z)
    {
        if(mPath.empty())
            return true;

        ESM::Pathgrid::Point nextPoint = *mPath.begin();
        if(distanceZCorrected(nextPoint, x, y, z) < 64)
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
    void PathFinder::syncStart(const std::list<ESM::Pathgrid::Point> &path)
    {
        if (mPath.size() < 2)
            return; //nothing to pop
        std::list<ESM::Pathgrid::Point>::const_iterator oldStart = path.begin();
        std::list<ESM::Pathgrid::Point>::iterator iter = ++mPath.begin();

        if(    (*iter).mX == oldStart->mX
            && (*iter).mY == oldStart->mY
            && (*iter).mZ == oldStart->mZ
            && (*iter).mAutogenerated == oldStart->mAutogenerated
            && (*iter).mConnectionNum == oldStart->mConnectionNum )
        {
            mPath.pop_front();
        }

    }

    // TODO: Any multi threading concerns?
    PathgridGraph::PathgridGraph()
        : mCell(NULL)
        , mIsGraphConstructed(false)
        , mPathgrid(NULL)
        , mGraph(0)
        , mSCCId(0)
        , mSCCIndex(0)
    {
    }

    /*
     * mGraph is populated with the cost of each allowed edge.
     *
     * The data structure is based on the code in buildPath2() but modified.
     * Please check git history if interested.
     *
     * mGraph[v].edges[i].index = w
     *
     *   v = point index of location "from"
     *   i = index of edges from point v
     *   w = point index of location "to"
     *
     *
     * Example: (notice from p(0) to p(2) is not allowed in this example)
     *
     *   mGraph[0].edges[0].index = 1
     *            .edges[1].index = 3
     *
     *   mGraph[1].edges[0].index = 0
     *            .edges[1].index = 2
     *            .edges[2].index = 3
     *
     *   mGraph[2].edges[0].index = 1
     *
     *   (etc, etc)
     *
     *
     *        low
     *        cost
     *   p(0) <---> p(1) <------------> p(2)
     *    ^          ^
     *    |          |
     *    |          +-----> p(3)
     *    +---------------->
     *      high cost
     */
    bool PathgridGraph::initPathgridGraph(const ESM::Cell* cell)
    {
        if(!cell)
            return false;

        mCell = cell;
        mPathgrid = MWBase::Environment::get().getWorld()->getStore().get<ESM::Pathgrid>().search(*cell);

        if(!mPathgrid)
            return false;

        mGraph.resize(mPathgrid->mPoints.size());
        for(int i = 0; i < static_cast<int> (mPathgrid->mEdges.size()); i++)
        {
            ConnectedPoint neighbour;
            neighbour.cost = costAStar(mPathgrid->mPoints[mPathgrid->mEdges[i].mV0],
                                       mPathgrid->mPoints[mPathgrid->mEdges[i].mV1]);
            // forward path of the edge
            neighbour.index = mPathgrid->mEdges[i].mV1;
            mGraph[mPathgrid->mEdges[i].mV0].edges.push_back(neighbour);
            // reverse path of the edge
            // NOTE: These are redundant, ESM already contains the required reverse paths
            //neighbour.index = mPathgrid->mEdges[i].mV0;
            //mGraph[mPathgrid->mEdges[i].mV1].edges.push_back(neighbour);
        }
        buildConnectedPoints();
        mIsGraphConstructed = true;
#if 0
        std::cout << "loading pathgrid " <<
                     +"\""+ mPathgrid->mCell +"\""
                     +", "+ std::to_string(mPathgrid->mData.mX)
                     +", "+ std::to_string(mPathgrid->mData.mY)
                     << std::endl;
#endif
        return true;
    }

    // v is the pathgrid point index (some call them vertices)
    void PathgridGraph::recursiveStrongConnect(int v)
    {
        mSCCPoint[v].first = mSCCIndex;  // index
        mSCCPoint[v].second = mSCCIndex; // lowlink
        mSCCIndex++;
        mSCCStack.push_back(v);
        int w;

        for(int i = 0; i < static_cast<int> (mGraph[v].edges.size()); i++)
        {
            w = mGraph[v].edges[i].index;
            if(mSCCPoint[w].first == -1) // not visited
            {
                recursiveStrongConnect(w); // recurse
                mSCCPoint[v].second = std::min(mSCCPoint[v].second,
                                               mSCCPoint[w].second);
            }
            else
            {
                if(find(mSCCStack.begin(), mSCCStack.end(), w) != mSCCStack.end())
                    mSCCPoint[v].second = std::min(mSCCPoint[v].second,
                                                   mSCCPoint[w].first);
            }
        }

        if(mSCCPoint[v].second == mSCCPoint[v].first)
        {   // new component
            do
            {
                w = mSCCStack.back();
                mSCCStack.pop_back();
                mGraph[w].componentId = mSCCId;
            }
            while(w != v);
            mSCCId++;
        }
        return;
    }

    /*
     * mGraph contains the strongly connected component group id's along
     * with pre-calculated edge costs.
     *
     * A cell can have disjointed pathgrids, e.g. Seyda Neen has 3
     *
     * mGraph for Seyda Neen will therefore have 3 different values.  When
     * selecting a random pathgrid point for AiWander, mGraph can be checked
     * for quickly finding whether the destination is reachable.
     *
     * Otherwise, buildPath can automatically select a closest reachable end
     * pathgrid point (reachable from the closest start point).
     *
     * Using Tarjan's algorithm:
     *
     *  mGraph                   | graph G   |
     *  mSCCPoint                | V         | derived from mPoints
     *  mGraph[v].edges          | E (for v) |
     *  mSCCIndex                | index     | tracking smallest unused index
     *  mSCCStack                | S         |
     *  mGraph[v].edges[i].index | w         |
     *
     */
    void PathgridGraph::buildConnectedPoints()
    {
        // both of these are set to zero in the constructor
        //mSCCId = 0; // how many strongly connected components in this cell
        //mSCCIndex = 0;
        int pointsSize = mPathgrid->mPoints.size();
        mSCCPoint.resize(pointsSize, std::pair<int, int> (-1, -1));
        mSCCStack.reserve(pointsSize);

        for(int v = 0; v < static_cast<int> (pointsSize); v++)
        {
            if(mSCCPoint[v].first == -1) // undefined (haven't visited)
                recursiveStrongConnect(v);
        }
#if 0
        std::cout << "components: " << std::to_string(mSCCId)
                     +", "+ mPathgrid->mCell
                     << std::endl;
#endif
    }

    bool PathgridGraph::isPointConnected(const int start, const int end) const
    {
        return (mGraph[start].componentId == mGraph[end].componentId);
    }

    /*
     * NOTE: Based on buildPath2(), please check git history if interested
     *       Should consider using a 3rd party library version (e.g. boost)
     *
     * Find the shortest path to the target goal using a well known algorithm.
     * Uses mGraph which has pre-computed costs for allowed edges.  It is assumed
     * that mGraph is already constructed.
     *
     * Should be possible to make this MT safe.
     *
     * Returns path (a list of pathgrid point indexes) which may be empty.
     *
     * Input params:
     *   start, goal - pathgrid point indexes (for this cell)
     *   isExterior - used to determine whether to convert to world co-ordinates
     *
     * Variables:
     *   openset - point indexes to be traversed, lowest cost at the front
     *   closedset - point indexes already traversed
     *   gScore - past accumulated costs vector indexed by point index
     *   fScore - future estimated costs vector indexed by point index
     *
     * TODO: An intersting exercise might be to cache the paths created for a
     *       start/goal pair.  To cache the results the paths need to be in
     *       pathgrid points form (currently they are converted to world
     *       co-ordinates).  Essentially trading speed w/ memory.
     */
    std::list<ESM::Pathgrid::Point> PathgridGraph::aStarSearch(const int start,
                                                               const int goal,
                                                               bool isExterior) const
    {
        std::list<ESM::Pathgrid::Point> path;
        if(!isPointConnected(start, goal))
        {
            return path; // there is no path, return an empty path
        }

        int graphSize = mGraph.size();
        std::vector<float> gScore;
        gScore.resize(graphSize, -1);
        std::vector<float> fScore;
        fScore.resize(graphSize, -1);
        std::vector<int> graphParent;
        graphParent.resize(graphSize, -1);

        // gScore & fScore keep costs for each pathgrid point in mPoints
        gScore[start] = 0;
        fScore[start] = costAStar(mPathgrid->mPoints[start], mPathgrid->mPoints[goal]);

        std::list<int> openset;
        std::list<int> closedset;
        openset.push_back(start);

        int current = -1;

        while(!openset.empty())
        {
            current = openset.front(); // front has the lowest cost
            openset.pop_front();

            if(current == goal)
                break;

            closedset.push_back(current); // remember we've been here

            // check all edges for the current point index
            for(int j = 0; j < static_cast<int> (mGraph[current].edges.size()); j++)
            {
                if(std::find(closedset.begin(), closedset.end(), mGraph[current].edges[j].index) ==
                   closedset.end())
                {
                    // not in closedset - i.e. have not traversed this edge destination
                    int dest = mGraph[current].edges[j].index;
                    float tentative_g = gScore[current] + mGraph[current].edges[j].cost;
                    bool isInOpenSet = std::find(openset.begin(), openset.end(), dest) != openset.end();
                    if(!isInOpenSet
                        || tentative_g < gScore[dest])
                    {
                        graphParent[dest] = current;
                        gScore[dest] = tentative_g;
                        fScore[dest] = tentative_g + costAStar(mPathgrid->mPoints[dest],
                                                               mPathgrid->mPoints[goal]);
                        if(!isInOpenSet)
                        {
                            // add this edge to openset, lowest cost goes to the front
                            // TODO: if this causes performance problems a hash table may help
                            std::list<int>::iterator it = openset.begin();
                            for(it = openset.begin(); it!= openset.end(); it++)
                            {
                                if(fScore[*it] > fScore[dest])
                                    break;
                            }
                            openset.insert(it, dest);
                        }
                    }
                } // if in closedset, i.e. traversed this edge already, try the next edge
            }
        }

        if(current != goal)
            return path; // for some reason couldn't build a path

        // reconstruct path to return, using world co-ordinates
        float xCell = 0;
        float yCell = 0;
        if (isExterior)
        {
            xCell = mPathgrid->mData.mX * ESM::Land::REAL_SIZE;
            yCell = mPathgrid->mData.mY * ESM::Land::REAL_SIZE;
        }

        while(graphParent[current] != -1)
        {
            ESM::Pathgrid::Point pt = mPathgrid->mPoints[current];
            pt.mX += xCell;
            pt.mY += yCell;
            path.push_front(pt);
            current = graphParent[current];
        }
        return path;
    }
}

