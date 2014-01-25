#ifndef GAME_MWMECHANICS_PATHFINDING_H
#define GAME_MWMECHANICS_PATHFINDING_H

#include <components/esm/loadpgrd.hpp>
#include <list>

namespace MWMechanics
{
    class PathFinder
    {
        public:
            PathFinder();

            void clearPath();
            void buildPath(const ESM::Pathgrid::Point &startPoint, const ESM::Pathgrid::Point &endPoint,
                           const ESM::Pathgrid* pathGrid, float xCell = 0, float yCell = 0,
                           bool allowShortcuts = true);

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

        private:
            std::list<ESM::Pathgrid::Point> mPath;
            bool mIsPathConstructed;
    };
}

#endif
