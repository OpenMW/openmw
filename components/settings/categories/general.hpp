#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_GENERAL_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_GENERAL_H

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
    struct GeneralCategory : WithIndex
    {
        using WithIndex::WithIndex;

        SettingValue<int> mAnisotropy{ mIndex, "General", "anisotropy", makeClampSanitizerInt(0, 16) };
        SettingValue<std::string> mScreenshotFormat{ mIndex, "General", "screenshot format",
            makeEnumSanitizerString({ "jpg", "png", "tga" }) };
        SettingValue<std::string> mTextureMagFilter{ mIndex, "General", "texture mag filter",
            makeEnumSanitizerString({ "nearest", "linear", "cubic" }) };
        SettingValue<std::string> mTextureMinFilter{ mIndex, "General", "texture min filter",
            makeEnumSanitizerString({ "nearest", "linear", "cubic" }) };
        SettingValue<std::string> mTextureMipmap{ mIndex, "General", "texture mipmap",
            makeEnumSanitizerString({ "none", "nearest", "linear" }) };
        SettingValue<bool> mNotifyOnSavedScreenshot{ mIndex, "General", "notify on saved screenshot" };
        SettingValue<std::vector<std::string>> mPreferredLocales{ mIndex, "General", "preferred locales" };
        SettingValue<bool> mGmstOverridesL10n{ mIndex, "General", "gmst overrides l10n" };
        SettingValue<std::size_t> mLogBufferSize{ mIndex, "General", "log buffer size" };
        SettingValue<std::size_t> mConsoleHistoryBufferSize{ mIndex, "General", "console history buffer size" };
    };
}

#endif
