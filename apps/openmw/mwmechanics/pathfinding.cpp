#include "pathfinding.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "OgreMath.h"

#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/adjacency_list.hpp>

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

    static float sgn(float a)
    {
        if(a > 0)
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

    std::list<ESM::Pathgrid::Point> findPath(PointID start, PointID end, PathGridGraph graph)
    {
        std::vector<PointID> p(boost::num_vertices(graph));
        std::vector<float> d(boost::num_vertices(graph));
        std::list<ESM::Pathgrid::Point> shortest_path;

        try
        {
            boost::dijkstra_shortest_paths(graph, start,
                boost::predecessor_map(&p[0]).distance_map(&d[0]).visitor(goalVisited(end)));
        }

        catch(found_path fg)
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

namespace MWMechanics
{
    PathFinder::PathFinder()
    {
        mIsPathConstructed = false;
    }

    void PathFinder::clearPath()
    {
        if(!mPath.empty())
            mPath.clear();
        mIsPathConstructed = false;
    }

    void PathFinder::buildPath(ESM::Pathgrid::Point startPoint, ESM::Pathgrid::Point endPoint,
        const ESM::Pathgrid* pathGrid, float xCell, float yCell, bool allowShortcuts)
    {
        if(allowShortcuts)
        {
            if(MWBase::Environment::get().getWorld()->castRay(startPoint.mX, startPoint.mY, startPoint.mZ, endPoint.mX, endPoint.mY,
                endPoint.mZ))
                allowShortcuts = false;
        }

        if(!allowShortcuts)
        {
            int startNode = getClosestPoint(pathGrid, startPoint.mX - xCell, startPoint.mY - yCell,startPoint.mZ);
            int endNode = getClosestPoint(pathGrid, endPoint.mX - xCell, endPoint.mY - yCell, endPoint.mZ);

            if(startNode != -1 && endNode != -1)
            {
                PathGridGraph graph = buildGraph(pathGrid, xCell, yCell);
                mPath = findPath(startNode, endNode, graph);

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

    float PathFinder::getZAngleToNext(float x, float y)
    {
        // This should never happen (programmers should have an if statement checking mIsPathConstructed that prevents this call
        // if otherwise).
        if(mPath.empty())
            return 0;

        ESM::Pathgrid::Point nextPoint = *mPath.begin();
        float directionX = nextPoint.mX - x;
        float directionY = nextPoint.mY - y;
        float directionResult = sqrt(directionX * directionX + directionY * directionY);

        return Ogre::Radian(acos(directionY / directionResult) * sgn(asin(directionX / directionResult))).valueDegrees();
    }

    bool PathFinder::checkPathCompleted(float x, float y, float z)
    {
        if(mPath.empty())
            return true;

        ESM::Pathgrid::Point nextPoint = *mPath.begin();
        if(distanceZCorrected(nextPoint, x, y, z) < 40)
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

    std::list<ESM::Pathgrid::Point> PathFinder::getPath()
    {
        return mPath;
    }

    bool PathFinder::isPathConstructed()
    {
        return mIsPathConstructed;
    }
}

