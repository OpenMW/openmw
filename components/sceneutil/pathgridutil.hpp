#ifndef OPENMW_COMPONENTS_PATHGRIDUTIL_H
#define OPENMW_COMPONENTS_PATHGRIDUTIL_H

#include <osg/ref_ptr>
#include <osg/Geometry>

namespace ESM
{
    struct Pathgrid;
}

namespace SceneUtil
{
    osg::ref_ptr<osg::Geometry> createPathgridGeometry(const ESM::Pathgrid& pathgrid);
}

#endif
