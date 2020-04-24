#include "shadow.hpp"

#include <osgShadow/ShadowedScene>

#include <components/settings/settings.hpp>

namespace SceneUtil
{
    using namespace osgShadow;

    void ShadowManager::setupShadowSettings()
    {
        mEnableShadows = Settings::Manager::getBool("enable shadows", "Shadows");

        if (!mEnableShadows)
        {
            mShadowTechnique->disableShadows();
            return;
        }
        
        mShadowTechnique->enableShadows();

        mShadowSettings->setLightNum(0);
        mShadowSettings->setReceivesShadowTraversalMask(~0u);

        int numberOfShadowMapsPerLight = Settings::Manager::getInt("number of shadow maps", "Shadows");
        numberOfShadowMapsPerLight = std::max(1, std::min(numberOfShadowMapsPerLight, 8));

        mShadowSettings->setNumShadowMapsPerLight(numberOfShadowMapsPerLight);
        mShadowSettings->setBaseShadowTextureUnit(8 - numberOfShadowMapsPerLight);

        const float maximumShadowMapDistance = Settings::Manager::getFloat("maximum shadow map distance", "Shadows");
        if (maximumShadowMapDistance > 0)
        {
            const float shadowFadeStart = std::min(std::max(0.f, Settings::Manager::getFloat("shadow fade start", "Shadows")), 1.f);
            mShadowSettings->setMaximumShadowMapDistance(maximumShadowMapDistance);
            mShadowTechnique->setShadowFadeStart(maximumShadowMapDistance * shadowFadeStart);
        }

        mShadowSettings->setMinimumShadowMapNearFarRatio(Settings::Manager::getFloat("minimum lispsm near far ratio", "Shadows"));
        if (Settings::Manager::getBool("compute tight scene bounds", "Shadows"))
            mShadowSettings->setComputeNearFarModeOverride(osg::CullSettings::COMPUTE_NEAR_FAR_USING_PRIMITIVES);

        int mapres = Settings::Manager::getInt("shadow map resolution", "Shadows");
        mShadowSettings->setTextureSize(osg::Vec2s(mapres, mapres));

        mShadowTechnique->setSplitPointUniformLogarithmicRatio(Settings::Manager::getFloat("split point uniform logarithmic ratio", "Shadows"));
        mShadowTechnique->setSplitPointDeltaBias(Settings::Manager::getFloat("split point bias", "Shadows"));

        mShadowTechnique->setPolygonOffset(Settings::Manager::getFloat("polygon offset factor", "Shadows"), Settings::Manager::getFloat("polygon offset units", "Shadows"));

        if (Settings::Manager::getBool("use front face culling", "Shadows"))
            mShadowTechnique->enableFrontFaceCulling();
        else
            mShadowTechnique->disableFrontFaceCulling();

        if (Settings::Manager::getBool("allow shadow map overlap", "Shadows"))
            mShadowSettings->setMultipleShadowMapHint(osgShadow::ShadowSettings::CASCADED);
        else
            mShadowSettings->setMultipleShadowMapHint(osgShadow::ShadowSettings::PARALLEL_SPLIT);

        if (Settings::Manager::getBool("enable debug hud", "Shadows"))
            mShadowTechnique->enableDebugHUD();
        else
            mShadowTechnique->disableDebugHUD();
    }

    void ShadowManager::disableShadowsForStateSet(osg::ref_ptr<osg::StateSet> stateset)
    {
        int numberOfShadowMapsPerLight = Settings::Manager::getInt("number of shadow maps", "Shadows");
        numberOfShadowMapsPerLight = std::max(1, std::min(numberOfShadowMapsPerLight, 8));

        int baseShadowTextureUnit = 8 - numberOfShadowMapsPerLight;
        
        osg::ref_ptr<osg::Image> fakeShadowMapImage = new osg::Image();
        fakeShadowMapImage->allocateImage(1, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT);
        *(float*)fakeShadowMapImage->data() = std::numeric_limits<float>::infinity();
        osg::ref_ptr<osg::Texture> fakeShadowMapTexture = new osg::Texture2D(fakeShadowMapImage);
        fakeShadowMapTexture->setShadowComparison(true);
        fakeShadowMapTexture->setShadowCompareFunc(osg::Texture::ShadowCompareFunc::ALWAYS);
        for (int i = baseShadowTextureUnit; i < baseShadowTextureUnit + numberOfShadowMapsPerLight; ++i)
        {
            stateset->setTextureAttributeAndModes(i, fakeShadowMapTexture, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE | osg::StateAttribute::PROTECTED);
            stateset->addUniform(new osg::Uniform(("shadowTexture" + std::to_string(i - baseShadowTextureUnit)).c_str(), i));
            stateset->addUniform(new osg::Uniform(("shadowTextureUnit" + std::to_string(i - baseShadowTextureUnit)).c_str(), i));
        }
    }

