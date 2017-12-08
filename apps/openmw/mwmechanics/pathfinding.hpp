#ifndef GAME_MWMECHANICS_PATHFINDING_H
#define GAME_MWMECHANICS_PATHFINDING_H

#include <list>
#include <cassert>

#include <components/esm/defs.hpp>
#include <components/esm/loadpgrd.hpp>

namespace MWWorld
{
    class CellStore;
}

namespace MWMechanics
{
    class PathgridGraph;

    float distance(const ESM::Pathgrid::Point& point, float x, float y, float);
    float distance(const ESM::Pathgrid::Point& a, const ESM::Pathgrid::Point& b);
    float getZAngleToDir(const osg::Vec3f& dir);
    float getXAngleToDir(const osg::Vec3f& dir);
    float getZAngleToPoint(const ESM::Pathgrid::Point &origin, const ESM::Pathgrid::Point &dest);
    float getXAngleToPoint(const ESM::Pathgrid::Point &origin, const ESM::Pathgrid::Point &dest);

     const float PATHFIND_Z_REACH = 50.0f;
    //static const float sMaxSlope = 49.0f; // duplicate as in physicssystem
    // distance after which actor (failed previously to shortcut) will try again
    const float PATHFIND_SHORTCUT_RETRY_DIST = 300.0f;

    // cast up-down ray with some offset from actor position to check for pits/obstacles on the way to target;
    // magnitude of pits/obstacles is defined by PATHFIND_Z_REACH
    bool checkWayIsClear(const osg::Vec3f& from, const osg::Vec3f& to, float offsetXY);

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

            void buildPath(const ESM::Pathgrid::Point &startPoint, const ESM::Pathgrid::Point &endPoint,
                           const MWWorld::CellStore* cell, const PathgridGraph& pathgridGraph);

            bool checkPathCompleted(float x, float y, float tolerance = PathTolerance);
            ///< \Returns true if we are within \a tolerance units of the last path point.

            /// In radians
            float getZAngleToNext(float x, float y) const;

            float getXAngleToNext(float x, float y, float z) const;

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

            const MWWorld::CellStore* getPathCell() const;

            /** Synchronize new path with old one to avoid visiting 1 waypoint 2 times
            @note
                BuildPath() takes closest PathGrid point to NPC as first point of path.
                This is undesirable if NPC has just passed a Pathgrid point, as this
                makes the 2nd point of the new path == the 1st point of old path.
                Which results in NPC "running in a circle" back to the just passed waypoint.
             */
            void buildSyncedPath(const ESM::Pathgrid::Point &startPoint, const ESM::Pathgrid::Point &endPoint,
                const MWWorld::CellStore* cell, const PathgridGraph& pathgridGraph);

            void addPointToPath(const ESM::Pathgrid::Point &point)
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
            
            // Slightly cheaper version for comparisons.
            // Caller needs to be careful for very short distances (i.e. less than 1)
            // or when accumuating the results i.e. (a + b)^2 != a^2 + b^2
            //
            static float DistanceSquared(ESM::Pathgrid::Point point, const osg::Vec3f& pos)
            {
                return (MWMechanics::PathFinder::MakeOsgVec3(point) - pos).length2();
            }

            // Return the closest pathgrid point index from the specified position
            // coordinates.  NOTE: Does not check if there is a sensible way to get there
            // (e.g. a cliff in front).
            //
            // NOTE: pos is expected to be in local coordinates, as is grid->mPoints
            //
            static int GetClosestPoint(const ESM::Pathgrid* grid, const osg::Vec3f& pos)
            {
                assert(grid && !grid->mPoints.empty());

                float distanceBetween = DistanceSquared(grid->mPoints[0], pos);
                int closestIndex = 0;

                // TODO: if this full scan causes performance problems mapping pathgrid
                //       points to a quadtree may help
                for(unsigned int counter = 1; counter < grid->mPoints.size(); counter++)
                {
                    float potentialDistBetween = DistanceSquared(grid->mPoints[counter], pos);
                    if(potentialDistBetween < distanceBetween)
                    {
                        distanceBetween = potentialDistBetween;
                        closestIndex = counter;
                    }
                }

                return closestIndex;
            }

        private:
            std::list<ESM::Pathgrid::Point> mPath;

            const ESM::Pathgrid *mPathgrid;
            const MWWorld::CellStore* mCell;
    };
}

#endif
