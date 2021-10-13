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

namespace SceneUtil
{
    class MWShadowTechnique;
    class ShadowManager
    {
    public:
        static void disableShadowsForStateSet(osg::ref_ptr<osg::StateSet> stateSet);

        static Shader::ShaderManager::DefineMap getShadowsDisabledDefines();

        ShadowManager(osg::ref_ptr<osg::Group> sceneRoot, osg::ref_ptr<osg::Group> rootNode, unsigned int outdoorShadowCastingMask, unsigned int indoorShadowCastingMask, unsigned int worldMask, Shader::ShaderManager &shaderManager);
        ~ShadowManager();

        void setupShadowSettings();

        Shader::ShaderManager::DefineMap getShadowDefines();

        void enableIndoorMode();

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

#endif //COMPONENTS_SCENEUTIL_SHADOW_H
