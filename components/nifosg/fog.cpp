#include "fog.hpp"

#include <osgUtil/CullVisitor>

#include <components/sceneutil/fog.hpp>

namespace NifOsg
{

    void Fog::apply(osg::StateSet* stateset, osg::NodeVisitor* nv)
    {
        float fov, aspect, near, far;
        nv->asCullVisitor()->getProjectionMatrix()->getPerspective(fov, aspect, near, far);
        SceneUtil::setFogStart(*stateset, near * mDepth + far * (1.f - mDepth));
        SceneUtil::setFogEnd(*stateset, far);
        SceneUtil::setFogColor(*stateset, mColor);
    }

}
