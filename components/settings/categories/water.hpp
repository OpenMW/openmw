#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_WATER_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_WATER_H

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
    struct WaterCategory
    {
        SettingValue<bool> mShader{ "Water", "shader" };
        SettingValue<int> mRttSize{ "Water", "rtt size", makeMaxSanitizerInt(1) };
        SettingValue<bool> mRefraction{ "Water", "refraction" };
        SettingValue<int> mReflectionDetail{ "Water", "reflection detail", makeEnumSanitizerInt({ 0, 1, 2, 3, 4, 5 }) };
        SettingValue<int> mRainRippleDetail{ "Water", "rain ripple detail", makeEnumSanitizerInt({ 0, 1, 2 }) };
        SettingValue<float> mSmallFeatureCullingPixelSize{ "Water", "small feature culling pixel size",
            makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mRefractionScale{ "Water", "refraction scale", makeClampSanitizerFloat(0, 1) };
    };
}

#endif
