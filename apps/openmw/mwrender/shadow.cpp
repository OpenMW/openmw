#include "shadow.hpp"

#include <osgShadow/ShadowSettings>
#include <osgShadow/ViewDependentShadowMap>

#include <components/settings/settings.hpp>

using namespace osgShadow;

namespace MWRender
{

Shader::ShaderManager::DefineMap ShadowManager::getShadowDefines()
{
    Shader::ShaderManager::DefineMap definesWithShadows;
    definesWithShadows["shadows_enabled"] = "1";
    definesWithShadows["texture_offset"] = std::to_string(mOutdoorShadowTechnique->getTextureOffset());
    definesWithShadows["num_pssm_texture"] = std::to_string(mOutdoorShadowTechnique->getSplitCount());
    definesWithShadows["pssm_texture_size"] = std::to_string(mOutdoorShadowTechnique->getTextureResolution());
    return definesWithShadows;
}

Shader::ShaderManager::DefineMap getShadowsDisabledDefines()
{
    Shader::ShaderManager::DefineMap definesWithShadows;
    definesWithShadows["shadows_enabled"] = "0";
    definesWithShadows["texture_offset"] =  "0";
    definesWithShadows["num_pssm_texture"] =  "0";
    definesWithShadows["pssm_texture_size"] = "0";
    return definesWithShadows;
}

void ShadowManager::setupShadowSettings()
{
    /*useless (for the moment current pssm don't use shadowsettings for the moment)*/
    mShadowSettings->setLightNum(0);
    mShadowSettings->setReceivesShadowTraversalMask(~0u);

    int numberOfShadowMapsPerLight = Settings::Manager::getInt("number of shadow maps", "Shadows");
    mShadowSettings->setNumShadowMapsPerLight(numberOfShadowMapsPerLight);

}

ShadowManager::ShadowManager(osg::Group* parent, osg::Group* sceneRoot,
                             unsigned int outdoorShadowCastingMask,
                             unsigned int indoorShadowCastingMask,
                             Shader::ShaderManager &shaderManager) :
    mShadowedScene(new osgShadow::ShadowedScene),
    mOutdoorShadowCastingMask(outdoorShadowCastingMask),
    mIndoorShadowCastingMask(indoorShadowCastingMask)
{
    mShadowedScene->addChild(sceneRoot);
    parent->addChild(mShadowedScene);
    mShadowSettings = mShadowedScene->getShadowSettings();
    mShadowSettings->setReceivesShadowTraversalMask(~0u);

    mEnableShadows=Settings::Manager::getBool("enable shadows", "Shadows");
    Shader::ShaderManager::DefineMap shadowDefines;
    if(mEnableShadows) {
        ///PSSM Setup
        ParallelSplitShadowMap* pssm=new ParallelSplitShadowMap;

        float ftemp = Settings::Manager::getFloat("pssm distlight", "Shadows");
        if(ftemp>0) pssm->setMinNearDistanceForSplits(ftemp);
        int itemp = Settings::Manager::getInt("pssm textures resolution", "Shadows");
        if(itemp>0) pssm->setTextureResolution(itemp);
        itemp = Settings::Manager::getInt("pssm shadowmap count", "Shadows");
        if(itemp>0) pssm->setSplitCount(itemp);
        itemp = Settings::Manager::getInt("pssm texunitoffset", "Shadows");
        if(itemp>0) pssm->setTextureOffset(itemp);
        ftemp = Settings::Manager::getFloat("pssm shadow ambient", "Shadows");
        if(ftemp>0)
        {
            osg::Vec2 ambiant=pssm->getAmbientBias();
            ambiant.y()=ftemp;
            pssm->setAmbientBias(ambiant);
        }
        float unit=Settings::Manager::getFloat("pssm polygon offset units", "Shadows");
        float factor=Settings::Manager::getFloat("pssm polygon offset factor", "Shadows");
        if(unit!=0||factor!=0)
            pssm->setPolygonOffset(osg::Vec2(factor, unit));

        mOutdoorShadowTechnique=pssm;

        //TODO IndoorTechnique
        //setupShadowSettings();
        enableOutdoorMode();
        shadowDefines = getShadowDefines();
    }
    else shadowDefines = getShadowsDisabledDefines();

    Shader::ShaderManager::DefineMap globalDefines = shaderManager.getGlobalDefines();

    for (auto itr = shadowDefines.begin(); itr != shadowDefines.end(); itr++)
        globalDefines[itr->first] = itr->second;
    shaderManager.setGlobalDefines(globalDefines);
    shaderManager.lockGlobalDefines();

}

void ShadowManager::enableIndoorMode()
{
    //mShadowedScene->setShadowTechnique(mIndoorShadowTechnique);
    mShadowSettings->setCastsShadowTraversalMask(mIndoorShadowCastingMask);
}
void ShadowManager::enableOutdoorMode()
{
    mShadowedScene->setShadowTechnique(mOutdoorShadowTechnique);
    mShadowSettings->setCastsShadowTraversalMask(mOutdoorShadowCastingMask);
}
}
