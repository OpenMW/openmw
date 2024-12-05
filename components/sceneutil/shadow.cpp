#include "shadow.hpp"

#include <osgShadow/ShadowSettings>
#include <osgShadow/ShadowedScene>

#include <components/misc/strings/algorithm.hpp>
#include <components/settings/categories/shadows.hpp>
#include <components/stereo/stereomanager.hpp>

#include "mwshadowtechnique.hpp"

namespace SceneUtil
{
    using namespace osgShadow;

    ShadowManager* ShadowManager::sInstance = nullptr;

    const ShadowManager& ShadowManager::instance()
    {
        if (sInstance)
            return *sInstance;
        else
            throw std::logic_error("No ShadowManager exists yet");
    }

    void ShadowManager::setupShadowSettings(
        const Settings::ShadowsCategory& settings, Shader::ShaderManager& shaderManager)
    {
        mEnableShadows = settings.mEnableShadows;

        if (!mEnableShadows)
        {
            mShadowTechnique->disableShadows();
            return;
        }

        mShadowTechnique->enableShadows();

        mShadowSettings->setLightNum(0);
        mShadowSettings->setReceivesShadowTraversalMask(~0u);

        const int numberOfShadowMapsPerLight = settings.mNumberOfShadowMaps;

        mShadowSettings->setNumShadowMapsPerLight(numberOfShadowMapsPerLight);
        mShadowSettings->setBaseShadowTextureUnit(shaderManager.reserveGlobalTextureUnits(
            Shader::ShaderManager::Slot::ShadowMaps, numberOfShadowMapsPerLight));

        const float maximumShadowMapDistance = settings.mMaximumShadowMapDistance;
        if (maximumShadowMapDistance > 0)
        {
            const float shadowFadeStart = settings.mShadowFadeStart;
            mShadowSettings->setMaximumShadowMapDistance(maximumShadowMapDistance);
            mShadowTechnique->setShadowFadeStart(maximumShadowMapDistance * shadowFadeStart);
        }

        mShadowSettings->setMinimumShadowMapNearFarRatio(settings.mMinimumLispsmNearFarRatio);

        const std::string& computeSceneBounds = settings.mComputeSceneBounds;
        if (Misc::StringUtils::ciEqual(computeSceneBounds, "primitives"))
            mShadowSettings->setComputeNearFarModeOverride(osg::CullSettings::COMPUTE_NEAR_FAR_USING_PRIMITIVES);
        else if (Misc::StringUtils::ciEqual(computeSceneBounds, "bounds"))
            mShadowSettings->setComputeNearFarModeOverride(osg::CullSettings::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);

        const int mapres = settings.mShadowMapResolution;
        mShadowSettings->setTextureSize(osg::Vec2s(mapres, mapres));

        mShadowTechnique->setSplitPointUniformLogarithmicRatio(settings.mSplitPointUniformLogarithmicRatio);
        mShadowTechnique->setSplitPointDeltaBias(settings.mSplitPointBias);

        mShadowTechnique->setPolygonOffset(settings.mPolygonOffsetFactor, settings.mPolygonOffsetUnits);

        if (settings.mUseFrontFaceCulling)
            mShadowTechnique->enableFrontFaceCulling();
        else
            mShadowTechnique->disableFrontFaceCulling();

        mShadowSettings->setMultipleShadowMapHint(osgShadow::ShadowSettings::CASCADED);

        if (settings.mEnableDebugHud)
            mShadowTechnique->enableDebugHUD();
        else
            mShadowTechnique->disableDebugHUD();
    }

    void ShadowManager::disableShadowsForStateSet(osg::StateSet& stateset) const
    {
        if (!mEnableShadows)
            return;

        osg::ref_ptr<osg::Image> fakeShadowMapImage = new osg::Image();
        fakeShadowMapImage->allocateImage(1, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT);
        *(float*)fakeShadowMapImage->data() = std::numeric_limits<float>::infinity();
        osg::ref_ptr<osg::Texture> fakeShadowMapTexture = new osg::Texture2D(fakeShadowMapImage);
        fakeShadowMapTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        fakeShadowMapTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        fakeShadowMapTexture->setShadowComparison(true);
        fakeShadowMapTexture->setShadowCompareFunc(osg::Texture::ShadowCompareFunc::ALWAYS);
        for (unsigned int i = mShadowSettings->getBaseShadowTextureUnit();
             i < mShadowSettings->getBaseShadowTextureUnit() + mShadowSettings->getNumShadowMapsPerLight(); ++i)
        {
            stateset.setTextureAttribute(i, fakeShadowMapTexture,
                osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE | osg::StateAttribute::PROTECTED);
            stateset.addUniform(new osg::Uniform(
                ("shadowTexture" + std::to_string(i - mShadowSettings->getBaseShadowTextureUnit())).c_str(),
                static_cast<int>(i)));
        }
    }

