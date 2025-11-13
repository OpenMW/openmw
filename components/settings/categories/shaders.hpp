#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_SHADERS_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_SHADERS_H

#include <components/sceneutil/lightingmethod.hpp>
#include <components/settings/sanitizerimpl.hpp>
#include <components/settings/settingvalue.hpp>

#include <osg/Math>
#include <osg/Vec2f>
#include <osg/Vec3f>

#include <cstdint>
#include <string>
#include <string_view>

namespace Settings
{
    struct ShadersCategory : WithIndex
    {
        using WithIndex::WithIndex;

        SettingValue<bool> mForceShaders{ mIndex, "Shaders", "force shaders" };
        SettingValue<bool> mForcePerPixelLighting{ mIndex, "Shaders", "force per pixel lighting" };
        SettingValue<bool> mClampLighting{ mIndex, "Shaders", "clamp lighting" };
        SettingValue<bool> mAutoUseObjectNormalMaps{ mIndex, "Shaders", "auto use object normal maps" };
        SettingValue<bool> mAutoUseObjectSpecularMaps{ mIndex, "Shaders", "auto use object specular maps" };
        SettingValue<bool> mAutoUseTerrainNormalMaps{ mIndex, "Shaders", "auto use terrain normal maps" };
        SettingValue<bool> mAutoUseTerrainSpecularMaps{ mIndex, "Shaders", "auto use terrain specular maps" };
        SettingValue<std::string> mNormalMapPattern{ mIndex, "Shaders", "normal map pattern" };
        SettingValue<std::string> mNormalHeightMapPattern{ mIndex, "Shaders", "normal height map pattern" };
        SettingValue<std::string> mSpecularMapPattern{ mIndex, "Shaders", "specular map pattern" };
        SettingValue<std::string> mTerrainSpecularMapPattern{ mIndex, "Shaders", "terrain specular map pattern" };
        SettingValue<bool> mTerrainDeformation{ mIndex, "Shaders", "terrain deformation" };
        SettingValue<bool> mTerrainDeformationTessellation{ mIndex, "Shaders", "terrain deformation tessellation" };
        SettingValue<float> mTerrainDeformationTessBaseLevel{ mIndex, "Shaders", "terrain deformation tess base level",
            makeClampSanitizerFloat(4, 16) };
        SettingValue<float> mTerrainDeformationTessMaxLevel{ mIndex, "Shaders", "terrain deformation tess max level",
            makeClampSanitizerFloat(16, 64) };
        SettingValue<float> mTerrainDeformationMaxDepth{ mIndex, "Shaders", "terrain deformation max depth",
            makeClampSanitizerFloat(10, 100) };
        SettingValue<float> mTerrainDeformationDecayRate{ mIndex, "Shaders", "terrain deformation decay rate",
            makeClampSanitizerFloat(0.90f, 0.999f) };
        SettingValue<bool> mApplyLightingToEnvironmentMaps{ mIndex, "Shaders", "apply lighting to environment maps" };
        SettingValue<SceneUtil::LightingMethod> mLightingMethod{ mIndex, "Shaders", "lighting method" };
        SettingValue<bool> mClassicFalloff{ mIndex, "Shaders", "classic falloff" };
        SettingValue<bool> mMatchSunlightToSun{ mIndex, "Shaders", "match sunlight to sun" };
        SettingValue<float> mLightBoundsMultiplier{ mIndex, "Shaders", "light bounds multiplier",
            makeClampSanitizerFloat(0, 5) };
        SettingValue<float> mMaximumLightDistance{ mIndex, "Shaders", "maximum light distance",
            makeMaxSanitizerFloat(0) };
        SettingValue<float> mLightFadeStart{ mIndex, "Shaders", "light fade start", makeClampSanitizerFloat(0, 1) };
        SettingValue<int> mMaxLights{ mIndex, "Shaders", "max lights", makeClampSanitizerInt(2, 64) };
        SettingValue<float> mMinimumInteriorBrightness{ mIndex, "Shaders", "minimum interior brightness",
            makeClampSanitizerFloat(0, 1) };
        SettingValue<bool> mAntialiasAlphaTest{ mIndex, "Shaders", "antialias alpha test" };
        SettingValue<bool> mAdjustCoverageForAlphaTest{ mIndex, "Shaders", "adjust coverage for alpha test" };
        SettingValue<bool> mSoftParticles{ mIndex, "Shaders", "soft particles" };
        SettingValue<bool> mWeatherParticleOcclusion{ mIndex, "Shaders", "weather particle occlusion" };
        SettingValue<float> mWeatherParticleOcclusionSmallFeatureCullingPixelSize{ mIndex, "Shaders",
            "weather particle occlusion small feature culling pixel size" };
    };
}

#endif
