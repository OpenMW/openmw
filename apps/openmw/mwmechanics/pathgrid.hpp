#ifndef GAME_MWMECHANICS_PATHGRID_H
#define GAME_MWMECHANICS_PATHGRID_H

#include <deque>

#include <components/esm3/loadpgrd.hpp>

namespace MWMechanics
{
    class PathgridGraph
    {
        PathgridGraph()
            : mPathgrid(nullptr)
        {
        }

    public:
        explicit PathgridGraph(const ESM::Pathgrid& pathGrid);

        const ESM::Pathgrid* getPathgrid() const { return mPathgrid; }

        // returns true if end point is strongly connected (i.e. reachable
        // from start point) both start and end are pathgrid point indexes
        bool isPointConnected(const size_t start, const size_t end) const;

        // get neighbouring nodes for index node and put them to "nodes" vector
        void getNeighbouringPoints(const size_t index, ESM::Pathgrid::PointList& nodes) const;

        // the input parameters are pathgrid point indexes
        // the output list is in local (internal cells) or world (external
        // cells) coordinates
        //
        // NOTE: if start equals end an empty path is returned
        std::deque<ESM::Pathgrid::Point> aStarSearch(const size_t start, const size_t end) const;

        static const PathgridGraph sEmpty;

    private:
        const ESM::Pathgrid* mPathgrid;

        class Builder;

        struct ConnectedPoint // edge
        {
            size_t index; // pathgrid point index of neighbour
            float cost;
        };

        struct Node // point
        {
            int componentId;
            std::vector<ConnectedPoint> edges; // neighbours
        };

        // componentId is an integer indicating the groups of connected
        // pathgrid points (all connected points will have the same value)
        //
        // In Seyda Neen there are 3:
        //
        //   52, 53 and 54 are one set (enclosed yard)
        //   48, 49, 50, 51, 84, 85, 86, 87, 88, 89, 90 (ship & office)
        //   all other pathgrid points are the third set
        //
        std::vector<Node> mGraph;
    };
}

#endif
