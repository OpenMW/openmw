#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_FOG_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_FOG_H

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
    struct FogCategory
    {
        SettingValue<bool> mUseDistantFog{ "Fog", "use distant fog" };
        SettingValue<float> mDistantLandFogStart{ "Fog", "distant land fog start" };
        SettingValue<float> mDistantLandFogEnd{ "Fog", "distant land fog end" };
        SettingValue<float> mDistantUnderwaterFogStart{ "Fog", "distant underwater fog start" };
        SettingValue<float> mDistantUnderwaterFogEnd{ "Fog", "distant underwater fog end" };
        SettingValue<float> mDistantInteriorFogStart{ "Fog", "distant interior fog start" };
        SettingValue<float> mDistantInteriorFogEnd{ "Fog", "distant interior fog end" };
        SettingValue<bool> mRadialFog{ "Fog", "radial fog" };
        SettingValue<bool> mExponentialFog{ "Fog", "exponential fog" };
        SettingValue<bool> mSkyBlending{ "Fog", "sky blending" };
        SettingValue<float> mSkyBlendingStart{ "Fog", "sky blending start", makeClampStrictMaxSanitizerFloat(0, 1) };
        SettingValue<osg::Vec2f> mSkyRttResolution{ "Fog", "sky rtt resolution" };
    };
}

#endif
