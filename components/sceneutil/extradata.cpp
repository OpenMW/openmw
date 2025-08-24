#include "extradata.hpp"

#include <osg/Node>

#include <components/misc/osguservalues.hpp>
#include <components/sceneutil/depth.hpp>

namespace SceneUtil
{
    void setupSoftEffect(osg::Node& node, const SoftEffectConfig& config)
    {
        static const osg::ref_ptr<SceneUtil::AutoDepth> depth
            = new SceneUtil::AutoDepth(osg::Depth::LEQUAL, 0, 1, false);

        osg::StateSet* stateset = node.getOrCreateStateSet();

        stateset->addUniform(new osg::Uniform("particleSize", config.mSize));
        stateset->addUniform(new osg::Uniform("particleFade", config.mFalloff));
        stateset->addUniform(new osg::Uniform("softFalloffDepth", config.mFalloffDepth));
        stateset->setAttributeAndModes(depth, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

        node.setUserValue(Misc::OsgUserValues::sXSoftEffect, true);
    }

    void setupDistortion(osg::Node& node, const DistortionConfig& config)
    {
        static const osg::ref_ptr<SceneUtil::AutoDepth> depth
            = new SceneUtil::AutoDepth(osg::Depth::ALWAYS, 0, 1, false);

        osg::StateSet* stateset = node.getOrCreateStateSet();

        stateset->setNestRenderBins(false);
        stateset->setRenderBinDetails(14, "Distortion", osg::StateSet::OVERRIDE_PROTECTED_RENDERBIN_DETAILS);
        stateset->addUniform(new osg::Uniform("distortionStrength", config.mStrength));

        stateset->setAttributeAndModes(depth, osg::StateAttribute::ON);
    }
}
