#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_VIDEO_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_VIDEO_H

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
    struct VideoCategory
    {
        SettingValue<int> mResolutionX{ "Video", "resolution x", makeMaxSanitizerInt(1) };
        SettingValue<int> mResolutionY{ "Video", "resolution y", makeMaxSanitizerInt(1) };
        SettingValue<int> mWindowMode{ "Video", "window mode", makeEnumSanitizerInt({ 0, 1, 2 }) };
        SettingValue<int> mScreen{ "Video", "screen", makeMaxSanitizerInt(0) };
        SettingValue<bool> mMinimizeOnFocusLoss{ "Video", "minimize on focus loss" };
        SettingValue<bool> mWindowBorder{ "Video", "window border" };
        SettingValue<int> mAntialiasing{ "Video", "antialiasing", makeEnumSanitizerInt({ 0, 2, 4, 8, 16 }) };
        SettingValue<int> mVsyncMode{ "Video", "vsync mode", makeEnumSanitizerInt({ 0, 1, 2 }) };
        SettingValue<float> mFramerateLimit{ "Video", "framerate limit", makeMaxSanitizerFloat(0) };
        SettingValue<float> mContrast{ "Video", "contrast", makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mGamma{ "Video", "gamma", makeMaxStrictSanitizerFloat(0) };
        SettingValue<std::string> mScreenshotType{ "Video", "screenshot type" };
    };
}

#endif
