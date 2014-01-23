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

            //When first point of newly created path is the nearest to actor point, then
            //the cituation can occure when this point is undesirable (if the 2nd point of new path == the 1st point of old path)
            //This functions deletes that point.
            void syncStart(const std::list<ESM::Pathgrid::Point> &path);

        private:
            std::list<ESM::Pathgrid::Point> mPath;
            bool mIsPathConstructed;
    };
}

#endif
