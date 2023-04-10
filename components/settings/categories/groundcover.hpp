#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_GROUNDCOVER_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_GROUNDCOVER_H

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
    struct GroundcoverCategory
    {
        SettingValue<bool> mEnabled{ "Groundcover", "enabled" };
        SettingValue<bool> mPaging{ "Groundcover", "paging" };
        SettingValue<bool> mAutoLoad{ "Groundcover", "auto load" };
        SettingValue<float> mMinChunkSize{ "Groundcover", "min chunk size", makeClampSanitizerFloat(0.125, 1) };
        SettingValue<float> mMergeFactor{ "Groundcover", "merge factor", makeClampSanitizerFloat(0, 5000) };
        SettingValue<float> mDensity{ "Groundcover", "density", makeClampSanitizerFloat(0, 1) };
        SettingValue<float> mRenderingDistance{ "Groundcover", "rendering distance", makeMaxSanitizerFloat(0) };
        SettingValue<int> mStompMode{ "Groundcover", "stomp mode", makeEnumSanitizerInt({ 0, 1, 2 }) };
        SettingValue<int> mStompIntensity{ "Groundcover", "stomp intensity", makeEnumSanitizerInt({ 0, 1, 2 }) };
    };
}

#endif
