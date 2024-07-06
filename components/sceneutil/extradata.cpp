#include "extradata.hpp"

#include <unordered_set>

#include <osg/Node>
#include <osg/ValueObject>
#include <osgParticle/ParticleSystem>

#include <yaml-cpp/yaml.h>

#include <components/misc/osguservalues.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/shader/shadermanager.hpp>

namespace SceneUtil
{
    void setupSoftEffect(osg::Node& node, float size, bool falloff, float falloffDepth)
    {
        static const osg::ref_ptr<SceneUtil::AutoDepth> depth = new SceneUtil::AutoDepth(osg::Depth::LESS, 0, 1, false);

        osg::StateSet* stateset = node.getOrCreateStateSet();

        stateset->addUniform(new osg::Uniform("particleSize", size));
        stateset->addUniform(new osg::Uniform("particleFade", falloff));
        stateset->addUniform(new osg::Uniform("softFalloffDepth", falloffDepth));
        stateset->setAttributeAndModes(depth, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

        node.setUserValue(Misc::OsgUserValues::sXSoftEffect, true);
    }

    void setupDistortion(osg::Node& node, float distortionStrength)
    {
        static const osg::ref_ptr<SceneUtil::AutoDepth> depth
            = new SceneUtil::AutoDepth(osg::Depth::ALWAYS, 0, 1, false);

        osg::StateSet* stateset = node.getOrCreateStateSet();

        stateset->setNestRenderBins(false);
        stateset->setRenderBinDetails(14, "Distortion", osg::StateSet::OVERRIDE_PROTECTED_RENDERBIN_DETAILS);
        stateset->addUniform(new osg::Uniform("distortionStrength", distortionStrength));

        stateset->setAttributeAndModes(depth, osg::StateAttribute::ON);
    }

    void ProcessExtraDataVisitor::apply(osg::Node& node)
    {
        if (!mSceneMgr->getSoftParticles())
            return;

        std::string source;

        constexpr float defaultFalloffDepth = 300.f; // arbitrary value that simply looks good with common cases

        if (node.getUserValue(Misc::OsgUserValues::sExtraData, source) && !source.empty())
        {
            YAML::Node root = YAML::Load(source);

            for (const auto& it : root["shader"])
            {
                std::string key = it.first.as<std::string>();

                if (key == "soft_effect")
                {
                    auto size = it.second["size"].as<float>(45.f);
                    auto falloff = it.second["falloff"].as<bool>(false);
                    auto falloffDepth = it.second["falloffDepth"].as<float>(defaultFalloffDepth);

                    setupSoftEffect(node, size, falloff, falloffDepth);
                }
                else if (key == "distortion")
                {
                    auto strength = it.second["strength"].as<float>(0.1f);

                    setupDistortion(node, strength);
                }
            }

            node.setUserValue(Misc::OsgUserValues::sExtraData, std::string{});
        }
        else if (osgParticle::ParticleSystem* partsys = dynamic_cast<osgParticle::ParticleSystem*>(&node))
        {
            setupSoftEffect(
                node, partsys->getDefaultParticleTemplate().getSizeRange().maximum, false, defaultFalloffDepth);
        }

        traverse(node);
    }
}
