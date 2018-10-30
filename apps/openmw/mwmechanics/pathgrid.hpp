#ifndef GAME_MWMECHANICS_PATHGRID_H
#define GAME_MWMECHANICS_PATHGRID_H

#include <deque>

#include <components/esm/loadpgrd.hpp>

namespace ESM
{
    struct Cell;
}

namespace MWWorld
{
    class CellStore;
}

namespace MWMechanics
{
    class PathgridGraph
    {
        public:
            PathgridGraph(const MWWorld::CellStore* cell);

            bool load(const MWWorld::CellStore *cell);

            const ESM::Pathgrid* getPathgrid() const;

            // returns true if end point is strongly connected (i.e. reachable
            // from start point) both start and end are pathgrid point indexes
            bool isPointConnected(const int start, const int end) const;

            // get neighbouring nodes for index node and put them to "nodes" vector
            void getNeighbouringPoints(const int index, ESM::Pathgrid::PointList &nodes) const;

            // the input parameters are pathgrid point indexes
            // the output list is in local (internal cells) or world (external
            // cells) coordinates
            //
            // NOTE: if start equals end an empty path is returned
            std::deque<ESM::Pathgrid::Point> aStarSearch(const int start, const int end) const;

        private:

            const ESM::Cell *mCell;
            const ESM::Pathgrid *mPathgrid;
            bool mIsExterior;

            struct ConnectedPoint // edge
            {
                int index; // pathgrid point index of neighbour
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
            bool mIsGraphConstructed;

            // variables used to calculate connected components
            int mSCCId;
            int mSCCIndex;
            std::vector<int> mSCCStack;
            typedef std::pair<int, int> VPair; // first is index, second is lowlink
            std::vector<VPair> mSCCPoint;
            // methods used to calculate connected components
            void recursiveStrongConnect(int v);
            void buildConnectedPoints();
    };
}

#endif
