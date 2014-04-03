#ifndef GAME_MWMECHANICS_PATHFINDING_H
#define GAME_MWMECHANICS_PATHFINDING_H

#include <components/esm/loadpgrd.hpp>
#include <components/esm/loadcell.hpp>
#include <list>

#include <OgreMath.h>

namespace MWWorld
{
    class CellStore;
}

namespace MWMechanics
{
    class PathFinder
    {
        public:
            PathFinder();

            static float sgn(Ogre::Radian a)
            {
                if(a.valueRadians() > 0)
                    return 1.0;
                return -1.0;
            }

            static float sgn(float a)
            {
                if(a > 0)
                    return 1.0;
                return -1.0;
            }

            void clearPath();

            void buildPath(const ESM::Pathgrid::Point &startPoint, const ESM::Pathgrid::Point &endPoint,
                           const MWWorld::CellStore* cell, bool allowShortcuts = true);

            bool checkPathCompleted(float x, float y, float z);
            ///< \Returns true if the last point of the path has been reached.

            bool checkWaypoint(float x, float y, float z);
            ///< \Returns true if a way point was reached

            float getZAngleToNext(float x, float y) const;

            float getDistToNext(float x, float y, float z);

            bool isPathConstructed() const
            {
                return mIsPathConstructed;
            }

            int getPathSize() const
            {
                return mPath.size();
            }

            std::list<ESM::Pathgrid::Point> getPath() const
            {
                return mPath;
            }

            // When first point of newly created path is the nearest to actor point,
            // then a situation can occure when this point is undesirable
            // (if the 2nd point of new path == the 1st point of old path)
            // This functions deletes that point.
            void syncStart(const std::list<ESM::Pathgrid::Point> &path);

            void addPointToPath(ESM::Pathgrid::Point &point)
            {
                mPath.push_back(point);
            }

        private:

            bool mIsPathConstructed;

            std::list<ESM::Pathgrid::Point> mPath;

            const ESM::Pathgrid *mPathgrid;
            const MWWorld::CellStore* mCell;
    };

    class PathgridGraph
    {
        public:
            PathgridGraph();

            bool isGraphConstructed() const
            {
                return mIsGraphConstructed;
            };

            bool initPathgridGraph(const ESM::Cell *cell);

            // returns true if end point is strongly connected (i.e. reachable
            // from start point) both start and end are pathgrid point indexes
            bool isPointConnected(const int start, const int end) const;

            // isOutside is used whether to convert path to world co-ordinates
            std::list<ESM::Pathgrid::Point> aStarSearch(const int start, const int end,
                                                        const bool isOutside) const;
        private:

            const ESM::Cell *mCell;
            const ESM::Pathgrid *mPathgrid;

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
