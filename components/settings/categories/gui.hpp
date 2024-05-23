#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_GUI_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_GUI_H

#include <components/settings/sanitizerimpl.hpp>
#include <components/settings/settingvalue.hpp>

#include <osg/Math>
#include <osg/Vec2f>
#include <osg/Vec3f>

#include <MyGUI_Colour.h>

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
        SettingValue<MyGUI::Colour> mColorBackgroundOwned{ mIndex, "GUI", "color background owned" };
        SettingValue<MyGUI::Colour> mColorCrosshairOwned{ mIndex, "GUI", "color crosshair owned" };
        SettingValue<bool> mKeyboardNavigation{ mIndex, "GUI", "keyboard navigation" };
        SettingValue<bool> mColorTopicEnable{ mIndex, "GUI", "color topic enable" };
        SettingValue<MyGUI::Colour> mColorTopicSpecific{ mIndex, "GUI", "FontColor_color_specific" };
        SettingValue<MyGUI::Colour> mColorTopicSpecificOver{ mIndex, "GUI", "FontColor_color_specific_over" };
        SettingValue<MyGUI::Colour> mColorTopicSpecificPressed{ mIndex, "GUI", "FontColor_color_specific_pressed" };
        SettingValue<MyGUI::Colour> mColorTopicExhausted{ mIndex, "GUI", "FontColor_color_exhausted" };
        SettingValue<MyGUI::Colour> mColorTopicExhaustedOver{ mIndex, "GUI", "FontColor_color_exhausted_over" };
        SettingValue<MyGUI::Colour> mColorTopicExhaustedPressed{ mIndex, "GUI", "FontColor_color_exhausted_pressed" };
    };
}

#endif
