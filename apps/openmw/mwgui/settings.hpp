#ifndef OPENMW_APPS_OPENMW_MWGUI_SETTINGS_H
#define OPENMW_APPS_OPENMW_MWGUI_SETTINGS_H

#include "components/settings/settingvalue.hpp"

namespace MWGui
{
    struct WindowRectSettingValues
    {
        Settings::SettingValue<float>& mX;
        Settings::SettingValue<float>& mY;
        Settings::SettingValue<float>& mW;
        Settings::SettingValue<float>& mH;
    };

    struct WindowSettingValues
    {
        WindowRectSettingValues mRegular;
        WindowRectSettingValues mMaximized;
        Settings::SettingValue<bool>& mIsMaximized;
    };

    WindowSettingValues makeAlchemyWindowSettingValues();
    WindowSettingValues makeBarterWindowSettingValues();
    WindowSettingValues makeCompanionWindowSettingValues();
    WindowSettingValues makeConsoleWindowSettingValues();
    WindowSettingValues makeContainerWindowSettingValues();
    WindowSettingValues makeDebugWindowSettingValues();
    WindowSettingValues makeDialogueWindowSettingValues();
    WindowSettingValues makeInventoryWindowSettingValues();
    WindowSettingValues makeInventoryBarterWindowSettingValues();
    WindowSettingValues makeInventoryCompanionWindowSettingValues();
    WindowSettingValues makeInventoryContainerWindowSettingValues();
    WindowSettingValues makeMapWindowSettingValues();
    WindowSettingValues makePostprocessorWindowSettingValues();
    WindowSettingValues makeSettingsWindowSettingValues();
    WindowSettingValues makeSpellsWindowSettingValues();
    WindowSettingValues makeStatsWindowSettingValues();
}

#endif
