#include "shadow.hpp"

namespace MWRender
{
    void MWShadow::ViewData::init(MWShadow * st, osgUtil::CullVisitor * cv)
    {
        LightSpacePerspectiveShadowMapDB::ViewData::init(st, cv);
        osg::StateSet * stateset = _camera->getOrCreateStateSet();
        stateset->removeAttribute(osg::StateAttribute::CULLFACE);
    }
}
