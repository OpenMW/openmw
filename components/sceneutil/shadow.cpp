#include "shadow.hpp"

#include <osgShadow/ShadowedScene>
#include <osg/CullFace>
#include <osg/Geode>
#include <osg/io_utils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>

#include <components/settings/settings.hpp>

namespace SceneUtil
{
    using namespace osgShadow;

    void ShadowManager::setupShadowSettings(int castsShadowMask)
    {
        if (!Settings::Manager::getBool("enable shadows", "Shadows"))
        {
            mShadowTechnique->disableShadows();
            return;
        }
        else
            mShadowTechnique->enableShadows();

        osg::ref_ptr<osgShadow::ShadowSettings> settings = mShadowedScene->getShadowSettings();

        settings->setLightNum(0);
        settings->setCastsShadowTraversalMask(castsShadowMask);
        settings->setReceivesShadowTraversalMask(~0u);

        int numberOfShadowMapsPerLight = Settings::Manager::getInt("number of shadow maps", "Shadows");
        settings->setNumShadowMapsPerLight(numberOfShadowMapsPerLight);
        settings->setBaseShadowTextureUnit(8 - numberOfShadowMapsPerLight);

        settings->setMinimumShadowMapNearFarRatio(Settings::Manager::getFloat("minimum lispsm near far ratio", "Shadows"));
        if (Settings::Manager::getBool("compute tight scene bounds", "Shadows"))
            settings->setComputeNearFarModeOverride(osg::CullSettings::COMPUTE_NEAR_FAR_USING_PRIMITIVES);

        int mapres = Settings::Manager::getInt("shadow map resolution", "Shadows");
        settings->setTextureSize(osg::Vec2s(mapres, mapres));

        if (Settings::Manager::getBool("enable debug hud", "Shadows"))
            mShadowTechnique->enableDebugHUD();
        else
            mShadowTechnique->disableDebugHUD();
    }

    void ShadowManager::disableShadowsForStateSet(osg::ref_ptr<osg::StateSet> stateset)
    {
        int numberOfShadowMapsPerLight = Settings::Manager::getInt("number of shadow maps", "Shadows");
        int baseShadowTextureUnit = 8 - numberOfShadowMapsPerLight;
        
        osg::ref_ptr<osg::Image> fakeShadowMapImage = new osg::Image();
        fakeShadowMapImage->allocateImage(1, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT);
        *(float*)fakeShadowMapImage->data() = std::numeric_limits<float>::infinity();
        osg::ref_ptr<osg::Texture> fakeShadowMapTexture = new osg::Texture2D(fakeShadowMapImage);
        fakeShadowMapTexture->setShadowComparison(true);
        fakeShadowMapTexture->setShadowCompareFunc(osg::Texture::ShadowCompareFunc::ALWAYS);
        for (int i = baseShadowTextureUnit; i < baseShadowTextureUnit + numberOfShadowMapsPerLight; ++i)
            stateset->setTextureAttributeAndModes(i, fakeShadowMapTexture, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE | osg::StateAttribute::PROTECTED);
    }

    ShadowManager::ShadowManager(osg::ref_ptr<osg::Group> sceneRoot, osg::ref_ptr<osg::Group> rootNode) : enableShadows(Settings::Manager::getBool("enable shadows", "Shadows")),
        numberOfShadowMapsPerLight(Settings::Manager::getInt("number of shadow maps", "Shadows")),
        baseShadowTextureUnit(8 - numberOfShadowMapsPerLight),
        mShadowedScene(new osgShadow::ShadowedScene),
        mShadowTechnique(new MWShadowTechnique)
    {
        mShadowedScene->setShadowTechnique(mShadowTechnique);
        mShadowedScene->addChild(sceneRoot);
        rootNode->addChild(mShadowedScene);
    }

    Shader::ShaderManager::DefineMap ShadowManager::getShadowDefines()
    {
        if (!enableShadows)
            return getShadowsDisabledDefines();

        Shader::ShaderManager::DefineMap definesWithShadows;
        definesWithShadows.insert(std::make_pair(std::string("shadows_enabled"), std::string("1")));
        for (int i = 0; i < numberOfShadowMapsPerLight; ++i)
            definesWithShadows["shadow_texture_unit_list"] += std::to_string(i) + ",";
        // remove extra comma
        definesWithShadows["shadow_texture_unit_list"] = definesWithShadows["shadow_texture_unit_list"].substr(0, definesWithShadows["shadow_texture_unit_list"].length() - 1);

        return definesWithShadows;
    }

    Shader::ShaderManager::DefineMap ShadowManager::getShadowsDisabledDefines()
    {
        Shader::ShaderManager::DefineMap definesWithShadows;
        definesWithShadows.insert(std::make_pair(std::string("shadows_enabled"), std::string("0")));
        definesWithShadows["shadow_texture_unit_list"] = "";

        return definesWithShadows;
    }
}
