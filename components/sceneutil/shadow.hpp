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
    protected:
        static ShadowManager* _instance;
        ShadowManager(osg::Group* sceneRoot, osg::Group* rootNode, Shader::ShaderManager *shaderManager);
    public:
        static ShadowManager* get(osg::Group* sceneRoot=0, osg::Group* rootNode=0, Shader::ShaderManager *shaderManager=0)
        {
            if(!_instance)
                _instance = new ShadowManager(sceneRoot, rootNode, shaderManager);
            return _instance;
        }

        void disableShadowsForStateSet(osg::StateSet* stateSet);

        static Shader::ShaderManager::DefineMap getShadowsDisabledDefines();

        void setupShadowSettings();

        osgShadow::ShadowSettings * getShadowSettings() { return mShadowSettings; }
        inline unsigned int getOutdoorShadowCastingMask() const { return mOutdoorShadowCastingMask; }
        inline void setOutdoorShadowCastingMask( unsigned int m) { mOutdoorShadowCastingMask = m; }
        inline unsigned int getIndoorShadowCastingMask() const { return mIndoorShadowCastingMask; }
        inline void setIndoorShadowCastingMask( unsigned int m) { mIndoorShadowCastingMask = m; }

        Shader::ShaderManager::DefineMap getShadowDefines();

        MWShadowTechnique * getShadowTechnique() { return mShadowTechnique; }

        void enableIndoorMode();
        void enableOutdoorMode();

    protected:
        bool mEnableShadows;

        osg::ref_ptr<osgShadow::ShadowedScene> mShadowedScene;
        osg::ref_ptr<osgShadow::ShadowSettings> mShadowSettings;
        osg::ref_ptr<MWShadowTechnique> mShadowTechnique;

        unsigned int mOutdoorShadowCastingMask;
        unsigned int mIndoorShadowCastingMask;
        Shader::ShaderManager & mShaderManager;
        typedef std::vector<osg::observer_ptr<osg::StateSet> > RegisteredUnshadowedStateSet;
        RegisteredUnshadowedStateSet _registeredUnshadowedStateSet;
    };
}

#endif //COMPONENTS_SCENEUTIL_SHADOW_H
