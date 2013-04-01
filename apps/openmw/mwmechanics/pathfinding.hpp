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

        void buildPath(ESM::Pathgrid::Point startPoint,ESM::Pathgrid::Point endPoint,
            const ESM::Pathgrid* pathGrid,float xCell = 0,float yCell = 0);

        bool checkIfNextPointReached(float x,float y,float z);//returns true if the last point of the path has been reached.
        float getZAngleToNext(float x,float y,float z);

        std::list<ESM::Pathgrid::Point> getPath();
        bool isPathConstructed();

    private:
        std::list<ESM::Pathgrid::Point> mPath;
        bool mIsPathConstructed;
    };
}

#endif