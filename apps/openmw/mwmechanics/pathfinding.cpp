#include "pathfinding.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "OgreMath.h"
#include "OgreVector3.h"


#include <map>

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

    static float sgn(Ogre::Radian a)
    {
        if(a.valueRadians() > 0)
            return 1.0;
        return -1.0;
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

    /*std::list<ESM::Pathgrid::Point> reconstructPath(const std::vector<MWMechanics::PathFinder::Node>& graph,const ESM::Pathgrid* pathgrid, int lastNode,float xCell, float yCell)
    {
        std::list<ESM::Pathgrid::Point> path;
        while(graph[lastNode].parent != -1)
        {
            //std::cout << "not empty" << xCell;
            ESM::Pathgrid::Point pt = pathgrid->mPoints[lastNode];
            pt.mX += xCell;
            pt.mY += yCell;
            path.push_front(pt);
            lastNode = graph[lastNode].parent;
        }
        return path;
    }*/



    /*std::list<ESM::Pathgrid::Point> buildPath2(const ESM::Pathgrid* pathgrid,int start,int goal,float xCell = 0, float yCell = 0)
    {
        std::vector<Node> graph;
        for(unsigned int i = 0; i < pathgrid->mPoints.size(); i++)
        {
            Node node;
            node.label = i;
            node.parent = -1;
            graph.push_back(node);
        }
        for(unsigned int i = 0; i < pathgrid->mEdges.size(); i++)
        {
            Edge edge;
            edge.destination = pathgrid->mEdges[i].mV1;
            edge.cost = distance(pathgrid->mPoints[pathgrid->mEdges[i].mV0],pathgrid->mPoints[pathgrid->mEdges[i].mV1]);
            graph[pathgrid->mEdges[i].mV0].edges.push_back(edge);
            edge.destination = pathgrid->mEdges[i].mV0;
            graph[pathgrid->mEdges[i].mV1].edges.push_back(edge);
        }

        std::vector<float> g_score(pathgrid->mPoints.size(),-1.);
        std::vector<float> f_score(pathgrid->mPoints.size(),-1.);

        g_score[start] = 0;
        f_score[start] = distance(pathgrid->mPoints[start],pathgrid->mPoints[goal]);

        std::list<int> openset;
        std::list<int> closedset;
        openset.push_back(start);

        int current = -1;

        while(!openset.empty())
        {
            current = openset.front();
            openset.pop_front();

            if(current == goal) break;

            closedset.push_back(current);

            for(int j = 0;j<graph[current].edges.size();j++)
            {
                //int next = graph[current].edges[j].destination
                if(std::find(closedset.begin(),closedset.end(),graph[current].edges[j].destination) == closedset.end())
                {
                    int dest = graph[current].edges[j].destination;
                    float tentative_g = g_score[current] + graph[current].edges[j].cost;
                    bool isInOpenSet = std::find(openset.begin(),openset.end(),dest) != openset.end();
                    if(!isInOpenSet
                        || tentative_g < g_score[dest] )
                    {
                        graph[dest].parent = current;
                        g_score[dest] = tentative_g;
                        f_score[dest] = tentative_g + distance(pathgrid->mPoints[dest],pathgrid->mPoints[goal]);
                        if(!isInOpenSet)
                        {
                            std::list<int>::iterator it = openset.begin();
                            for(it = openset.begin();it!= openset.end();it++)
                            {
                                if(g_score[*it]>g_score[dest])
                                    break;
                            }
                            openset.insert(it,dest);
                        }
                    }
                }
            }

        }
        return reconstructPath(graph,pathgrid,current,xCell,yCell);

    }*/

}

namespace MWMechanics
{
    PathFinder::PathFinder()
        :mIsPathConstructed(false),mIsGraphConstructed(false)
    {
    }

    void PathFinder::clearPath()
    {
        if(!mPath.empty())
            mPath.clear();
        mIsPathConstructed = false;
    }

    void PathFinder::buildPathgridGraph(const ESM::Pathgrid* pathGrid)
    {
        mGraph.clear();
        mGScore.resize(pathGrid->mPoints.size(),-1);
        mFScore.resize(pathGrid->mPoints.size(),-1);
        Node defaultNode;
        defaultNode.label = -1;
        defaultNode.parent = -1;
        mGraph.resize(pathGrid->mPoints.size(),defaultNode);
        for(unsigned int i = 0; i < pathGrid->mPoints.size(); i++)
        {
            Node node;
            node.label = i;
            node.parent = -1;
            mGraph[i] = node;
        }
        for(unsigned int i = 0; i < pathGrid->mEdges.size(); i++)
        {
            Edge edge;
            edge.destination = pathGrid->mEdges[i].mV1;
            edge.cost = distance(pathGrid->mPoints[pathGrid->mEdges[i].mV0],pathGrid->mPoints[pathGrid->mEdges[i].mV1]);
            mGraph[pathGrid->mEdges[i].mV0].edges.push_back(edge);
            edge.destination = pathGrid->mEdges[i].mV0;
            mGraph[pathGrid->mEdges[i].mV1].edges.push_back(edge);
        }
        mIsGraphConstructed = true;
    }

    void PathFinder::cleanUpAStar()
    {
        for(int i=0;i<static_cast<int> (mGraph.size());i++)
        {
            mGraph[i].parent = -1;
            mGScore[i] = -1;
            mFScore[i] = -1;
        }
    }

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
            current = openset.front();
            openset.pop_front();

            if(current == goal) break;

            closedset.push_back(current);

            for(int j = 0;j<static_cast<int> (mGraph[current].edges.size());j++)
            {
                //int next = mGraph[current].edges[j].destination
                if(std::find(closedset.begin(),closedset.end(),mGraph[current].edges[j].destination) == closedset.end())
                {
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
                            std::list<int>::iterator it = openset.begin();
                            for(it = openset.begin();it!= openset.end();it++)
                            {
                                if(mGScore[*it]>mGScore[dest])
                                    break;
                            }
                            openset.insert(it,dest);
                        }
                    }
                }
            }

        }

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

        if(path.empty())
        {
            ESM::Pathgrid::Point pt = pathGrid->mPoints[goal];
            pt.mX += xCell;
            pt.mY += yCell;
            path.push_front(pt);
        }

        return path;
    }

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
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Pathgrid>().search(*mCell->mCell);
            float xCell = 0;
            float yCell = 0;

            if (mCell->isExterior())
            {
                xCell = mCell->mCell->mData.mX * ESM::Land::REAL_SIZE;
                yCell = mCell->mCell->mData.mY * ESM::Land::REAL_SIZE;
            }
            int startNode = getClosestPoint(pathGrid, startPoint.mX - xCell, startPoint.mY - yCell,startPoint.mZ);
            int endNode = getClosestPoint(pathGrid, endPoint.mX - xCell, endPoint.mY - yCell, endPoint.mZ);

            if(startNode != -1 && endNode != -1)
            {
                if(!mIsGraphConstructed) buildPathgridGraph(pathGrid);

                mPath = aStarSearch(pathGrid,startNode,endNode,xCell,yCell);//findPath(startNode, endNode, mGraph);

                if(!mPath.empty())
                {
                    mPath.push_back(endPoint);
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
        // This should never happen (programmers should have an if statement checking mIsPathConstructed that prevents this call
        // if otherwise).
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

