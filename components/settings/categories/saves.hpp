#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_SAVES_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_SAVES_H

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
    struct SavesCategory
    {
        SettingValue<std::string> mCharacter{ "Saves", "character" };
        SettingValue<bool> mAutosave{ "Saves", "autosave" };
        SettingValue<bool> mTimeplayed{ "Saves", "timeplayed" };
        SettingValue<int> mMaxQuicksaves{ "Saves", "max quicksaves", makeMaxSanitizerInt(1) };
    };
}

#endif
