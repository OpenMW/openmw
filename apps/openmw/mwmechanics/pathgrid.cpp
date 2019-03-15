#include "pathgrid.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"

namespace
{
    // See https://theory.stanford.edu/~amitp/GameProgramming/Heuristics.html
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
    float manhattan(const ESM::Pathgrid::Point& a, const ESM::Pathgrid::Point& b)
    {
        return 300.0f * (abs(a.mX - b.mX) + abs(a.mY - b.mY) + abs(a.mZ - b.mZ));
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
    float costAStar(const ESM::Pathgrid::Point& a, const ESM::Pathgrid::Point& b)
    {
        //return distance(a, b);
        return manhattan(a, b);
    }
}

namespace MWMechanics
{
    PathgridGraph::PathgridGraph(const MWWorld::CellStore *cell)
        : mCell(nullptr)
        , mPathgrid(nullptr)
        , mIsExterior(0)
        , mGraph(0)
        , mIsGraphConstructed(false)
        , mSCCId(0)
        , mSCCIndex(0)
    {
        load(cell);
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
    bool PathgridGraph::load(const MWWorld::CellStore *cell)
    {
        if(!cell)
            return false;

        if(mIsGraphConstructed)
            return true;

        mCell = cell->getCell();
        mIsExterior = cell->getCell()->isExterior();
        mPathgrid = MWBase::Environment::get().getWorld()->getStore().get<ESM::Pathgrid>().search(*cell->getCell());
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
        return true;
    }

    const ESM::Pathgrid *PathgridGraph::getPathgrid() const
    {
        return mPathgrid;
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
        int pointsSize = static_cast<int> (mPathgrid->mPoints.size());
        mSCCPoint.resize(pointsSize, std::pair<int, int> (-1, -1));
        mSCCStack.reserve(pointsSize);

        for(int v = 0; v < pointsSize; v++)
        {
            if(mSCCPoint[v].first == -1) // undefined (haven't visited)
                recursiveStrongConnect(v);
        }
    }

    bool PathgridGraph::isPointConnected(const int start, const int end) const
    {
        return (mGraph[start].componentId == mGraph[end].componentId);
    }

    void PathgridGraph::getNeighbouringPoints(const int index, ESM::Pathgrid::PointList &nodes) const
    {
        for(int i = 0; i < static_cast<int> (mGraph[index].edges.size()); i++)
        {
            int neighbourIndex = mGraph[index].edges[i].index;
            if (neighbourIndex != index)
                nodes.push_back(mPathgrid->mPoints[neighbourIndex]);
        }
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
     * Returns path which may be empty.  path contains pathgrid points in local
     * cell coordinates (indoors) or world coordinates (external).
     *
     * Input params:
     *   start, goal - pathgrid point indexes (for this cell)
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
     *       coordinates).  Essentially trading speed w/ memory.
     */
    std::deque<ESM::Pathgrid::Point> PathgridGraph::aStarSearch(const int start, const int goal) const
    {
        std::deque<ESM::Pathgrid::Point> path;
        if(!isPointConnected(start, goal))
        {
            return path; // there is no path, return an empty path
        }

        int graphSize = static_cast<int> (mGraph.size());
        std::vector<float> gScore (graphSize, -1);
        std::vector<float> fScore (graphSize, -1);
        std::vector<int> graphParent (graphSize, -1);

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
                            for(it = openset.begin(); it!= openset.end(); ++it)
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

        // reconstruct path to return, using local coordinates
        while(graphParent[current] != -1)
        {
            path.push_front(mPathgrid->mPoints[current]);
            current = graphParent[current];
        }

        // add first node to path explicitly
        path.push_front(mPathgrid->mPoints[start]);
        return path;
    }
}

