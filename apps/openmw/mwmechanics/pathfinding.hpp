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

            void buildPathgridGraph(const ESM::Pathgrid* pathGrid,float xCell = 0, float yCell = 0);

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
            std::list<ESM::Pathgrid::Point> mPath;
            bool mIsPathConstructed;

            typedef boost::adjacency_list< boost::vecS, boost::vecS, boost::undirectedS,
                boost::property<boost::vertex_index_t, int, ESM::Pathgrid::Point>, boost::property<boost::edge_weight_t, float> >
                PathGridGraph;
            PathGridGraph mGraph;
            bool mIsGraphConstructed;
            const MWWorld::CellStore* mCell;
    };
}

#endif
