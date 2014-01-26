#ifndef GAME_MWMECHANICS_PATHFINDING_H
#define GAME_MWMECHANICS_PATHFINDING_H

#include <components/esm/loadpgrd.hpp>
#include <list>
#include <boost/graph/adjacency_list.hpp>

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

            void clearPath();

            void buildPathgridGraph(const ESM::Pathgrid* pathGrid);

            void buildPath(const ESM::Pathgrid::Point &startPoint, const ESM::Pathgrid::Point &endPoint,
                           const MWWorld::CellStore* cell, bool allowShortcuts = true);

            bool checkPathCompleted(float x, float y, float z);
            ///< \Returns true if the last point of the path has been reached.
            bool checkWaypoint(float x, float y, float z);
            ///< \Returns true if a way point was reached
            float getZAngleToNext(float x, float y) const;

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

            void addPointToPath(ESM::Pathgrid::Point &point)
            {
                mPath.push_back(point);
            }

        private:

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

            std::vector<float> mGScore;
            std::vector<float> mFScore;

            std::list<ESM::Pathgrid::Point> aStarSearch(const ESM::Pathgrid* pathGrid,int start,int goal,float xCell = 0, float yCell = 0);
            void cleanUpAStar();

            std::vector<Node> mGraph;
            bool mIsPathConstructed;


            std::list<ESM::Pathgrid::Point> mPath;
            bool mIsGraphConstructed;
            const MWWorld::CellStore* mCell;
    };
}

#endif
