#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_GENERAL_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_GENERAL_H

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
    struct GeneralCategory
    {
        SettingValue<int> mAnisotropy{ "General", "anisotropy", makeClampSanitizerInt(0, 16) };
        SettingValue<std::string> mScreenshotFormat{ "General", "screenshot format",
            makeEnumSanitizerString({ "jpg", "png", "tga" }) };
        SettingValue<std::string> mTextureMagFilter{ "General", "texture mag filter",
            makeEnumSanitizerString({ "nearest", "linear" }) };
        SettingValue<std::string> mTextureMinFilter{ "General", "texture min filter",
            makeEnumSanitizerString({ "nearest", "linear" }) };
        SettingValue<std::string> mTextureMipmap{ "General", "texture mipmap",
            makeEnumSanitizerString({ "none", "nearest", "linear" }) };
        SettingValue<bool> mNotifyOnSavedScreenshot{ "General", "notify on saved screenshot" };
        SettingValue<std::string> mPreferredLocales{ "General", "preferred locales" };
        SettingValue<std::size_t> mLogBufferSize{ "General", "log buffer size" };
        SettingValue<std::size_t> mConsoleHistoryBufferSize{ "General", "console history buffer size" };
    };
}

#endif
