#ifndef COMPONENTS_SCENEUTIL_SHADOW_H
#define COMPONENTS_SCENEUTIL_SHADOW_H

#include <osgShadow/ViewDependentShadowMap>

#include <components/shader/shadermanager.hpp>

namespace SceneUtil
{
    class MWShadow : public osgShadow::ViewDependentShadowMap
    {
    public:
        static const int numberOfShadowMapsPerLight = 3;
        static const bool enableShadows = true;
        static const bool debugHud = true;

        MWShadow();

        const static int baseShadowTextureUnit = 8 - numberOfShadowMapsPerLight;

        virtual void cull(osgUtil::CullVisitor& cv);

        virtual Shader::ShaderManager::DefineMap getShadowDefines();

        virtual Shader::ShaderManager::DefineMap getShadowsDisabledDefines();
    protected:
        const int debugTextureUnit;

        std::vector<osg::ref_ptr<osg::Camera>> debugCameras;

        osg::ref_ptr<osg::Program> debugProgram;

        std::vector<osg::ref_ptr<osg::Node>> debugGeometry;
    };
}

#endif //COMPONENTS_SCENEUTIL_SHADOW_H
