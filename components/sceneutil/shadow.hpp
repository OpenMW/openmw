#ifndef COMPONENTS_SCENEUTIL_SHADOW_H
#define COMPONENTS_SCENEUTIL_SHADOW_H

#include <osgShadow/ShadowSettings>

#include <components/terrain/quadtreeworld.hpp>
#include <components/shader/shadermanager.hpp>

#include "mwshadowtechnique.hpp"

namespace SceneUtil
{
    class ShadowManager
    {
    public:
        static void disableShadowsForStateSet(osg::ref_ptr<osg::StateSet> stateSet);

        ShadowManager(osg::ref_ptr<osg::Group> sceneRoot, osg::ref_ptr<osg::Group> rootNode);

        virtual void setupShadowSettings(int castsShadowMask);

        virtual Shader::ShaderManager::DefineMap getShadowDefines();

        virtual Shader::ShaderManager::DefineMap getShadowsDisabledDefines();
    protected:
        const int numberOfShadowMapsPerLight;
        const bool enableShadows;

        const int baseShadowTextureUnit;

        osg::ref_ptr<osgShadow::ShadowedScene> mShadowedScene;

        osg::ref_ptr<MWShadowTechnique> mShadowTechnique;
    };
}

#endif //COMPONENTS_SCENEUTIL_SHADOW_H
