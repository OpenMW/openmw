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

        /// @return true if Shadowing is enabled
        inline bool isShadowEnable() const { return mEnableShadows;}

        /// @return current shadowStateSet (to reuse shadowmap result map&texgen)
        osg::StateSet * getShadowedStateSet(){ return mOutdoorShadowTechnique->getShadowedStateSet(); }

        /// @return an uniform to nullify shadow
        static osg::Uniform* getUnshadowedUniform();

        void enableIndoorMode();

        void enableOutdoorMode();

        osgShadow::ShadowSettings* getShadowSettings(){return mShadowSettings;}
    protected:
        static osg::ref_ptr<osg::Uniform> _unshadowedAmbientBias;
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
