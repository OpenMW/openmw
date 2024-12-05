#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_FOG_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_FOG_H

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
    struct FogCategory : WithIndex
    {
        using WithIndex::WithIndex;

        SettingValue<bool> mUseDistantFog{ mIndex, "Fog", "use distant fog" };
        SettingValue<float> mDistantLandFogStart{ mIndex, "Fog", "distant land fog start" };
        SettingValue<float> mDistantLandFogEnd{ mIndex, "Fog", "distant land fog end" };
        SettingValue<float> mDistantUnderwaterFogStart{ mIndex, "Fog", "distant underwater fog start" };
        SettingValue<float> mDistantUnderwaterFogEnd{ mIndex, "Fog", "distant underwater fog end" };
        SettingValue<float> mDistantInteriorFogStart{ mIndex, "Fog", "distant interior fog start" };
        SettingValue<float> mDistantInteriorFogEnd{ mIndex, "Fog", "distant interior fog end" };
        SettingValue<bool> mRadialFog{ mIndex, "Fog", "radial fog" };
        SettingValue<bool> mExponentialFog{ mIndex, "Fog", "exponential fog" };
        SettingValue<bool> mSkyBlending{ mIndex, "Fog", "sky blending" };
        SettingValue<float> mSkyBlendingStart{ mIndex, "Fog", "sky blending start",
            makeClampStrictMaxSanitizerFloat(0, 1) };
        SettingValue<osg::Vec2f> mSkyRttResolution{ mIndex, "Fog", "sky rtt resolution" };
    };
}

#endif
