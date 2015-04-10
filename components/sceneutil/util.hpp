#ifndef OPENMW_COMPONENTS_SCENEUTIL_UTIL_H
#define OPENMW_COMPONENTS_SCENEUTIL_UTIL_H

#include <osg/Matrix>
#include <osg/BoundingSphere>

namespace SceneUtil
{

    // Transform a bounding sphere by a matrix
    // based off private code in osg::Transform
    // TODO: patch osg to make public
    void transformBoundingSphere (const osg::Matrix& matrix, osg::BoundingSphere& bsphere);

}

#endif
