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
    struct GUICategory : WithIndex
    {
        using WithIndex::WithIndex;

        SettingValue<float> mScalingFactor{ mIndex, "GUI", "scaling factor", makeClampSanitizerFloat(0.5f, 8) };
        SettingValue<int> mFontSize{ mIndex, "GUI", "font size", makeClampSanitizerInt(12, 18) };
        SettingValue<float> mMenuTransparency{ mIndex, "GUI", "menu transparency", makeClampSanitizerFloat(0, 1) };
        SettingValue<float> mTooltipDelay{ mIndex, "GUI", "tooltip delay", makeMaxSanitizerFloat(0) };
        SettingValue<bool> mStretchMenuBackground{ mIndex, "GUI", "stretch menu background" };
        SettingValue<bool> mSubtitles{ mIndex, "GUI", "subtitles" };
        SettingValue<bool> mHitFader{ mIndex, "GUI", "hit fader" };
        SettingValue<bool> mWerewolfOverlay{ mIndex, "GUI", "werewolf overlay" };
        SettingValue<float> mColorBackgroundOwned{ mIndex, "GUI", "color background owned",
            makeClampSanitizerFloat(0, 1) };
        SettingValue<float> mColorCrosshairOwned{ mIndex, "GUI", "color crosshair owned",
            makeClampSanitizerFloat(0, 1) };
        SettingValue<bool> mKeyboardNavigation{ mIndex, "GUI", "keyboard navigation" };
        SettingValue<bool> mColorTopicEnable{ mIndex, "GUI", "color topic enable" };
        SettingValue<float> mColorTopicSpecific{ mIndex, "GUI", "color topic specific", makeClampSanitizerFloat(0, 1) };
        SettingValue<float> mColorTopicExhausted{ mIndex, "GUI", "color topic exhausted",
            makeClampSanitizerFloat(0, 1) };
    };
}

#endif