    ShadowManager::ShadowManager(osg::ref_ptr<osg::Group> sceneRoot, osg::ref_ptr<osg::Group> rootNode, unsigned int outdoorShadowCastingMask, unsigned int indoorShadowCastingMask, Shader::ShaderManager &shaderManager) : mShadowedScene(new osgShadow::ShadowedScene),
        mShadowTechnique(new MWShadowTechnique),
        mOutdoorShadowCastingMask(outdoorShadowCastingMask),
        mIndoorShadowCastingMask(indoorShadowCastingMask)
    {
        mShadowedScene->setShadowTechnique(mShadowTechnique);

        mShadowedScene->addChild(sceneRoot);
        rootNode->addChild(mShadowedScene);

        mShadowSettings = mShadowedScene->getShadowSettings();
        setupShadowSettings();

        mShadowTechnique->setupCastingShader(shaderManager);

        enableOutdoorMode();
    }

    Shader::ShaderManager::DefineMap ShadowManager::getShadowDefines()
    {
        if (!mEnableShadows)
            return getShadowsDisabledDefines();

        Shader::ShaderManager::DefineMap definesWithShadows;

        definesWithShadows["shadows_enabled"] = "1";

        for (unsigned int i = 0; i < mShadowSettings->getNumShadowMapsPerLight(); ++i)
            definesWithShadows["shadow_texture_unit_list"] += std::to_string(i) + ",";
        // remove extra comma
        definesWithShadows["shadow_texture_unit_list"] = definesWithShadows["shadow_texture_unit_list"].substr(0, definesWithShadows["shadow_texture_unit_list"].length() - 1);

        definesWithShadows["shadowMapsOverlap"] = Settings::Manager::getBool("allow shadow map overlap", "Shadows") ? "1" : "0";

        definesWithShadows["useShadowDebugOverlay"] = Settings::Manager::getBool("enable debug overlay", "Shadows") ? "1" : "0";

        // switch this to reading settings if it's ever exposed to the user
        definesWithShadows["perspectiveShadowMaps"] = mShadowSettings->getShadowMapProjectionHint() == ShadowSettings::PERSPECTIVE_SHADOW_MAP ? "1" : "0";

        definesWithShadows["disableNormalOffsetShadows"] = Settings::Manager::getFloat("normal offset distance", "Shadows") == 0.0 ? "1" : "0";

        definesWithShadows["shadowNormalOffset"] = std::to_string(Settings::Manager::getFloat("normal offset distance", "Shadows"));

        definesWithShadows["limitShadowMapDistance"] = Settings::Manager::getFloat("maximum shadow map distance", "Shadows") > 0 ? "1" : "0";

        return definesWithShadows;
    }

    Shader::ShaderManager::DefineMap ShadowManager::getShadowsDisabledDefines()
    {
        Shader::ShaderManager::DefineMap definesWithoutShadows;

        definesWithoutShadows["shadows_enabled"] = "0";

        definesWithoutShadows["shadow_texture_unit_list"] = "";

        definesWithoutShadows["shadowMapsOverlap"] = "0";

        definesWithoutShadows["useShadowDebugOverlay"] = "0";

        definesWithoutShadows["perspectiveShadowMaps"] = "0";

        definesWithoutShadows["disableNormalOffsetShadows"] = "0";

        definesWithoutShadows["shadowNormalOffset"] = "0.0";

        definesWithoutShadows["limitShadowMapDistance"] = "0";

        return definesWithoutShadows;
    }

    void ShadowManager::enableIndoorMode()
    {
        if (Settings::Manager::getBool("enable indoor shadows", "Shadows"))
            mShadowSettings->setCastsShadowTraversalMask(mIndoorShadowCastingMask);
        else
            mShadowTechnique->disableShadows();
    }

    void ShadowManager::enableOutdoorMode()
    {
        if (mEnableShadows)
            mShadowTechnique->enableShadows();
        mShadowSettings->setCastsShadowTraversalMask(mOutdoorShadowCastingMask);
    }
}
