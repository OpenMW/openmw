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
    const float DiamondHalfHeight = 40.f;
    const float DiamondHalfWidth = 16.f;

    osg::ref_ptr<osg::Geometry> createPathgridGeometry(const ESM::Pathgrid& pathgrid);

    osg::ref_ptr<osg::Geometry> createPathgridSelectedWireframe(const ESM::Pathgrid& pathgrid,
        const std::vector<unsigned short>& selected);

    unsigned short getPathgridNode(unsigned short vertexIndex);
}

#endif
