#ifndef GAME_MWMECHANICS_PATHFINDING_H
#define GAME_MWMECHANICS_PATHFINDING_H

#include <components/esm/loadpgrd.hpp>
#include <list>

#include <OgreMath.h>

namespace MWWorld
{
    class CellStore;
}

namespace MWMechanics
{
    float distance(ESM::Pathgrid::Point point, float x, float y, float);
    float distance(ESM::Pathgrid::Point a, ESM::Pathgrid::Point b);
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

            const std::list<ESM::Pathgrid::Point>& getPath() const
            {
                return mPath;
            }

            /** Synchronize new path with old one to avoid visiting 1 waypoint 2 times
            @note
                If the first point is chosen as the nearest one
                the situation can occur when the 1st point of the new path is undesirable
                (i.e. the 2nd point of new path == the 1st point of old path).
            @param path - old path
            @return true if such point was found and deleted
             */
            bool syncStart(const std::list<ESM::Pathgrid::Point> &path);

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
}

#endif
