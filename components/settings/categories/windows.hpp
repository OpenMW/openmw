#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_WINDOWS_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_WINDOWS_H

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
    struct WindowsCategory
    {
        SettingValue<float> mStatsX{ "Windows", "stats x" };
        SettingValue<float> mStatsY{ "Windows", "stats y" };
        SettingValue<float> mStatsW{ "Windows", "stats w" };
        SettingValue<float> mStatsH{ "Windows", "stats h" };
        SettingValue<float> mStatsMaximizedX{ "Windows", "stats maximized x" };
        SettingValue<float> mStatsMaximizedY{ "Windows", "stats maximized y" };
        SettingValue<float> mStatsMaximizedW{ "Windows", "stats maximized w" };
        SettingValue<float> mStatsMaximizedH{ "Windows", "stats maximized h" };
        SettingValue<bool> mStatsPin{ "Windows", "stats pin" };
        SettingValue<bool> mStatsHidden{ "Windows", "stats hidden" };
        SettingValue<bool> mStatsMaximized{ "Windows", "stats maximized" };
        SettingValue<float> mSpellsX{ "Windows", "spells x" };
        SettingValue<float> mSpellsY{ "Windows", "spells y" };
        SettingValue<float> mSpellsW{ "Windows", "spells w" };
        SettingValue<float> mSpellsH{ "Windows", "spells h" };
        SettingValue<float> mSpellsMaximizedX{ "Windows", "spells maximized x" };
        SettingValue<float> mSpellsMaximizedY{ "Windows", "spells maximized y" };
        SettingValue<float> mSpellsMaximizedW{ "Windows", "spells maximized w" };
        SettingValue<float> mSpellsMaximizedH{ "Windows", "spells maximized h" };
        SettingValue<bool> mSpellsPin{ "Windows", "spells pin" };
        SettingValue<bool> mSpellsHidden{ "Windows", "spells hidden" };
        SettingValue<bool> mSpellsMaximized{ "Windows", "spells maximized" };
        SettingValue<float> mMapX{ "Windows", "map x" };
        SettingValue<float> mMapY{ "Windows", "map y" };
        SettingValue<float> mMapW{ "Windows", "map w" };
        SettingValue<float> mMapH{ "Windows", "map h" };
        SettingValue<float> mMapMaximizedX{ "Windows", "map maximized x" };
        SettingValue<float> mMapMaximizedY{ "Windows", "map maximized y" };
        SettingValue<float> mMapMaximizedW{ "Windows", "map maximized w" };
        SettingValue<float> mMapMaximizedH{ "Windows", "map maximized h" };
        SettingValue<bool> mMapPin{ "Windows", "map pin" };
        SettingValue<bool> mMapHidden{ "Windows", "map hidden" };
        SettingValue<bool> mMapMaximized{ "Windows", "map maximized" };
        SettingValue<float> mInventoryX{ "Windows", "inventory x" };
        SettingValue<float> mInventoryY{ "Windows", "inventory y" };
        SettingValue<float> mInventoryW{ "Windows", "inventory w" };
        SettingValue<float> mInventoryH{ "Windows", "inventory h" };
        SettingValue<float> mInventoryMaximizedX{ "Windows", "inventory maximized x" };
        SettingValue<float> mInventoryMaximizedY{ "Windows", "inventory maximized y" };
        SettingValue<float> mInventoryMaximizedW{ "Windows", "inventory maximized w" };
        SettingValue<float> mInventoryMaximizedH{ "Windows", "inventory maximized h" };
        SettingValue<bool> mInventoryPin{ "Windows", "inventory pin" };
        SettingValue<bool> mInventoryHidden{ "Windows", "inventory hidden" };
        SettingValue<bool> mInventoryMaximized{ "Windows", "inventory maximized" };
        SettingValue<float> mInventoryContainerX{ "Windows", "inventory container x" };
        SettingValue<float> mInventoryContainerY{ "Windows", "inventory container y" };
        SettingValue<float> mInventoryContainerW{ "Windows", "inventory container w" };
        SettingValue<float> mInventoryContainerH{ "Windows", "inventory container h" };
        SettingValue<float> mInventoryContainerMaximizedX{ "Windows", "inventory container maximized x" };
        SettingValue<float> mInventoryContainerMaximizedY{ "Windows", "inventory container maximized y" };
        SettingValue<float> mInventoryContainerMaximizedW{ "Windows", "inventory container maximized w" };
        SettingValue<float> mInventoryContainerMaximizedH{ "Windows", "inventory container maximized h" };
        SettingValue<bool> mInventoryContainerMaximized{ "Windows", "inventory container maximized" };
        SettingValue<float> mInventoryBarterX{ "Windows", "inventory barter x" };
        SettingValue<float> mInventoryBarterY{ "Windows", "inventory barter y" };
        SettingValue<float> mInventoryBarterW{ "Windows", "inventory barter w" };
        SettingValue<float> mInventoryBarterH{ "Windows", "inventory barter h" };
        SettingValue<float> mInventoryBarterMaximizedX{ "Windows", "inventory barter maximized x" };
        SettingValue<float> mInventoryBarterMaximizedY{ "Windows", "inventory barter maximized y" };
        SettingValue<float> mInventoryBarterMaximizedW{ "Windows", "inventory barter maximized w" };
        SettingValue<float> mInventoryBarterMaximizedH{ "Windows", "inventory barter maximized h" };
        SettingValue<bool> mInventoryBarterMaximized{ "Windows", "inventory barter maximized" };
        SettingValue<float> mInventoryCompanionX{ "Windows", "inventory companion x" };
        SettingValue<float> mInventoryCompanionY{ "Windows", "inventory companion y" };
        SettingValue<float> mInventoryCompanionW{ "Windows", "inventory companion w" };
        SettingValue<float> mInventoryCompanionH{ "Windows", "inventory companion h" };
        SettingValue<float> mInventoryCompanionMaximizedX{ "Windows", "inventory companion maximized x" };
        SettingValue<float> mInventoryCompanionMaximizedY{ "Windows", "inventory companion maximized y" };
        SettingValue<float> mInventoryCompanionMaximizedW{ "Windows", "inventory companion maximized w" };
        SettingValue<float> mInventoryCompanionMaximizedH{ "Windows", "inventory companion maximized h" };
        SettingValue<bool> mInventoryCompanionMaximized{ "Windows", "inventory companion maximized" };
        SettingValue<float> mDialogueX{ "Windows", "dialogue x" };
        SettingValue<float> mDialogueY{ "Windows", "dialogue y" };
        SettingValue<float> mDialogueW{ "Windows", "dialogue w" };
        SettingValue<float> mDialogueH{ "Windows", "dialogue h" };
        SettingValue<float> mDialogueMaximizedX{ "Windows", "dialogue maximized x" };
        SettingValue<float> mDialogueMaximizedY{ "Windows", "dialogue maximized y" };
        SettingValue<float> mDialogueMaximizedW{ "Windows", "dialogue maximized w" };
        SettingValue<float> mDialogueMaximizedH{ "Windows", "dialogue maximized h" };
        SettingValue<bool> mDialogueMaximized{ "Windows", "dialogue maximized" };
        SettingValue<float> mAlchemyX{ "Windows", "alchemy x" };
        SettingValue<float> mAlchemyY{ "Windows", "alchemy y" };
        SettingValue<float> mAlchemyW{ "Windows", "alchemy w" };
        SettingValue<float> mAlchemyH{ "Windows", "alchemy h" };
        SettingValue<float> mAlchemyMaximizedX{ "Windows", "alchemy maximized x" };
        SettingValue<float> mAlchemyMaximizedY{ "Windows", "alchemy maximized y" };
        SettingValue<float> mAlchemyMaximizedW{ "Windows", "alchemy maximized w" };
        SettingValue<float> mAlchemyMaximizedH{ "Windows", "alchemy maximized h" };
        SettingValue<bool> mAlchemyMaximized{ "Windows", "alchemy maximized" };
        SettingValue<float> mConsoleX{ "Windows", "console x" };
        SettingValue<float> mConsoleY{ "Windows", "console y" };
        SettingValue<float> mConsoleW{ "Windows", "console w" };
        SettingValue<float> mConsoleH{ "Windows", "console h" };
        SettingValue<float> mConsoleMaximizedX{ "Windows", "console maximized x" };
        SettingValue<float> mConsoleMaximizedY{ "Windows", "console maximized y" };
        SettingValue<float> mConsoleMaximizedW{ "Windows", "console maximized w" };
        SettingValue<float> mConsoleMaximizedH{ "Windows", "console maximized h" };
        SettingValue<bool> mConsoleMaximized{ "Windows", "console maximized" };
        SettingValue<float> mContainerX{ "Windows", "container x" };
        SettingValue<float> mContainerY{ "Windows", "container y" };
        SettingValue<float> mContainerW{ "Windows", "container w" };
        SettingValue<float> mContainerH{ "Windows", "container h" };
        SettingValue<float> mContainerMaximizedX{ "Windows", "container maximized x" };
        SettingValue<float> mContainerMaximizedY{ "Windows", "container maximized y" };
        SettingValue<float> mContainerMaximizedW{ "Windows", "container maximized w" };
        SettingValue<float> mContainerMaximizedH{ "Windows", "container maximized h" };
        SettingValue<bool> mContainerMaximized{ "Windows", "container maximized" };
        SettingValue<float> mBarterX{ "Windows", "barter x" };
        SettingValue<float> mBarterY{ "Windows", "barter y" };
        SettingValue<float> mBarterW{ "Windows", "barter w" };
        SettingValue<float> mBarterH{ "Windows", "barter h" };
        SettingValue<float> mBarterMaximizedX{ "Windows", "barter maximized x" };
        SettingValue<float> mBarterMaximizedY{ "Windows", "barter maximized y" };
        SettingValue<float> mBarterMaximizedW{ "Windows", "barter maximized w" };
        SettingValue<float> mBarterMaximizedH{ "Windows", "barter maximized h" };
        SettingValue<bool> mBarterMaximized{ "Windows", "barter maximized" };
        SettingValue<float> mCompanionX{ "Windows", "companion x" };
        SettingValue<float> mCompanionY{ "Windows", "companion y" };
        SettingValue<float> mCompanionW{ "Windows", "companion w" };
        SettingValue<float> mCompanionH{ "Windows", "companion h" };
        SettingValue<float> mCompanionMaximizedX{ "Windows", "companion maximized x" };
        SettingValue<float> mCompanionMaximizedY{ "Windows", "companion maximized y" };
        SettingValue<float> mCompanionMaximizedW{ "Windows", "companion maximized w" };
        SettingValue<float> mCompanionMaximizedH{ "Windows", "companion maximized h" };
        SettingValue<bool> mCompanionMaximized{ "Windows", "companion maximized" };
        SettingValue<float> mSettingsX{ "Windows", "settings x" };
        SettingValue<float> mSettingsY{ "Windows", "settings y" };
        SettingValue<float> mSettingsW{ "Windows", "settings w" };
        SettingValue<float> mSettingsH{ "Windows", "settings h" };
        SettingValue<float> mSettingsMaximizedX{ "Windows", "settings maximized x" };
        SettingValue<float> mSettingsMaximizedY{ "Windows", "settings maximized y" };
        SettingValue<float> mSettingsMaximizedW{ "Windows", "settings maximized w" };
        SettingValue<float> mSettingsMaximizedH{ "Windows", "settings maximized h" };
        SettingValue<bool> mSettingsMaximized{ "Windows", "settings maximized" };
        SettingValue<float> mPostprocessorH{ "Windows", "postprocessor h" };
        SettingValue<float> mPostprocessorW{ "Windows", "postprocessor w" };
        SettingValue<float> mPostprocessorX{ "Windows", "postprocessor x" };
        SettingValue<float> mPostprocessorY{ "Windows", "postprocessor y" };
        SettingValue<float> mPostprocessorMaximizedX{ "Windows", "postprocessor maximized x" };
        SettingValue<float> mPostprocessorMaximizedY{ "Windows", "postprocessor maximized y" };
        SettingValue<float> mPostprocessorMaximizedW{ "Windows", "postprocessor maximized w" };
        SettingValue<float> mPostprocessorMaximizedH{ "Windows", "postprocessor maximized h" };
        SettingValue<bool> mPostprocessorMaximized{ "Windows", "postprocessor maximized" };
    };
}

#endif
