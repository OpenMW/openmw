#ifndef COMPONENTS_SCENEUTIL_SHADOW_H
#define COMPONENTS_SCENEUTIL_SHADOW_H

#include <components/shader/shadermanager.hpp>

namespace osg
{
    class StateSet;
    class Group;
}

namespace osgShadow
{
    class ShadowSettings;
    class ShadowedScene;
}

namespace Settings
{
    struct ShadowsCategory;
}

namespace SceneUtil
{
    class MWShadowTechnique;
    class ShadowManager
    {
    public:
        static void disableShadowsForStateSet(const Settings::ShadowsCategory& settings, osg::StateSet& stateset);

        static Shader::ShaderManager::DefineMap getShadowsDisabledDefines();

        explicit ShadowManager(osg::ref_ptr<osg::Group> sceneRoot, osg::ref_ptr<osg::Group> rootNode,
            unsigned int outdoorShadowCastingMask, unsigned int indoorShadowCastingMask, unsigned int worldMask,
            const Settings::ShadowsCategory& settings, Shader::ShaderManager& shaderManager);
        ~ShadowManager();

        void setupShadowSettings(const Settings::ShadowsCategory& settings, Shader::ShaderManager& shaderManager);

        Shader::ShaderManager::DefineMap getShadowDefines(const Settings::ShadowsCategory& settings);

        void enableIndoorMode(const Settings::ShadowsCategory& settings);

        void enableOutdoorMode();

    protected:
        bool mEnableShadows;

        osg::ref_ptr<osgShadow::ShadowedScene> mShadowedScene;
        osg::ref_ptr<osgShadow::ShadowSettings> mShadowSettings;
        osg::ref_ptr<MWShadowTechnique> mShadowTechnique;

        unsigned int mOutdoorShadowCastingMask;
        unsigned int mIndoorShadowCastingMask;
    };
}

#endif // COMPONENTS_SCENEUTIL_SHADOW_H
