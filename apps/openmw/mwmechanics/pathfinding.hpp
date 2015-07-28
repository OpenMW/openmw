#ifndef GAME_MWMECHANICS_PATHFINDING_H
#define GAME_MWMECHANICS_PATHFINDING_H

#include <components/esm/defs.hpp>
#include <components/esm/loadpgrd.hpp>
#include <list>

namespace MWWorld
{
    class CellStore;
}

namespace MWMechanics
{
    float distance(const ESM::Pathgrid::Point& point, float x, float y, float);
    float distance(const ESM::Pathgrid::Point& a, const ESM::Pathgrid::Point& b);
    class PathFinder
    {
        public:
            PathFinder();

            static const int PathTolerance = 32;

            static float sgn(float val)
            {
                if(val > 0)
                    return 1.0;
                return -1.0;
            }

            static int sgn(int a)
            {
                if(a > 0)
                    return 1;
                return -1;
            }

            void clearPath();

            bool checkPathCompleted(float x, float y, float tolerance = PathTolerance);
            ///< \Returns true if we are within \a tolerance units of the last path point.

            /// In radians
            float getZAngleToNext(float x, float y) const;

            bool isPathConstructed() const
            {
                return !mPath.empty();
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
                BuildPath() takes closest PathGrid point to NPC as first point of path.
                This is undesireable if NPC has just passed a Pathgrid point, as this
                makes the 2nd point of the new path == the 1st point of old path.
                Which results in NPC "running in a circle" back to the just passed waypoint.
             */
            void buildSyncedPath(const ESM::Pathgrid::Point &startPoint, const ESM::Pathgrid::Point &endPoint,
                const MWWorld::CellStore* cell, bool allowShortcuts = true);

            void addPointToPath(ESM::Pathgrid::Point &point)
            {
                mPath.push_back(point);
            }

            /// utility function to convert a osg::Vec3f to a Pathgrid::Point
            static ESM::Pathgrid::Point MakePathgridPoint(const osg::Vec3f& v)
            {
                return ESM::Pathgrid::Point(static_cast<int>(v[0]), static_cast<int>(v[1]), static_cast<int>(v[2]));
            }

            /// utility function to convert an ESM::Position to a Pathgrid::Point
            static ESM::Pathgrid::Point MakePathgridPoint(const ESM::Position& p)
            {
                return ESM::Pathgrid::Point(static_cast<int>(p.pos[0]), static_cast<int>(p.pos[1]), static_cast<int>(p.pos[2]));
            }

            static osg::Vec3f MakeOsgVec3(const ESM::Pathgrid::Point& p)
            {
                return osg::Vec3f(static_cast<float>(p.mX), static_cast<float>(p.mY), static_cast<float>(p.mZ));
            }

        private:
            void buildPath(const ESM::Pathgrid::Point &startPoint, const ESM::Pathgrid::Point &endPoint,
                const MWWorld::CellStore* cell, bool allowShortcuts = true);

            std::list<ESM::Pathgrid::Point> mPath;

            const ESM::Pathgrid *mPathgrid;
            const MWWorld::CellStore* mCell;
    };
}

#endif
