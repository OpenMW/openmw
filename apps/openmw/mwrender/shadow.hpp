#ifndef OPENMW_MWRENDER_SHADOW_H
#define OPENMW_MWRENDER_SHADOW_H

#include <osgShadow/ShadowSettings>
#include <osgShadow/ShadowedScene>
#include <components/shader/shadermanager.hpp>
#include "parallelsplitshadowmap.hpp"

namespace MWRender
{
    class ShadowManager
    {
    public:
        ShadowManager(osg::Group* parent, osg::Group* sceneRoot,
                      unsigned int outdoorShadowCastingMask,
                      unsigned int indoorShadowCastingMask,
                      Shader::ShaderManager &shaderManager);
        ~ShadowManager() = default;

        void enableIndoorMode();

        void enableOutdoorMode();

    protected:
        void setupShadowSettings();
        Shader::ShaderManager::DefineMap getShadowDefines();

        bool mEnableShadows;
        osg::ref_ptr<osgShadow::ShadowedScene> mShadowedScene;
        osg::ref_ptr<osgShadow::ShadowSettings> mShadowSettings;
        osg::ref_ptr<osgShadow::ShadowTechnique> mIndoorShadowTechnique;
        osg::ref_ptr<ParallelSplitShadowMap> mOutdoorShadowTechnique;
        unsigned int mOutdoorShadowCastingMask;
        unsigned int mIndoorShadowCastingMask;
    };
}

#endif //OPENMW_MWRENDER_SHADOW_H
