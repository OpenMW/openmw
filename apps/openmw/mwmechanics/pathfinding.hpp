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
}

#endif
