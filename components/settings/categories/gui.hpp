#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_GUI_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_GUI_H

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
    struct GUICategory
    {
        SettingValue<float> mScalingFactor{ "GUI", "scaling factor", makeClampSanitizerFloat(0.5f, 8) };
        SettingValue<int> mFontSize{ "GUI", "font size", makeClampSanitizerInt(12, 18) };
        SettingValue<float> mMenuTransparency{ "GUI", "menu transparency", makeClampSanitizerFloat(0, 1) };
        SettingValue<float> mTooltipDelay{ "GUI", "tooltip delay", makeMaxSanitizerFloat(0) };
        SettingValue<bool> mStretchMenuBackground{ "GUI", "stretch menu background" };
        SettingValue<bool> mSubtitles{ "GUI", "subtitles" };
        SettingValue<bool> mHitFader{ "GUI", "hit fader" };
        SettingValue<bool> mWerewolfOverlay{ "GUI", "werewolf overlay" };
        SettingValue<float> mColorBackgroundOwned{ "GUI", "color background owned", makeClampSanitizerFloat(0, 1) };
        SettingValue<float> mColorCrosshairOwned{ "GUI", "color crosshair owned", makeClampSanitizerFloat(0, 1) };
        SettingValue<bool> mKeyboardNavigation{ "GUI", "keyboard navigation" };
        SettingValue<bool> mColorTopicEnable{ "GUI", "color topic enable" };
        SettingValue<float> mColorTopicSpecific{ "GUI", "color topic specific", makeClampSanitizerFloat(0, 1) };
        SettingValue<float> mColorTopicExhausted{ "GUI", "color topic exhausted", makeClampSanitizerFloat(0, 1) };
    };
}

#endif
