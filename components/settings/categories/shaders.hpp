#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_SHADERS_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_SHADERS_H

#include "components/settings/sanitizerimpl.hpp"
#include "components/settings/settingvalue.hpp"

#include <osg/Math>
#include <osg/Vec2f>
#include <osg/Vec3f>

#include <cstdint>
#include <string>
#include <string_view>

namespace Settings
{
    struct ShadersCategory
    {
        SettingValue<bool> mForceShaders{ "Shaders", "force shaders" };
        SettingValue<bool> mForcePerPixelLighting{ "Shaders", "force per pixel lighting" };
        SettingValue<bool> mClampLighting{ "Shaders", "clamp lighting" };
        SettingValue<bool> mAutoUseObjectNormalMaps{ "Shaders", "auto use object normal maps" };
        SettingValue<bool> mAutoUseObjectSpecularMaps{ "Shaders", "auto use object specular maps" };
        SettingValue<bool> mAutoUseTerrainNormalMaps{ "Shaders", "auto use terrain normal maps" };
        SettingValue<bool> mAutoUseTerrainSpecularMaps{ "Shaders", "auto use terrain specular maps" };
        SettingValue<std::string> mNormalMapPattern{ "Shaders", "normal map pattern" };
        SettingValue<std::string> mNormalHeightMapPattern{ "Shaders", "normal height map pattern" };
        SettingValue<std::string> mSpecularMapPattern{ "Shaders", "specular map pattern" };
        SettingValue<std::string> mTerrainSpecularMapPattern{ "Shaders", "terrain specular map pattern" };
        SettingValue<bool> mApplyLightingToEnvironmentMaps{ "Shaders", "apply lighting to environment maps" };
        SettingValue<std::string> mLightingMethod{ "Shaders", "lighting method",
            makeEnumSanitizerString({ "legacy", "shaders compatibility", "shaders" }) };
        SettingValue<float> mLightBoundsMultiplier{ "Shaders", "light bounds multiplier",
            makeClampSanitizerFloat(0, 5) };
        SettingValue<float> mMaximumLightDistance{ "Shaders", "maximum light distance" };
        SettingValue<float> mLightFadeStart{ "Shaders", "light fade start", makeClampSanitizerFloat(0, 1) };
        SettingValue<int> mMaxLights{ "Shaders", "max lights", makeClampSanitizerInt(2, 64) };
        SettingValue<float> mMinimumInteriorBrightness{ "Shaders", "minimum interior brightness",
            makeClampSanitizerFloat(0, 1) };
        SettingValue<bool> mAntialiasAlphaTest{ "Shaders", "antialias alpha test" };
        SettingValue<bool> mAdjustCoverageForAlphaTest{ "Shaders", "adjust coverage for alpha test" };
        SettingValue<bool> mSoftParticles{ "Shaders", "soft particles" };
        SettingValue<bool> mWeatherParticleOcclusion{ "Shaders", "weather particle occlusion" };
        SettingValue<float> mWeatherParticleOcclusionSmallFeatureCullingPixelSize{ "Shaders",
            "weather particle occlusion small feature culling pixel size" };
    };
}

#endif
