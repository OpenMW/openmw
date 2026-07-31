#include "fog.hpp"

#include <osgUtil/CullVisitor>

#include <components/sceneutil/fog.hpp>

namespace NifOsg
{

    void Fog::apply(osg::StateSet* stateset, osg::NodeVisitor* nv)
    {
        SceneUtil::FogUniformController fog{ *stateset };
        float fov, aspect, near, far;
        nv->asCullVisitor()->getProjectionMatrix()->getPerspective(fov, aspect, near, far);
        fog.setStart(near * mDepth + far * (1.f - mDepth));
        fog.setEnd(far);
        fog.setColor(mColor);
    }

}
