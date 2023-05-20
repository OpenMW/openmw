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
    struct GroundcoverCategory : WithIndex
    {
        SettingValue<bool> mEnabled{ "Groundcover", "enabled" };
        SettingValue<bool> mPaging{ "Groundcover", "paging" };
        SettingValue<bool> mAutoLoad{ "Groundcover", "auto load" };
        SettingValue<float> mMinChunkSize{ "Groundcover", "min chunk size", makeClampSanitizerFloat(0.125, 1) };
        SettingValue<float> mMergeFactor{ "Groundcover", "merge factor", makeMaxSanitizerFloat(0) };
        SettingValue<float> mDensity{ mIndex, "Groundcover", "density", makeClampSanitizerFloat(0, 1) };
        SettingValue<float> mRenderingDistance{ mIndex, "Groundcover", "rendering distance", makeMaxSanitizerFloat(0) };
        SettingValue<int> mStompMode{ mIndex, "Groundcover", "stomp mode", makeEnumSanitizerInt({ 0, 1, 2 }) };
        SettingValue<int> mStompIntensity{ mIndex, "Groundcover", "stomp intensity",
            makeEnumSanitizerInt({ 0, 1, 2 }) };
    };
}

#endif
