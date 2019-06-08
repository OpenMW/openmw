#ifndef COMPONENTS_SCENEUTIL_SHADOW_H
#define COMPONENTS_SCENEUTIL_SHADOW_H

#include <osgShadow/ShadowSettings>
#include <osgShadow/ShadowedScene>

#include <components/shader/shadermanager.hpp>

#include "mwshadowtechnique.hpp"

namespace SceneUtil
{
    class ShadowManager
    {
    public:
        static void disableShadowsForStateSet(osg::ref_ptr<osg::StateSet> stateSet);

        static Shader::ShaderManager::DefineMap getShadowsDisabledDefines();

        ShadowManager(osg::ref_ptr<osg::Group> sceneRoot, osg::ref_ptr<osg::Group> rootNode, Shader::ShaderManager &shaderManager);

        virtual ~ShadowManager() = default;

        virtual void setupShadowSettings();
        osgShadow::ShadowSettings * getShadowSettings() { return mShadowSettings; }
        inline unsigned int getOutdoorShadowCastingMask() const { return mOutdoorShadowCastingMask; }
        inline void setOutdoorShadowCastingMask( unsigned int m) { mOutdoorShadowCastingMask = m; }
        inline unsigned int getIndoorShadowCastingMask() const { return mIndoorShadowCastingMask; }
        inline void setIndoorShadowCastingMask( unsigned int m) { mIndoorShadowCastingMask = m; }

        virtual Shader::ShaderManager::DefineMap getShadowDefines();

        virtual void enableIndoorMode();

        virtual void enableOutdoorMode();

        MWShadowTechnique * getShadowTechnique() { return mShadowTechnique; }
    protected:
        bool mEnableShadows;

        osg::ref_ptr<osgShadow::ShadowedScene> mShadowedScene;
        osg::ref_ptr<osgShadow::ShadowSettings> mShadowSettings;
        osg::ref_ptr<MWShadowTechnique> mShadowTechnique;

        unsigned int mOutdoorShadowCastingMask;
        unsigned int mIndoorShadowCastingMask;
        Shader::ShaderManager & mShaderManager;
    };
}

#endif //COMPONENTS_SCENEUTIL_SHADOW_H
