#include "pathfinding.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/adjacency_list.hpp>
#include "boost/tuple/tuple.hpp"
#include "OgreMath.h"

namespace
{
	//helpers functions
    float distanceZCorrected(ESM::Pathgrid::Point point,float x,float y,float z)
    {
        return sqrt((point.mX - x)*(point.mX - x)+(point.mY - y)*(point.mY - y)+0.1*(point.mZ - z)*(point.mZ - z));
    }

    float distance(ESM::Pathgrid::Point point,float x,float y,float z)
    {
        return sqrt((point.mX - x)*(point.mX - x)+(point.mY - y)*(point.mY - y)+(point.mZ - z)*(point.mZ - z));
    }

    float distance(ESM::Pathgrid::Point a,ESM::Pathgrid::Point b)
    {
        return sqrt(float(a.mX - b.mX)*(a.mX - b.mX)+(a.mY - b.mY)*(a.mY - b.mY)+(a.mZ - b.mZ)*(a.mZ - b.mZ));
    }

    static float sgn(float a)
    {
        if(a>0) return 1.;
        else return -1.;
    }

    int getClosestPoint(const ESM::Pathgrid* grid,float x,float y,float z)
    {
        if(!grid) return -1;
        if(grid->mPoints.empty()) return -1;

        float m = distance(grid->mPoints[0],x,y,z);
        int i0 = 0;

        for(unsigned int i=1; i<grid->mPoints.size();++i)
        {
            if(distance(grid->mPoints[i],x,y,z)<m)
            {
                m = distance(grid->mPoints[i],x,y,z);
                i0 = i;
            }
        }
        return i0;
    }

    typedef boost::adjacency_list<boost::vecS,boost::vecS,boost::undirectedS,
        boost::property<boost::vertex_index_t,int,ESM::Pathgrid::Point>,boost::property<boost::edge_weight_t,float> > PathGridGraph;
    typedef boost::property_map<PathGridGraph, boost::edge_weight_t>::type WeightMap;
    typedef PathGridGraph::vertex_descriptor PointID;
    typedef PathGridGraph::edge_descriptor   PointConnectionID;

    struct found_path {};

    /*class goalVisited : public boost::default_astar_visitor
    {
    public:
        goalVisited(PointID goal) : mGoal(goal) {}

        void examine_vertex(PointID u, const PathGridGraph g)
        {
            if(u == mGoal)
                throw found_path();
        }
    private:
        PointID mGoal;
    };

    class DistanceHeuristic : public boost::atasr_heuristic <PathGridGraph, float>
    {
    public:
        DistanceHeuristic(const PathGridGraph & l, PointID goal)
            : mGraph(l), mGoal(goal) {}

        float operator()(PointID u)
        {
            const ESM::Pathgrid::Point & U = mGraph[u];
            const ESM::Pathgrid::Point & V = mGraph[mGoal];
            float dx = U.mX - V.mX;
            float dy = U.mY - V.mY;
            float dz = U.mZ - V.mZ;
            return sqrt(dx * dx + dy * dy + dz * dz);
        }
    private:
        const PathGridGraph & mGraph;
        PointID mGoal;
    };*/

	class goalVisited : public boost::default_dijkstra_visitor
	{
	public:
		goalVisited(PointID goal) : mGoal(goal) {}

		void examine_vertex(PointID u, const PathGridGraph g)
		{
			if(u == mGoal)
				throw found_path();
		}
	private:
		PointID mGoal;
	};


    PathGridGraph buildGraph(const ESM::Pathgrid* pathgrid,float xCell = 0,float yCell = 0)
    {
        PathGridGraph graph;

        for(unsigned int i = 0;i<pathgrid->mPoints.size();++i)
        {
            PointID pID = boost::add_vertex(graph);
            graph[pID].mX = pathgrid->mPoints[i].mX + xCell;
            graph[pID].mY = pathgrid->mPoints[i].mY + yCell;
            graph[pID].mZ = pathgrid->mPoints[i].mZ;
        }

        for(unsigned int i = 0;i<pathgrid->mEdges.size();++i)
        {
            PointID u = pathgrid->mEdges[i].mV0;
            PointID v = pathgrid->mEdges[i].mV1;

            PointConnectionID edge;
            bool done;
            boost::tie(edge,done) = boost::add_edge(u,v,graph);
            WeightMap weightmap = boost::get(boost::edge_weight, graph);
            weightmap[edge] = distance(graph[u],graph[v]);

        }

        return graph;
    }

    std::list<ESM::Pathgrid::Point> findPath(PointID start,PointID end,PathGridGraph graph){
        std::vector<PointID> p(boost::num_vertices(graph));
        std::vector<float> d(boost::num_vertices(graph));
        std::list<ESM::Pathgrid::Point> shortest_path;

        try {
            boost::dijkstra_shortest_paths
                (
                graph,
                start,
                boost::predecessor_map(&p[0]).distance_map(&d[0]).visitor(goalVisited(end))//.weight_map(boost::get(&Edge::distance, graph))
				);

        } catch(found_path fg) {
            for(PointID v = end;; v = p[v]) {
                shortest_path.push_front(graph[v]);
                if(p[v] == v)
                    break;
            }
        }
        return shortest_path;
    }

    //end of helpers functions

}

namespace MWMechanics
{
    PathFinder::PathFinder()
    {
        mIsPathConstructed = false;
    }

    void PathFinder::buildPath(ESM::Pathgrid::Point startPoint,ESM::Pathgrid::Point endPoint,
        const ESM::Pathgrid* pathGrid,float xCell,float yCell)
    {
        //first check if there is an obstacle
        if(MWBase::Environment::get().getWorld()->castRay(startPoint.mX,startPoint.mY,startPoint.mZ,
            endPoint.mX,endPoint.mY,endPoint.mZ) )
        {
            int start = getClosestPoint(pathGrid,startPoint.mX - xCell,startPoint.mY - yCell,startPoint.mZ);
            int end = getClosestPoint(pathGrid,endPoint.mX - xCell,endPoint.mY - yCell,endPoint.mZ);

            if(start != -1 && end != -1)
            {
                PathGridGraph graph = buildGraph(pathGrid,xCell,yCell);
                mPath = findPath(start,end,graph);
            }
        }
        mPath.push_back(endPoint);
        mIsPathConstructed = true;
    }

    float PathFinder::getZAngleToNext(float x,float y,float z)
    {
        if(mPath.empty())
        {
            return 0;/// shouldn't happen!
        }
        ESM::Pathgrid::Point nextPoint = *mPath.begin();
        float dX = nextPoint.mX - x;
        float dY = nextPoint.mY - y;
        float h = sqrt(dX*dX+dY*dY);
        return Ogre::Radian(acos(dY/h)*sgn(asin(dX/h))).valueDegrees();
    }

    bool PathFinder::checkIfNextPointReached(float x,float y,float z)
    {
        if(mPath.empty())
        {
            return true;
        }
        ESM::Pathgrid::Point nextPoint = *mPath.begin();
        if(distanceZCorrected(nextPoint,x,y,z) < 20)
        {
            mPath.pop_front();
            if(mPath.empty())
            {
                return true;
            }
            nextPoint = *mPath.begin();
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