#ifndef OCCLUSION_SETTINGS_H
#define OCCLUSION_SETTINGS_H

namespace SceneUtil{

struct OcclusionQuerySettings
{
    /// OQN settings
    bool enable;
    bool debugDisplay;
    unsigned int querypixelcount;
    unsigned int queryframecount;
    float querymargin;
    float securepopdistance;

    ///RenderBin and Mask
    unsigned int OQMask;
    unsigned int OQRenderBin;

    ///Octree parameters
    unsigned int maxBVHOQLevelCount;

    ///subdivision criterions
    float minOQNSize;
    unsigned int maxOQNCapacity;
};
}
#endif
