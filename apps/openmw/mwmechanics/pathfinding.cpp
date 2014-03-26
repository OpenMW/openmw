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

    int getClosestPoint(const ESM::Pathgrid* grid, float x, float y, float z)
    {
        if(!grid || grid->mPoints.empty())
            return -1;

        float distanceBetween = distance(grid->mPoints[0], x, y, z);
        int closestIndex = 0;

        for(unsigned int counter = 1; counter < grid->mPoints.size(); counter++)
        {
            if(distance(grid->mPoints[counter], x, y, z) < distanceBetween)
            {
                distanceBetween = distance(grid->mPoints[counter], x, y, z);
                closestIndex = counter;
            }
        }

        return closestIndex;
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
     * NOTE: based on buildPath2(), please check git history if interested
     *
     * Populate mGraph with the cost of each allowed edge (measured in distance ^2)
     * Any existing data in mGraph is wiped clean first.  The node's parent is
     * set with initial value of -1. The parent values are populated by aStarSearch().
     * mGSore and mFScore are also resized.
     *
     *
     * mGraph[f].edges[n].destination = t
     *
     *   f = point index of location "from"
     *   t = point index of location "to"
     *   n = index of edges from point f
     *
     *
     * Example: (note from p(0) to p(2) not allowed)
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
            mGraph[i] = node; // TODO: old code used push_back(node), check if any difference
        }
        // store the costs (measured in distance ^2) of each edge, in both directions
        for(unsigned int i = 0; i < pathGrid->mEdges.size(); i++)
        {
            Edge edge;
            edge.cost = distance(pathGrid->mPoints[pathGrid->mEdges[i].mV0],
                                 pathGrid->mPoints[pathGrid->mEdges[i].mV1]);
            // forward path of the edge
            edge.destination = pathGrid->mEdges[i].mV1;
            mGraph[pathGrid->mEdges[i].mV0].edges.push_back(edge);
            // reverse path of the edge
            // NOTE: These are redundant, ESM already contains the required reverse paths
            //edge.destination = pathGrid->mEdges[i].mV0;
            //mGraph[pathGrid->mEdges[i].mV1].edges.push_back(edge);
        }
        mIsGraphConstructed = true;
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
     * NOTE: based on buildPath2(), please check git history if interested
     *
     * Find the shortest path to the target goal using a well known algorithm.
     * Uses mGraph which has pre-computed costs for allowed edges.  It is assumed
     * that mGraph is already constructed.  The caller, i.e.  buildPath(), needs
     * to ensure this.
     *
     * Returns path (a list of pathgrid point indexes) which may be empty.
     *
     * openset - point indexes to be traversed, lowest cost at the front
     * closedset - point indexes already traversed
     *
     * mGScore - past accumulated costs vector indexed by point index
     * mFScore - future estimated costs vector indexed by point index
     *   these are resized by buildPathgridGraph()
     *
     * The heuristics used is distance^2 from current position to the final goal.
     */
    std::list<ESM::Pathgrid::Point> PathFinder::aStarSearch(const ESM::Pathgrid* pathGrid,int start,int goal,float xCell, float yCell)
    {
        cleanUpAStar();
        mGScore[start] = 0;
        mFScore[start] = distance(pathGrid->mPoints[start],pathGrid->mPoints[goal]);

        std::list<int> openset;
        std::list<int> closedset;
        openset.push_back(start);

        int current = -1;

        while(!openset.empty())
        {
            current = openset.front(); // front has the lowest cost
            openset.pop_front();

            if(current == goal) break;

            closedset.push_back(current);

            // check all edges for the "current" point index
            for(int j = 0; j < static_cast<int> (mGraph[current].edges.size()); j++)
            {
                if(std::find(closedset.begin(),closedset.end(),mGraph[current].edges[j].destination) == closedset.end())
                {
                    // not in closedset - i.e. have not traversed this edge destination
                    int dest = mGraph[current].edges[j].destination;
                    float tentative_g = mGScore[current] + mGraph[current].edges[j].cost;
                    bool isInOpenSet = std::find(openset.begin(),openset.end(),dest) != openset.end();
                    if(!isInOpenSet
                        || tentative_g < mGScore[dest] )
                    {
                        mGraph[dest].parent = current;
                        mGScore[dest] = tentative_g;
                        mFScore[dest] = tentative_g + distance(pathGrid->mPoints[dest],pathGrid->mPoints[goal]);
                        if(!isInOpenSet)
                        {
                            // add this edge to openset, lowest cost goes to the front
                            // TODO: if this causes performance problems a hash table may help (apparently)
                            std::list<int>::iterator it = openset.begin();
                            for(it = openset.begin();it!= openset.end();it++)
                            {
                                if(mGScore[*it] > mGScore[dest])
                                    break;
                            }
                            openset.insert(it, dest);
                        }
                    }
                } // if in closedset, i.e. traversed this edge already, try the next edge
            }
        }

        // reconstruct path to return
        std::list<ESM::Pathgrid::Point> path;
        while(mGraph[current].parent != -1)
        {
            //std::cout << "not empty" << xCell;
            ESM::Pathgrid::Point pt = pathGrid->mPoints[current];
            pt.mX += xCell;
            pt.mY += yCell;
            path.push_front(pt);
            current = mGraph[current].parent;
        }

        // TODO: Is this a bug?  If path is empty the destination is inserted.
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
     * NOTE: This method may fail to find a path.  The caller must check the result before using it.
     * If there is no path the AI routies need to implement some other heuristics to reach the target.
     *
     * Updates mPath using aStarSearch().
     *   mPathConstructed is set true if successful, false if not
     *
     * May update mGraph by calling buildPathgridGraph() if it isn't constructed yet.
     */
    void PathFinder::buildPath(const ESM::Pathgrid::Point &startPoint, const ESM::Pathgrid::Point &endPoint,
                               const MWWorld::CellStore* cell, bool allowShortcuts)
    {
        mPath.clear();
        if(mCell != cell) mIsGraphConstructed = false;
        mCell = cell;

        if(allowShortcuts)
        {
            if(MWBase::Environment::get().getWorld()->castRay(startPoint.mX, startPoint.mY, startPoint.mZ,
                                                              endPoint.mX, endPoint.mY, endPoint.mZ))
                allowShortcuts = false;
        }

        if(!allowShortcuts)
        {
            const ESM::Pathgrid *pathGrid =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Pathgrid>().search(*mCell->getCell());
            float xCell = 0;
            float yCell = 0;

            if (mCell->isExterior())
            {
                xCell = mCell->getCell()->mData.mX * ESM::Land::REAL_SIZE;
                yCell = mCell->getCell()->mData.mY * ESM::Land::REAL_SIZE;
            }
            int startNode = getClosestPoint(pathGrid, startPoint.mX - xCell, startPoint.mY - yCell,startPoint.mZ);
            int endNode = getClosestPoint(pathGrid, endPoint.mX - xCell, endPoint.mY - yCell, endPoint.mZ);

            if(startNode != -1 && endNode != -1)
            {
                if(!mIsGraphConstructed) buildPathgridGraph(pathGrid);

                mPath = aStarSearch(pathGrid,startNode,endNode,xCell,yCell);

                if(!mPath.empty())
                {
                    mIsPathConstructed = true;
                }
            }
        }
        else
        {
            mPath.push_back(endPoint);
            mIsPathConstructed = true;
        }

        if(mPath.empty())
            mIsPathConstructed = false;
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

