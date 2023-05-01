#ifndef COMPONENTS_SCENEUTIL_LIGHTCOMMON
#define COMPONENTS_SCENEUTIL_LIGHTCOMMON

#include <osg/Vec4>

namespace ESM4
{
    struct Light;
}

namespace ESM
{
    struct Light;
}

namespace SceneUtil
{
    struct LightCommon
    {
        explicit LightCommon(const ESM::Light& light);
        explicit LightCommon(const ESM4::Light& light);

        bool mFlicker;
        bool mFlickerSlow;
        bool mNegative;
        bool mPulse;
        bool mPulseSlow;
        bool mOffDefault;

        osg::Vec4 mColor;
        float mRadius;
    };

}
#endif
