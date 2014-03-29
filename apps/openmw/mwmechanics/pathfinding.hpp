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

            void buildPathgridGraph(const ESM::Pathgrid* pathGrid);

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

            // While a public method is defined here, it is anticipated that
            // mSCComp will only be used internally.
            std::vector<int> getSCComp() const
            {
                return mSCComp;
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

            // contains an integer indicating the groups of connected pathgrid points
            // (all connected points will have the same value)
            //
            // In Seyda Neen there are 3:
            //
            //   52, 53 and 54 are one set (enclosed yard)
            //   48, 49, 50, 51, 84, 85, 86, 87, 88, 89, 90 are another (ship & office)
            //   all other pathgrid points are the third set
            //
            std::vector<int> mSCComp;
            // variables used to calculate mSCComp
            int mSCCId;
            int mSCCIndex;
            std::list<int> mSCCStack;
            typedef std::pair<int, int> VPair; // first is index, second is lowlink
            std::vector<VPair> mSCCPoint;
            // methods used to calculate mSCComp
            void recursiveStrongConnect(int v);
            void buildConnectedPoints(const ESM::Pathgrid* pathGrid);
    };
}

#endif
