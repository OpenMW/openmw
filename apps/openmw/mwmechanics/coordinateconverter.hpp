#ifndef GAME_MWMECHANICS_COORDINATECONVERTER_H
#define GAME_MWMECHANICS_COORDINATECONVERTER_H

#include <components/esm/defs.hpp>
#include <components/esm/loadpgrd.hpp>

namespace ESM
{
    struct Cell;
}

namespace MWMechanics
{
    /// \brief convert coordinates between world and local cell
    class CoordinateConverter
    {
        public:
            CoordinateConverter(const ESM::Cell* cell);

            /// in-place conversion from local to world
            void ToWorld(ESM::Pathgrid::Point& point);

            /// in-place conversion from local to world
            void ToWorld(osg::Vec3f& point);

            /// in-place conversion from world to local
            void ToLocal(osg::Vec3f& point);

            osg::Vec3f ToLocalVec3(const ESM::Pathgrid::Point& point);

        private:
            int mCellX;
            int mCellY;
    };
}

#endif
