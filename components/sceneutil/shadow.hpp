#ifndef COMPONENTS_SCENEUTIL_SHADOW_H
#define COMPONENTS_SCENEUTIL_SHADOW_H

#include <osgShadow/ShadowSettings>
#include <osgShadow/ShadowedScene>

#include <components/shader/shadermanager.hpp>
#include <components/settings/settings.hpp>

#include "mwshadowtechnique.hpp"

namespace SceneUtil
{
    class ShadowManager
    {
    public:
        static void disableShadowsForStateSet(osg::ref_ptr<osg::StateSet> stateSet);

        static Shader::ShaderManager::DefineMap getShadowsDisabledDefines();

        ShadowManager(osg::ref_ptr<osg::Group> sceneRoot, osg::ref_ptr<osg::Group> rootNode, Shader::ShaderManager* shaderManager);

        void setupShadowSettings();

        Shader::ShaderManager::DefineMap getShadowDefines();

        void processChangedSettings(const Settings::CategorySettingVector &changed, osgViewer::Viewer* viewer, bool isIndoor);

        void enableIndoorMode();

        void enableOutdoorMode();
    protected:
        bool mEnableShadows;

        osg::ref_ptr<osgShadow::ShadowedScene> mShadowedScene;
        osg::ref_ptr<osgShadow::ShadowSettings> mShadowSettings;
        osg::ref_ptr<MWShadowTechnique> mShadowTechnique;
        Shader::ShaderManager* mShaderManager;

        unsigned int mOutdoorShadowCastingMask;
        unsigned int mIndoorShadowCastingMask;
    };
}

#endif //COMPONENTS_SCENEUTIL_SHADOW_H
