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

        ShadowManager(osg::ref_ptr<osg::Group> sceneRoot, osg::ref_ptr<osg::Group> rootNode, unsigned int outdoorShadowCastingMask, unsigned int indoorShadowCastingMask);

        virtual void setupShadowSettings();

        virtual Shader::ShaderManager::DefineMap getShadowDefines();

        virtual Shader::ShaderManager::DefineMap getShadowsDisabledDefines();

        virtual void enableIndoorMode();

        virtual void enableOutdoorMode();
    protected:
        bool mEnableShadows;

        unsigned int mOutdoorShadowCastingMask;
        unsigned int mIndoorShadowCastingMask;

        osg::ref_ptr<osgShadow::ShadowedScene> mShadowedScene;
        osg::ref_ptr<osgShadow::ShadowSettings> mShadowSettings;
        osg::ref_ptr<MWShadowTechnique> mShadowTechnique;
    };
}

#endif //COMPONENTS_SCENEUTIL_SHADOW_H
