#include "aitravel.hpp"
#include <iostream>

#include "character.hpp"

#include "../mwworld/class.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "movement.hpp"
#include "../mwworld/player.hpp"

#include <boost/graph/astar_search.hpp>
#include <boost/graph/adjacency_list.hpp>
#include "boost/tuple/tuple.hpp"

MWMechanics::AiTravel::AiTravel(float x, float y, float z)
: mX(x),mY(y),mZ(z),isPathConstructed(false)
{
}

MWMechanics::AiTravel * MWMechanics::AiTravel::clone() const
{
    return new AiTravel(*this);
}

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

float sgn(float a)
{
    if(a>0) return 1.;
    else return -1.;
}

float getZAngle(float dX,float dY)
{
    float h = sqrt(dX*dX+dY*dY);
    return Ogre::Radian(acos(dY/h)*sgn(asin(dX/h))).valueDegrees();
}

typedef boost::adjacency_list<boost::vecS,boost::vecS,boost::undirectedS,
    boost::property<boost::vertex_index_t,int,ESM::Pathgrid::Point>,boost::property<boost::edge_weight_t,float> > PathGridGraph;
typedef boost::property_map<PathGridGraph, boost::edge_weight_t>::type WeightMap;
typedef PathGridGraph::vertex_descriptor PointID;
typedef PathGridGraph::edge_descriptor   PointConnectionID;

struct found_path {};

class goalVisited : public boost::default_astar_visitor
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

class DistanceHeuristic : public boost::astar_heuristic <PathGridGraph, float>
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
};

std::list<ESM::Pathgrid::Point> getPath(PointID start,PointID end,PathGridGraph graph){
    std::vector<PointID> p(boost::num_vertices(graph));
    std::vector<float> d(boost::num_vertices(graph));
    std::list<ESM::Pathgrid::Point> shortest_path;

    try {
        boost::astar_search
            (
            graph,
            start,
            DistanceHeuristic(graph,end),
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

bool MWMechanics::AiTravel::execute (const MWWorld::Ptr& actor)
{
    const ESM::Pathgrid *pathgrid =
        MWBase::Environment::get().getWorld()->getStore().get<ESM::Pathgrid>().search(*actor.getCell()->mCell);
    
    ESM::Position pos = actor.getRefData().getPosition();
    bool cellChange = actor.getCell()->mCell->mData.mX != cellX || actor.getCell()->mCell->mData.mY != cellY;

    MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
    if(actor.getCell()->mCell->mData.mX != player.getCell()->mCell->mData.mX)
    {
        int sideX = sgn(actor.getCell()->mCell->mData.mX - player.getCell()->mCell->mData.mX);
        //check if actor is near the border of an inactive cell. If so, disable aitravel.
        if(sideX*(pos.pos[0] - actor.getCell()->mCell->mData.mX * ESM::Land::REAL_SIZE) > sideX*(ESM::Land::REAL_SIZE/2. - 2000)) 
        {
            MWWorld::Class::get(actor).getMovementSettings(actor).mForwardBackward = 0;
            return true;
        }
    }
    if(actor.getCell()->mCell->mData.mY != player.getCell()->mCell->mData.mY)
    {
        int sideY = sgn(actor.getCell()->mCell->mData.mY - player.getCell()->mCell->mData.mY);
        //check if actor is near the border of an inactive cell. If so, disable aitravel.
        if(sideY*(pos.pos[1] - actor.getCell()->mCell->mData.mY * ESM::Land::REAL_SIZE) > sideY*(ESM::Land::REAL_SIZE/2. - 2000)) 
        {
            MWWorld::Class::get(actor).getMovementSettings(actor).mForwardBackward = 0;
            return true;
        }
    }

    if(!isPathConstructed ||cellChange)
    {
        cellX = actor.getCell()->mCell->mData.mX;
        cellY = actor.getCell()->mCell->mData.mY;
        float xCell = 0;
        float yCell = 0;
        if (actor.getCell()->mCell->isExterior())
        {
            xCell = actor.getCell()->mCell->mData.mX * ESM::Land::REAL_SIZE;
            yCell = actor.getCell()->mCell->mData.mY * ESM::Land::REAL_SIZE;
        }

        int start = getClosestPoint(pathgrid,pos.pos[0] - xCell,pos.pos[1] - yCell,pos.pos[2]);
        int end = getClosestPoint(pathgrid,mX - xCell,mY - yCell,mZ);

        if(start != -1 && end != -1)
        {
            PathGridGraph graph = buildGraph(pathgrid,xCell,yCell);
            mPath = getPath(start,end,graph);
        }

        ESM::Pathgrid::Point dest;
        dest.mX = mX;
        dest.mY = mY;
        dest.mZ = mZ;
        mPath.push_back(dest);
        isPathConstructed = true;
    }
    if(mPath.empty())
    {
        MWWorld::Class::get(actor).getMovementSettings(actor).mForwardBackward = 0;
        return true;
    }
    ESM::Pathgrid::Point nextPoint = *mPath.begin();
    if(distanceZCorrected(nextPoint,pos.pos[0],pos.pos[1],pos.pos[2]) < 20)
    {
        mPath.pop_front();
        if(mPath.empty())
        {
            MWWorld::Class::get(actor).getMovementSettings(actor).mForwardBackward = 0;
            return true;
        }
        nextPoint = *mPath.begin();
    }
    
    float dX = nextPoint.mX - pos.pos[0];
    float dY = nextPoint.mY - pos.pos[1];

    MWBase::Environment::get().getWorld()->rotateObject(actor,0,0,getZAngle(dX,dY),false);
    MWWorld::Class::get(actor).getMovementSettings(actor).mForwardBackward = 1;

    return false;
}

int MWMechanics::AiTravel::getTypeId() const
{
    return 1;
}