    ShadowManager::ShadowManager(osg::ref_ptr<osg::Group> sceneRoot, osg::ref_ptr<osg::Group> rootNode,
        unsigned int outdoorShadowCastingMask, unsigned int indoorShadowCastingMask, unsigned int worldMask,
        const Settings::ShadowsCategory& settings, Shader::ShaderManager& shaderManager)
        : mShadowedScene(new osgShadow::ShadowedScene)
        , mShadowTechnique(new MWShadowTechnique)
        , mOutdoorShadowCastingMask(outdoorShadowCastingMask)
        , mIndoorShadowCastingMask(indoorShadowCastingMask)
    {
        if (sInstance)
            throw std::logic_error("A ShadowManager already exists");

        mShadowedScene->setShadowTechnique(mShadowTechnique);

        if (Stereo::getStereo())
            Stereo::Manager::instance().setShadowTechnique(mShadowTechnique);

        mShadowedScene->addChild(sceneRoot);
        rootNode->addChild(mShadowedScene);
        mShadowedScene->setNodeMask(sceneRoot->getNodeMask());

        mShadowSettings = mShadowedScene->getShadowSettings();
        setupShadowSettings(settings, shaderManager);

        mShadowTechnique->setupCastingShader(shaderManager);
        mShadowTechnique->setWorldMask(worldMask);

        enableOutdoorMode();

        sInstance = this;
    }

    ShadowManager::~ShadowManager()
    {
        if (Stereo::getStereo())
            Stereo::Manager::instance().setShadowTechnique(nullptr);
    }

    Shader::ShaderManager::DefineMap ShadowManager::getShadowDefines(const Settings::ShadowsCategory& settings) const
    {
        if (!mEnableShadows)
            return getShadowsDisabledDefines();

        Shader::ShaderManager::DefineMap definesWithShadows;

        definesWithShadows["shadows_enabled"] = "1";

        for (unsigned int i = 0; i < mShadowSettings->getNumShadowMapsPerLight(); ++i)
            definesWithShadows["shadow_texture_unit_list"] += std::to_string(i) + ",";
        // remove extra comma
        definesWithShadows["shadow_texture_unit_list"] = definesWithShadows["shadow_texture_unit_list"].substr(
            0, definesWithShadows["shadow_texture_unit_list"].length() - 1);

        definesWithShadows["useShadowDebugOverlay"] = settings.mEnableDebugOverlay ? "1" : "0";

        // switch this to reading settings if it's ever exposed to the user
        definesWithShadows["perspectiveShadowMaps"]
            = mShadowSettings->getShadowMapProjectionHint() == ShadowSettings::PERSPECTIVE_SHADOW_MAP ? "1" : "0";

        definesWithShadows["disableNormalOffsetShadows"] = settings.mNormalOffsetDistance == 0.0 ? "1" : "0";

        definesWithShadows["shadowNormalOffset"] = std::to_string(settings.mNormalOffsetDistance);

        definesWithShadows["limitShadowMapDistance"] = settings.mMaximumShadowMapDistance > 0 ? "1" : "0";

        return definesWithShadows;
    }

    Shader::ShaderManager::DefineMap ShadowManager::getShadowsDisabledDefines()
    {
        Shader::ShaderManager::DefineMap definesWithoutShadows;

        definesWithoutShadows["shadows_enabled"] = "0";

        definesWithoutShadows["shadow_texture_unit_list"] = "";

        definesWithoutShadows["useShadowDebugOverlay"] = "0";

        definesWithoutShadows["perspectiveShadowMaps"] = "0";

        definesWithoutShadows["disableNormalOffsetShadows"] = "0";

        definesWithoutShadows["shadowNormalOffset"] = "0.0";

        definesWithoutShadows["limitShadowMapDistance"] = "0";

        return definesWithoutShadows;
    }

    void ShadowManager::enableIndoorMode(const Settings::ShadowsCategory& settings)
    {
        if (settings.mEnableIndoorShadows)
            mShadowSettings->setCastsShadowTraversalMask(mIndoorShadowCastingMask);
        else
            mShadowTechnique->disableShadows(true);
    }

    void ShadowManager::enableOutdoorMode()
    {
        if (mEnableShadows)
            mShadowTechnique->enableShadows();
        mShadowSettings->setCastsShadowTraversalMask(mOutdoorShadowCastingMask);
    }
}
