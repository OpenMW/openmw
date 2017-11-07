#ifndef OPENMW_MWRENDER_SHADOW_H
#define OPENMW_MWRENDER_SHADOW_H

#include <osgShadow/ViewDependentShadowMap>

namespace MWRender
{
    class MWShadow : public osgShadow::ViewDependentShadowMap
    {
    public:
        static const int numberOfShadowMapsPerLight = 2;
        static const bool debugHud = true;

        MWShadow();

        virtual void cull(osgUtil::CullVisitor& cv);
    protected:
        const int debugTextureUnit;

        std::vector<osg::ref_ptr<osg::Camera>> debugCameras;

        osg::ref_ptr<osg::Program> debugProgram;

        std::vector<osg::ref_ptr<osg::Node>> debugGeometry;
    };
}

#endif //OPENMW_MWRENDER_SHADOW_H
