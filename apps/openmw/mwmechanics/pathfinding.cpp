#include "pathfinding.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "OgreMath.h"

#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <map>

namespace
{
    struct found_path {};

    typedef boost::adjacency_list< boost::vecS, boost::vecS, boost::undirectedS,
        boost::property<boost::vertex_index_t, int, ESM::Pathgrid::Point>, boost::property<boost::edge_weight_t, float> >
        PathGridGraph;
    typedef boost::property_map<PathGridGraph, boost::edge_weight_t>::type WeightMap;
    typedef PathGridGraph::vertex_descriptor PointID;
    typedef PathGridGraph::edge_descriptor PointConnectionID;

    class goalVisited : public boost::default_dijkstra_visitor
    {
        public:
            goalVisited(PointID goal) {mGoal = goal;};
            void examine_vertex(PointID u, const PathGridGraph g) {if(u == mGoal) throw found_path();};

        private:
            PointID mGoal;
    };

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

    PathGridGraph buildGraph(const ESM::Pathgrid* pathgrid, float xCell = 0, float yCell = 0)
    {
        PathGridGraph graph;

        for(unsigned int counter = 0; counter < pathgrid->mPoints.size(); counter++)
        {
            PointID pID = boost::add_vertex(graph);
            graph[pID].mX = pathgrid->mPoints[counter].mX + xCell;
            graph[pID].mY = pathgrid->mPoints[counter].mY + yCell;
            graph[pID].mZ = pathgrid->mPoints[counter].mZ;
        }

        for(unsigned int counterTwo = 0; counterTwo < pathgrid->mEdges.size(); counterTwo++)
        {
            PointID u = pathgrid->mEdges[counterTwo].mV0;
            PointID v = pathgrid->mEdges[counterTwo].mV1;

            PointConnectionID edge;
            bool done;
            boost::tie(edge, done) = boost::add_edge(u, v, graph);
            WeightMap weightmap = boost::get(boost::edge_weight, graph);
            weightmap[edge] = distance(graph[u], graph[v]);
        }

        return graph;
    }

    std::list<ESM::Pathgrid::Point> findPath(PointID start, PointID end,const PathGridGraph& graph)
    {
        std::vector<PointID> p(boost::num_vertices(graph));
        std::vector<float> d(boost::num_vertices(graph));
        std::list<ESM::Pathgrid::Point> shortest_path;

        try
        {
            boost::dijkstra_shortest_paths(graph, start,
                boost::predecessor_map(&p[0]).distance_map(&d[0]).visitor(goalVisited(end)));
        }

        catch(found_path& fg)
        {
            for(PointID v = end; ; v = p[v])
            {
                shortest_path.push_front(graph[v]);
                if(p[v] == v)
                    break;
            }
        }

        return shortest_path;
    }
}

namespace
{
    struct Edge
    {
        int destination;
        float cost;
    };
    struct Node
    {
        int label;
        std::vector<Edge> edges;
        int parent;//used in pathfinding
    };

    std::list<ESM::Pathgrid::Point> reconstructPath(const std::vector<Node>& graph,const ESM::Pathgrid* pathgrid, int lastNode,float xCell, float yCell)
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
    }

    std::list<ESM::Pathgrid::Point> buildPath2(const ESM::Pathgrid* pathgrid,int start,int goal,float xCell = 0, float yCell = 0)
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

    }

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

    void PathFinder::buildPathgridGraph(const ESM::Pathgrid* pathGrid,float xCell, float yCell)
    {
        mGraph = buildGraph(pathGrid, xCell, yCell);
        mIsGraphConstructed = true;
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
                //if(!mIsGraphConstructed) buildPathgridGraph(pathGrid, xCell, yCell);

                mPath = buildPath2(pathGrid,startNode,endNode,xCell,yCell);//findPath(startNode, endNode, mGraph);

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
}

