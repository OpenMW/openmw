#ifndef OPENMW_COMPONENTS_WATERUTIL_H
#define OPENMW_COMPONENTS_WATERUTIL_H

#include <osg/ref_ptr>

namespace osg
{
    class Geometry;
    class StateSet;
}

namespace SceneUtil
{
    osg::ref_ptr<osg::Geometry> createWaterGeometry(float size, int segments, float textureRepeats);

    osg::ref_ptr<osg::StateSet> createSimpleWaterStateSet(float alpha, int renderBin);
}

#endif
