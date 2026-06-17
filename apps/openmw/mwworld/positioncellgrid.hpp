#ifndef OPENMW_APPS_OPENMW_MWWORLD_POSITIONCELLGRID_H
#define OPENMW_APPS_OPENMW_MWWORLD_POSITIONCELLGRID_H

#include <osg/Vec3f>
#include <osg/Vec4i>

namespace MWWorld
{
    struct PositionCellGrid
    {
        osg::Vec3f mPosition;
        osg::Vec4i mCellBounds;
    };
}

#endif
