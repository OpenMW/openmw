#ifndef OPENMW_COMPONENTS_PATHGRIDUTIL_H
#define OPENMW_COMPONENTS_PATHGRIDUTIL_H

#include <osg/Geometry>
#include <osg/ref_ptr>

namespace ESM
{
    struct Pathgrid;
}

namespace SceneUtil
{
    constexpr float DiamondHalfHeight = 40.f;
    constexpr float DiamondHalfWidth = 16.f;

    osg::ref_ptr<osg::Geometry> createPathgridGeometry(const ESM::Pathgrid& pathgrid);

    osg::ref_ptr<osg::Geometry> createPathgridSelectedWireframe(
        const ESM::Pathgrid& pathgrid, const std::vector<unsigned short>& selected);

    unsigned short getPathgridNode(unsigned vertexIndex);
}

#endif
