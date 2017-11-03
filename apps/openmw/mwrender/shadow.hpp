#ifndef OPENMW_MWRENDER_SHADOW_H
#define OPENMW_MWRENDER_SHADOW_H

#include <osgShadow/ViewDependentShadowMap>

namespace MWRender
{
    class MWShadow : public osgShadow::ViewDependentShadowMap
    {
    public:
        MWShadow();

        virtual void cull(osgUtil::CullVisitor& cv);
    protected:
        const int debugTextureUnit;

        osg::ref_ptr<osg::Camera> debugCamera;

        osg::ref_ptr<osg::Program> debugProgram;

        osg::ref_ptr<osg::Node> debugGeometry;
    };
}

#endif //OPENMW_MWRENDER_SHADOW_H
