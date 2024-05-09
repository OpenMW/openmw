#include "pathgrid.hpp"

#include <algorithm>
#include <list>
#include <set>

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
        // return distance(a, b);
        return manhattan(a, b);
    }

    constexpr size_t NoIndex = static_cast<size_t>(-1);
}

namespace MWMechanics
{

    class PathgridGraph::Builder
    {
        std::vector<Node>& mGraph;

        // variables used to calculate connected components
        int mSCCId = 0;
        size_t mSCCIndex = 0;
        std::vector<size_t> mSCCStack;
        std::vector<std::pair<size_t, size_t>> mSCCPoint; // first is index, second is lowlink

        // v is the pathgrid point index (some call them vertices)
        void recursiveStrongConnect(const size_t v)
        {
            mSCCPoint[v].first = mSCCIndex; // index
            mSCCPoint[v].second = mSCCIndex; // lowlink
            mSCCIndex++;
            mSCCStack.push_back(v);
            size_t w;

            for (const auto& edge : mGraph[v].edges)
            {
                w = edge.index;
                if (mSCCPoint[w].first == NoIndex) // not visited
                {
                    recursiveStrongConnect(w); // recurse
                    mSCCPoint[v].second = std::min(mSCCPoint[v].second, mSCCPoint[w].second);
                }
                else if (std::find(mSCCStack.begin(), mSCCStack.end(), w) != mSCCStack.end())
                    mSCCPoint[v].second = std::min(mSCCPoint[v].second, mSCCPoint[w].first);
            }

            if (mSCCPoint[v].second == mSCCPoint[v].first)
            { // new component
                do
                {
                    w = mSCCStack.back();
                    mSCCStack.pop_back();
                    mGraph[w].componentId = mSCCId;
                } while (w != v);
                mSCCId++;
            }
        }

    public:
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
        explicit Builder(PathgridGraph& graph)
            : mGraph(graph.mGraph)
        {
            // both of these are set to zero in the constructor
            // mSCCId = 0; // how many strongly connected components in this cell
            // mSCCIndex = 0;
            size_t pointsSize = graph.mPathgrid->mPoints.size();
            mSCCPoint.resize(pointsSize, std::pair<size_t, size_t>(NoIndex, NoIndex));
            mSCCStack.reserve(pointsSize);

            for (size_t v = 0; v < pointsSize; ++v)
            {
                if (mSCCPoint[v].first == NoIndex) // undefined (haven't visited)
                    recursiveStrongConnect(v);
            }
        }
    };

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
    PathgridGraph::PathgridGraph(const ESM::Pathgrid& pathgrid)
        : mPathgrid(&pathgrid)
    {
        mGraph.resize(mPathgrid->mPoints.size());
        for (const auto& edge : mPathgrid->mEdges)
        {
            ConnectedPoint neighbour;
            neighbour.cost = costAStar(mPathgrid->mPoints[edge.mV0], mPathgrid->mPoints[edge.mV1]);
            // forward path of the edge
            neighbour.index = edge.mV1;
            mGraph[edge.mV0].edges.push_back(neighbour);
            // reverse path of the edge
            // NOTE: These are redundant, ESM already contains the required reverse paths
            // neighbour.index = edge.mV0;
            // mGraph[edge.mV1].edges.push_back(neighbour);
        }
        Builder(*this);
    }

    const PathgridGraph PathgridGraph::sEmpty = {};

    bool PathgridGraph::isPointConnected(const size_t start, const size_t end) const
    {
        return (mGraph[start].componentId == mGraph[end].componentId);
    }

    void PathgridGraph::getNeighbouringPoints(const size_t index, ESM::Pathgrid::PointList& nodes) const
    {
        for (const auto& edge : mGraph[index].edges)
        {
            if (edge.index != index)
                nodes.push_back(mPathgrid->mPoints[edge.index]);
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
     * TODO: An interesting exercise might be to cache the paths created for a
     *       start/goal pair.  To cache the results the paths need to be in
     *       pathgrid points form (currently they are converted to world
     *       coordinates).  Essentially trading speed w/ memory.
     */
    std::deque<ESM::Pathgrid::Point> PathgridGraph::aStarSearch(const size_t start, const size_t goal) const
    {
        std::deque<ESM::Pathgrid::Point> path;
        if (!isPointConnected(start, goal))
        {
            return path; // there is no path, return an empty path
        }

        size_t graphSize = mGraph.size();
        std::vector<float> gScore(graphSize, -1);
        std::vector<float> fScore(graphSize, -1);
        std::vector<size_t> graphParent(graphSize, NoIndex);

        // gScore & fScore keep costs for each pathgrid point in mPoints
        gScore[start] = 0;
        fScore[start] = costAStar(mPathgrid->mPoints[start], mPathgrid->mPoints[goal]);

        std::list<size_t> openset;
        std::set<size_t> closedset;
        openset.push_back(start);

        size_t current = start;

        while (!openset.empty())
        {
            current = openset.front(); // front has the lowest cost
            openset.pop_front();

            if (current == goal)
                break;

            closedset.insert(current); // remember we've been here

            // check all edges for the current point index
            for (const auto& edge : mGraph[current].edges)
            {
                if (!closedset.contains(edge.index))
                {
                    // not in closedset - i.e. have not traversed this edge destination
                    size_t dest = edge.index;
                    float tentative_g = gScore[current] + edge.cost;
                    bool isInOpenSet = std::find(openset.begin(), openset.end(), dest) != openset.end();
                    if (!isInOpenSet || tentative_g < gScore[dest])
                    {
                        graphParent[dest] = current;
                        gScore[dest] = tentative_g;
                        fScore[dest] = tentative_g + costAStar(mPathgrid->mPoints[dest], mPathgrid->mPoints[goal]);
                        if (!isInOpenSet)
                        {
                            // add this edge to openset, lowest cost goes to the front
                            // TODO: if this causes performance problems a hash table may help
                            auto it = openset.begin();
                            for (; it != openset.end(); ++it)
                            {
                                if (fScore[*it] > fScore[dest])
                                    break;
                            }
                            openset.insert(it, dest);
                        }
                    }
                } // if in closedset, i.e. traversed this edge already, try the next edge
            }
        }

        if (current != goal)
            return path; // for some reason couldn't build a path

        // reconstruct path to return, using local coordinates
        while (graphParent[current] != NoIndex)
        {
            path.push_front(mPathgrid->mPoints[current]);
            current = graphParent[current];
        }

        // add first node to path explicitly
        path.push_front(mPathgrid->mPoints[start]);
        return path;
    }
}
