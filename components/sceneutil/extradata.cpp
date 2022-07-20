#include "extradata.hpp"

#include <unordered_set>

#include <osg/Node>
#include <osg/ValueObject>
#include <osgParticle/ParticleSystem>

#include <yaml-cpp/yaml.h>

#include <components/misc/osguservalues.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/resource/scenemanager.hpp>
#include <components/shader/shadermanager.hpp>
#include <components/serialization/osgyaml.hpp>

#include <components/debug/debuglog.hpp>

namespace SceneUtil
{
    void ProcessExtraDataVisitor::setupSoftEffect(osg::Node& node, float size, bool falloff)
    {
        if (!mSceneMgr->getSoftParticles())
            return;

        const int unitSoftEffect = mSceneMgr->getShaderManager().reserveGlobalTextureUnits(Shader::ShaderManager::Slot::OpaqueDepthTexture);
        static const osg::ref_ptr<SceneUtil::AutoDepth> depth = new SceneUtil::AutoDepth(osg::Depth::LESS, 0, 1, false);

        osg::StateSet* stateset = node.getOrCreateStateSet();

        stateset->addUniform(new osg::Uniform("opaqueDepthTex", unitSoftEffect));
        stateset->addUniform(new osg::Uniform("particleSize", size));
        stateset->addUniform(new osg::Uniform("particleFade", falloff));
        stateset->setAttributeAndModes(depth, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

        node.setUserValue(Misc::OsgUserValues::sXSoftEffect, true);
    }

    void ProcessExtraDataVisitor::apply(osg::Node& node)
    {
        std::string source;

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

                    setupSoftEffect(node, size, falloff);
                }
            }

            node.setUserValue(Misc::OsgUserValues::sExtraData, std::string{});
        }
        else if (osgParticle::ParticleSystem* partsys = dynamic_cast<osgParticle::ParticleSystem*>(&node))
        {
            setupSoftEffect(node, partsys->getDefaultParticleTemplate().getSizeRange().maximum, false);
        }

        traverse(node);
    }
}