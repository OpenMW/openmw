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
    float manhattan(ESM::Pathgrid::Point a, ESM::Pathgrid::Point b)
    {
        return 300 * (abs(a.mX - b.mX) + abs(a.mY - b.mY) + abs(a.mZ - b.mZ));
    }

    // Choose a heuristics - these may not be the best for directed graphs with
    // non uniform edge costs.
    //
    //   distance:
    //   - sqrt((curr.x - goal.x)^2 + (curr.y - goal.y)^2 + (curr.z - goal.z)^2)
    //   - slower but more accurate
    //
    //   Manhattan:
    //   - |curr.x - goal.x| + |curr.y - goal.y| + |curr.z - goal.z|
    //   - faster but not the shortest path
    float costAStar(ESM::Pathgrid::Point a, ESM::Pathgrid::Point b)
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

    // Uses mSCComp to choose a reachable end pathgrid point.  start is assumed reachable.
    std::pair<int, bool> getClosestReachablePoint(const ESM::Pathgrid* grid,
            Ogre::Vector3 pos, int start, std::vector<int> &sCComp)
    {
        // assume grid is fine
        int startGroup = sCComp[start];

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
                if (sCComp[counter] == startGroup)
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
          mIsGraphConstructed(false),
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
     * NOTE: Based on buildPath2(), please check git history if interested
     *
     * Populate mGraph with the cost of each allowed edge.
     *
     * Any existing data in mGraph is wiped clean first.  The node's parent
     * is set with initial value of -1. The parent values are populated by
     * aStarSearch() in order to reconstruct a path.
     *
     * mGraph[f].edges[n].destination = t
     *
     *   f = point index of location "from"
     *   t = point index of location "to"
     *   n = index of edges from point f
     *
     *
     * Example: (note from p(0) to p(2) not allowed in this example)
     *
     *   mGraph[0].edges[0].destination = 1
     *            .edges[1].destination = 3
     *
     *   mGraph[1].edges[0].destination = 0
     *            .edges[1].destination = 2
     *            .edges[2].destination = 3
     *
     *   mGraph[2].edges[0].destination = 1
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
    void PathFinder::buildPathgridGraph(const ESM::Pathgrid* pathGrid)
    {
        mGraph.clear();
        // resize lists
        mGScore.resize(pathGrid->mPoints.size(), -1);
        mFScore.resize(pathGrid->mPoints.size(), -1);
        Node defaultNode;
        defaultNode.label = -1;
        defaultNode.parent = -1;
        mGraph.resize(pathGrid->mPoints.size(),defaultNode);
        // initialise mGraph
        for(unsigned int i = 0; i < pathGrid->mPoints.size(); i++)
        {
            Node node;
            node.label = i;
            node.parent = -1;
            mGraph[i] = node;
        }
        // store the costs of each edge
        for(unsigned int i = 0; i < pathGrid->mEdges.size(); i++)
        {
            Edge edge;
            edge.cost = costAStar(pathGrid->mPoints[pathGrid->mEdges[i].mV0],
                                  pathGrid->mPoints[pathGrid->mEdges[i].mV1]);
            // forward path of the edge
            edge.destination = pathGrid->mEdges[i].mV1;
            mGraph[pathGrid->mEdges[i].mV0].edges.push_back(edge);
            // reverse path of the edge
            // NOTE: These are redundant, the ESM already contains the reverse paths.
            //edge.destination = pathGrid->mEdges[i].mV0;
            //mGraph[pathGrid->mEdges[i].mV1].edges.push_back(edge);
        }
        mIsGraphConstructed = true;
    }

    // v is the pathgrid point index (some call them vertices)
    void PathFinder::recursiveStrongConnect(int v)
    {
        mSCCPoint[v].first = mSCCIndex;  // index
        mSCCPoint[v].second = mSCCIndex; // lowlink
        mSCCIndex++;
        mSCCStack.push_back(v);
        int w;

        for(int i = 0; i < static_cast<int> (mGraph[v].edges.size()); i++)
        {
            w = mGraph[v].edges[i].destination;
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
        {
            // new component
            do
            {
                w = mSCCStack.back();
                mSCCStack.pop_back();
                mSCComp[w] = mSCCId;
            }
            while(w != v);

            mSCCId++;
        }
        return;
    }

    /*
     * mSCComp contains the strongly connected component group id's.
     *
     * A cell can have disjointed pathgrid, e.g. Seyda Neen which has 3
     *
     * mSCComp for Seyda Neen will have 3 different values.  When selecting a
     * random pathgrid point for AiWander, mSCComp can be checked for quickly
     * finding whether the destination is reachable.
     *
     * Otherwise, buildPath will automatically select a closest reachable end
     * pathgrid point (reachable from the closest start point).
     *
     * Using Tarjan's algorithm
     *
     *  mGraph            | graph G    |
     *  mSCCPoint         | V          | derived from pathGrid->mPoints
     *  mGraph[v].edges   | E (for v)  |
     *  mSCCIndex         | index      | keep track of smallest unused index
     *  mSCCStack         | S          |
     *  pathGrid
     *    ->mEdges[v].mV1 | w          | = mGraph[v].edges[i].destination
     *
     *  FIXME: Some of these can be cleaned up by including them to struct
     *         Node used by mGraph
     */
    void PathFinder::buildConnectedPoints(const ESM::Pathgrid* pathGrid)
    {
        mSCComp.clear();
        mSCComp.resize(pathGrid->mPoints.size(), 0);
        mSCCId = 0;

        mSCCIndex = 0;
        mSCCStack.clear();
        mSCCPoint.clear();
        mSCCPoint.resize(pathGrid->mPoints.size(), std::pair<int, int> (-1, -1));

        for(unsigned int v = 0; v < pathGrid->mPoints.size(); v++)
        {
            if(mSCCPoint[v].first == -1) // undefined (haven't visited)
                recursiveStrongConnect(v);
        }
    }

    void PathFinder::cleanUpAStar()
    {
        for(int i = 0; i < static_cast<int> (mGraph.size()); i++)
        {
            mGraph[i].parent = -1;
            mGScore[i] = -1;
            mFScore[i] = -1;
        }
    }

    /*
     * NOTE: Based on buildPath2(), please check git history if interested
     *       Should consider a using 3rd party library version (e.g. boost)
     *
     * Find the shortest path to the target goal using a well known algorithm.
     * Uses mGraph which has pre-computed costs for allowed edges.  It is assumed
     * that mGraph is already constructed.  The caller, i.e.  buildPath(), needs
     * to ensure this.
     *
     * Returns path (a list of pathgrid point indexes) which may be empty.
     *
     * Input params:
     *   start, goal - pathgrid point indexes (for this cell)
     *   xCell, yCell - values to add to convert path back to world scale
     *
     * Variables:
     *   openset - point indexes to be traversed, lowest cost at the front
     *   closedset - point indexes already traversed
     *
     * Class variables:
     *   mGScore - past accumulated costs vector indexed by point index
     *   mFScore - future estimated costs vector indexed by point index
     *     these are resized by buildPathgridGraph()
     */
    std::list<ESM::Pathgrid::Point> PathFinder::aStarSearch(const ESM::Pathgrid* pathGrid,
                                                            int start, int goal,
                                                            float xCell, float yCell)
    {
        cleanUpAStar();
        // mGScore & mFScore keep costs for each pathgrid point in pathGrid->mPoints
        mGScore[start] = 0;
        mFScore[start] = costAStar(pathGrid->mPoints[start], pathGrid->mPoints[goal]);

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
                if(std::find(closedset.begin(), closedset.end(), mGraph[current].edges[j].destination) ==
                   closedset.end())
                {
                    // not in closedset - i.e. have not traversed this edge destination
                    int dest = mGraph[current].edges[j].destination;
                    float tentative_g = mGScore[current] + mGraph[current].edges[j].cost;
                    bool isInOpenSet = std::find(openset.begin(), openset.end(), dest) != openset.end();
                    if(!isInOpenSet
                        || tentative_g < mGScore[dest])
                    {
                        mGraph[dest].parent = current;
                        mGScore[dest] = tentative_g;
                        mFScore[dest] = tentative_g +
                                        costAStar(pathGrid->mPoints[dest], pathGrid->mPoints[goal]);
                        if(!isInOpenSet)
                        {
                            // add this edge to openset, lowest cost goes to the front
                            // TODO: if this causes performance problems a hash table may help
                            std::list<int>::iterator it = openset.begin();
                            for(it = openset.begin(); it!= openset.end(); it++)
                            {
                                if(mFScore[*it] > mFScore[dest])
                                    break;
                            }
                            openset.insert(it, dest);
                        }
                    }
                } // if in closedset, i.e. traversed this edge already, try the next edge
            }
        }

        std::list<ESM::Pathgrid::Point> path;
        if(current != goal)
            return path; // for some reason couldn't build a path
                         // e.g. start was not reachable (we assume it is)

        // reconstruct path to return, using world co-ordinates
        while(mGraph[current].parent != -1)
        {
            ESM::Pathgrid::Point pt = pathGrid->mPoints[current];
            pt.mX += xCell;
            pt.mY += yCell;
            path.push_front(pt);
            current = mGraph[current].parent;
        }

        // TODO: Is this a bug?  If path is empty the algorithm couldn't find a path.
        //       Simply using the destination as the path in this scenario seems strange.
        //       Commented out pending further testing.
#if 0
        if(path.empty())
        {
            ESM::Pathgrid::Point pt = pathGrid->mPoints[goal];
            pt.mX += xCell;
            pt.mY += yCell;
            path.push_front(pt);
        }
#endif
        return path;
    }

    /*
     * NOTE: This method may fail to find a path.  The caller must check the
     * result before using it.  If there is no path the AI routies need to
     * implement some other heuristics to reach the target.
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
     * May update mGraph by calling buildPathgridGraph() if it isn't
     * constructed yet.  At the same time mConnectedPoints is also updated.
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
                               const MWWorld::CellStore* cell, bool allowShortcuts)
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

        if(mCell != cell)
        {
            mIsGraphConstructed = false; // must be in a new cell, need a new mGraph and mSCComp
            mCell = cell;
        }

        const ESM::Pathgrid *pathGrid =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Pathgrid>().search(*mCell->getCell());
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
        //
        // NOTE: getClosestPoint expects local co-ordinates
        //
        int startNode = getClosestPoint(pathGrid,
                    Ogre::Vector3(startPoint.mX - xCell, startPoint.mY - yCell, startPoint.mZ));

        if(startNode != -1) // only check once, assume pathGrid won't change
        {
            if(!mIsGraphConstructed)
            {
                buildPathgridGraph(pathGrid);   // pre-compute costs for use with aStarSearch
                buildConnectedPoints(pathGrid); // must before calling getClosestReachablePoint
            }
            std::pair<int, bool> endNode = getClosestReachablePoint(pathGrid,
                    Ogre::Vector3(endPoint.mX - xCell, endPoint.mY - yCell, endPoint.mZ),
                    startNode, mSCComp);

            if(endNode.first != -1)
            {
                mPath = aStarSearch(pathGrid, startNode, endNode.first, xCell, yCell);

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
            mIsPathConstructed = false; // this shouldn't really happen, but just in case
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

}

