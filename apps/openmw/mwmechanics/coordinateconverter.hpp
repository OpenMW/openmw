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
            void toWorld(ESM::Pathgrid::Point& point);

            /// in-place conversion from local to world
            void toWorld(osg::Vec3f& point);

            /// in-place conversion from world to local
            void toLocal(osg::Vec3f& point);

            osg::Vec3f toLocalVec3(const osg::Vec3f& point);

        private:
            int mCellX;
            int mCellY;
    };
}

#endif
