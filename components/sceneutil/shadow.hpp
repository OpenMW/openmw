#ifndef COMPONENTS_SCENEUTIL_SHADOW_H
#define COMPONENTS_SCENEUTIL_SHADOW_H

#include <osgShadow/ShadowSettings>
#include <osgShadow/ViewDependentShadowMap>

#include <components/shader/shadermanager.hpp>

namespace SceneUtil
{
    class MWShadow : public osgShadow::ViewDependentShadowMap
    {
    public:
        static void setupShadowSettings(osg::ref_ptr<osgShadow::ShadowSettings> settings, int castsShadowMask);

        static void disableShadowsForStateSet(osg::ref_ptr<osg::StateSet> stateSet);

        MWShadow();

        virtual void cull(osgUtil::CullVisitor& cv);

        virtual Shader::ShaderManager::DefineMap getShadowDefines();

        virtual Shader::ShaderManager::DefineMap getShadowsDisabledDefines();
    protected:
        const int debugTextureUnit;

        std::vector<osg::ref_ptr<osg::Camera>> debugCameras;

        osg::ref_ptr<osg::Program> debugProgram;

        std::vector<osg::ref_ptr<osg::Node>> debugGeometry;

        const int numberOfShadowMapsPerLight;
        const bool enableShadows;
        const bool debugHud;

        const int baseShadowTextureUnit;
    };
}

#endif //COMPONENTS_SCENEUTIL_SHADOW_H
