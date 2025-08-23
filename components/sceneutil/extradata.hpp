#ifndef OPENMW_COMPONENTS_RESOURCE_EXTRADATA_H
#define OPENMW_COMPONENTS_RESOURCE_EXTRADATA_H

namespace osg
{
    class Node;
}

namespace SceneUtil
{
    struct SoftEffectConfig
    {
        float mSize = 45.f;
        float mFalloffDepth = 300.f;
        bool mFalloff = false;
    };

    struct DistortionConfig
    {
        float mStrength = 0.1f;
    };

    void setupSoftEffect(osg::Node& node, const SoftEffectConfig& config);
    void setupDistortion(osg::Node& node, const DistortionConfig& config);
}

#endif
