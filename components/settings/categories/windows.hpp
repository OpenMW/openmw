#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_WINDOWS_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_WINDOWS_H

#include <components/settings/settingvalue.hpp>

#include <osg/Math>
#include <osg/Vec2f>
#include <osg/Vec3f>

#include <cstdint>
#include <string>
#include <string_view>

namespace Settings
{
    struct WindowsCategory : WithIndex
    {
        using WithIndex::WithIndex;

        SettingValue<float> mStatsX{ mIndex, "Windows", "stats x" };
        SettingValue<float> mStatsY{ mIndex, "Windows", "stats y" };
        SettingValue<float> mStatsW{ mIndex, "Windows", "stats w" };
        SettingValue<float> mStatsH{ mIndex, "Windows", "stats h" };
        SettingValue<float> mStatsMaximizedX{ mIndex, "Windows", "stats maximized x" };
        SettingValue<float> mStatsMaximizedY{ mIndex, "Windows", "stats maximized y" };
        SettingValue<float> mStatsMaximizedW{ mIndex, "Windows", "stats maximized w" };
        SettingValue<float> mStatsMaximizedH{ mIndex, "Windows", "stats maximized h" };
        SettingValue<bool> mStatsPin{ mIndex, "Windows", "stats pin" };
        SettingValue<bool> mStatsHidden{ mIndex, "Windows", "stats hidden" };
        SettingValue<bool> mStatsMaximized{ mIndex, "Windows", "stats maximized" };
        SettingValue<float> mSpellsX{ mIndex, "Windows", "spells x" };
        SettingValue<float> mSpellsY{ mIndex, "Windows", "spells y" };
        SettingValue<float> mSpellsW{ mIndex, "Windows", "spells w" };
        SettingValue<float> mSpellsH{ mIndex, "Windows", "spells h" };
        SettingValue<float> mSpellsMaximizedX{ mIndex, "Windows", "spells maximized x" };
        SettingValue<float> mSpellsMaximizedY{ mIndex, "Windows", "spells maximized y" };
        SettingValue<float> mSpellsMaximizedW{ mIndex, "Windows", "spells maximized w" };
        SettingValue<float> mSpellsMaximizedH{ mIndex, "Windows", "spells maximized h" };
        SettingValue<bool> mSpellsPin{ mIndex, "Windows", "spells pin" };
        SettingValue<bool> mSpellsHidden{ mIndex, "Windows", "spells hidden" };
        SettingValue<bool> mSpellsMaximized{ mIndex, "Windows", "spells maximized" };
        SettingValue<float> mMapX{ mIndex, "Windows", "map x" };
        SettingValue<float> mMapY{ mIndex, "Windows", "map y" };
        SettingValue<float> mMapW{ mIndex, "Windows", "map w" };
        SettingValue<float> mMapH{ mIndex, "Windows", "map h" };
        SettingValue<float> mMapMaximizedX{ mIndex, "Windows", "map maximized x" };
        SettingValue<float> mMapMaximizedY{ mIndex, "Windows", "map maximized y" };
        SettingValue<float> mMapMaximizedW{ mIndex, "Windows", "map maximized w" };
        SettingValue<float> mMapMaximizedH{ mIndex, "Windows", "map maximized h" };
        SettingValue<bool> mMapPin{ mIndex, "Windows", "map pin" };
        SettingValue<bool> mMapHidden{ mIndex, "Windows", "map hidden" };
        SettingValue<bool> mMapMaximized{ mIndex, "Windows", "map maximized" };
        SettingValue<float> mInventoryX{ mIndex, "Windows", "inventory x" };
        SettingValue<float> mInventoryY{ mIndex, "Windows", "inventory y" };
        SettingValue<float> mInventoryW{ mIndex, "Windows", "inventory w" };
        SettingValue<float> mInventoryH{ mIndex, "Windows", "inventory h" };
        SettingValue<float> mInventoryMaximizedX{ mIndex, "Windows", "inventory maximized x" };
        SettingValue<float> mInventoryMaximizedY{ mIndex, "Windows", "inventory maximized y" };
        SettingValue<float> mInventoryMaximizedW{ mIndex, "Windows", "inventory maximized w" };
        SettingValue<float> mInventoryMaximizedH{ mIndex, "Windows", "inventory maximized h" };
        SettingValue<bool> mInventoryPin{ mIndex, "Windows", "inventory pin" };
        SettingValue<bool> mInventoryHidden{ mIndex, "Windows", "inventory hidden" };
        SettingValue<bool> mInventoryMaximized{ mIndex, "Windows", "inventory maximized" };
        SettingValue<float> mInventoryContainerX{ mIndex, "Windows", "inventory container x" };
        SettingValue<float> mInventoryContainerY{ mIndex, "Windows", "inventory container y" };
        SettingValue<float> mInventoryContainerW{ mIndex, "Windows", "inventory container w" };
        SettingValue<float> mInventoryContainerH{ mIndex, "Windows", "inventory container h" };
        SettingValue<float> mInventoryContainerMaximizedX{ mIndex, "Windows", "inventory container maximized x" };
        SettingValue<float> mInventoryContainerMaximizedY{ mIndex, "Windows", "inventory container maximized y" };
        SettingValue<float> mInventoryContainerMaximizedW{ mIndex, "Windows", "inventory container maximized w" };
        SettingValue<float> mInventoryContainerMaximizedH{ mIndex, "Windows", "inventory container maximized h" };
        SettingValue<bool> mInventoryContainerMaximized{ mIndex, "Windows", "inventory container maximized" };
        SettingValue<float> mInventoryBarterX{ mIndex, "Windows", "inventory barter x" };
        SettingValue<float> mInventoryBarterY{ mIndex, "Windows", "inventory barter y" };
        SettingValue<float> mInventoryBarterW{ mIndex, "Windows", "inventory barter w" };
        SettingValue<float> mInventoryBarterH{ mIndex, "Windows", "inventory barter h" };
        SettingValue<float> mInventoryBarterMaximizedX{ mIndex, "Windows", "inventory barter maximized x" };
        SettingValue<float> mInventoryBarterMaximizedY{ mIndex, "Windows", "inventory barter maximized y" };
        SettingValue<float> mInventoryBarterMaximizedW{ mIndex, "Windows", "inventory barter maximized w" };
        SettingValue<float> mInventoryBarterMaximizedH{ mIndex, "Windows", "inventory barter maximized h" };
        SettingValue<bool> mInventoryBarterMaximized{ mIndex, "Windows", "inventory barter maximized" };
        SettingValue<float> mInventoryCompanionX{ mIndex, "Windows", "inventory companion x" };
        SettingValue<float> mInventoryCompanionY{ mIndex, "Windows", "inventory companion y" };
        SettingValue<float> mInventoryCompanionW{ mIndex, "Windows", "inventory companion w" };
        SettingValue<float> mInventoryCompanionH{ mIndex, "Windows", "inventory companion h" };
        SettingValue<float> mInventoryCompanionMaximizedX{ mIndex, "Windows", "inventory companion maximized x" };
        SettingValue<float> mInventoryCompanionMaximizedY{ mIndex, "Windows", "inventory companion maximized y" };
        SettingValue<float> mInventoryCompanionMaximizedW{ mIndex, "Windows", "inventory companion maximized w" };
        SettingValue<float> mInventoryCompanionMaximizedH{ mIndex, "Windows", "inventory companion maximized h" };
        SettingValue<bool> mInventoryCompanionMaximized{ mIndex, "Windows", "inventory companion maximized" };
        SettingValue<float> mDialogueX{ mIndex, "Windows", "dialogue x" };
        SettingValue<float> mDialogueY{ mIndex, "Windows", "dialogue y" };
        SettingValue<float> mDialogueW{ mIndex, "Windows", "dialogue w" };
        SettingValue<float> mDialogueH{ mIndex, "Windows", "dialogue h" };
        SettingValue<float> mDialogueMaximizedX{ mIndex, "Windows", "dialogue maximized x" };
        SettingValue<float> mDialogueMaximizedY{ mIndex, "Windows", "dialogue maximized y" };
        SettingValue<float> mDialogueMaximizedW{ mIndex, "Windows", "dialogue maximized w" };
        SettingValue<float> mDialogueMaximizedH{ mIndex, "Windows", "dialogue maximized h" };
        SettingValue<bool> mDialogueMaximized{ mIndex, "Windows", "dialogue maximized" };
        SettingValue<float> mAlchemyX{ mIndex, "Windows", "alchemy x" };
        SettingValue<float> mAlchemyY{ mIndex, "Windows", "alchemy y" };
        SettingValue<float> mAlchemyW{ mIndex, "Windows", "alchemy w" };
        SettingValue<float> mAlchemyH{ mIndex, "Windows", "alchemy h" };
        SettingValue<float> mAlchemyMaximizedX{ mIndex, "Windows", "alchemy maximized x" };
        SettingValue<float> mAlchemyMaximizedY{ mIndex, "Windows", "alchemy maximized y" };
        SettingValue<float> mAlchemyMaximizedW{ mIndex, "Windows", "alchemy maximized w" };
        SettingValue<float> mAlchemyMaximizedH{ mIndex, "Windows", "alchemy maximized h" };
        SettingValue<bool> mAlchemyMaximized{ mIndex, "Windows", "alchemy maximized" };
        SettingValue<float> mConsoleX{ mIndex, "Windows", "console x" };
        SettingValue<float> mConsoleY{ mIndex, "Windows", "console y" };
        SettingValue<float> mConsoleW{ mIndex, "Windows", "console w" };
        SettingValue<float> mConsoleH{ mIndex, "Windows", "console h" };
        SettingValue<float> mConsoleMaximizedX{ mIndex, "Windows", "console maximized x" };
        SettingValue<float> mConsoleMaximizedY{ mIndex, "Windows", "console maximized y" };
        SettingValue<float> mConsoleMaximizedW{ mIndex, "Windows", "console maximized w" };
        SettingValue<float> mConsoleMaximizedH{ mIndex, "Windows", "console maximized h" };
        SettingValue<bool> mConsoleMaximized{ mIndex, "Windows", "console maximized" };
        SettingValue<float> mContainerX{ mIndex, "Windows", "container x" };
        SettingValue<float> mContainerY{ mIndex, "Windows", "container y" };
        SettingValue<float> mContainerW{ mIndex, "Windows", "container w" };
        SettingValue<float> mContainerH{ mIndex, "Windows", "container h" };
        SettingValue<float> mContainerMaximizedX{ mIndex, "Windows", "container maximized x" };
        SettingValue<float> mContainerMaximizedY{ mIndex, "Windows", "container maximized y" };
        SettingValue<float> mContainerMaximizedW{ mIndex, "Windows", "container maximized w" };
        SettingValue<float> mContainerMaximizedH{ mIndex, "Windows", "container maximized h" };
        SettingValue<bool> mContainerMaximized{ mIndex, "Windows", "container maximized" };
        SettingValue<float> mBarterX{ mIndex, "Windows", "barter x" };
        SettingValue<float> mBarterY{ mIndex, "Windows", "barter y" };
        SettingValue<float> mBarterW{ mIndex, "Windows", "barter w" };
        SettingValue<float> mBarterH{ mIndex, "Windows", "barter h" };
        SettingValue<float> mBarterMaximizedX{ mIndex, "Windows", "barter maximized x" };
        SettingValue<float> mBarterMaximizedY{ mIndex, "Windows", "barter maximized y" };
        SettingValue<float> mBarterMaximizedW{ mIndex, "Windows", "barter maximized w" };
        SettingValue<float> mBarterMaximizedH{ mIndex, "Windows", "barter maximized h" };
        SettingValue<bool> mBarterMaximized{ mIndex, "Windows", "barter maximized" };
        SettingValue<float> mCompanionX{ mIndex, "Windows", "companion x" };
        SettingValue<float> mCompanionY{ mIndex, "Windows", "companion y" };
        SettingValue<float> mCompanionW{ mIndex, "Windows", "companion w" };
        SettingValue<float> mCompanionH{ mIndex, "Windows", "companion h" };
        SettingValue<float> mCompanionMaximizedX{ mIndex, "Windows", "companion maximized x" };
        SettingValue<float> mCompanionMaximizedY{ mIndex, "Windows", "companion maximized y" };
        SettingValue<float> mCompanionMaximizedW{ mIndex, "Windows", "companion maximized w" };
        SettingValue<float> mCompanionMaximizedH{ mIndex, "Windows", "companion maximized h" };
        SettingValue<bool> mCompanionMaximized{ mIndex, "Windows", "companion maximized" };
        SettingValue<float> mSettingsX{ mIndex, "Windows", "settings x" };
        SettingValue<float> mSettingsY{ mIndex, "Windows", "settings y" };
        SettingValue<float> mSettingsW{ mIndex, "Windows", "settings w" };
        SettingValue<float> mSettingsH{ mIndex, "Windows", "settings h" };
        SettingValue<float> mSettingsMaximizedX{ mIndex, "Windows", "settings maximized x" };
        SettingValue<float> mSettingsMaximizedY{ mIndex, "Windows", "settings maximized y" };
        SettingValue<float> mSettingsMaximizedW{ mIndex, "Windows", "settings maximized w" };
        SettingValue<float> mSettingsMaximizedH{ mIndex, "Windows", "settings maximized h" };
        SettingValue<bool> mSettingsMaximized{ mIndex, "Windows", "settings maximized" };
        SettingValue<float> mPostprocessorH{ mIndex, "Windows", "postprocessor h" };
        SettingValue<float> mPostprocessorW{ mIndex, "Windows", "postprocessor w" };
        SettingValue<float> mPostprocessorX{ mIndex, "Windows", "postprocessor x" };
        SettingValue<float> mPostprocessorY{ mIndex, "Windows", "postprocessor y" };
        SettingValue<float> mPostprocessorMaximizedX{ mIndex, "Windows", "postprocessor maximized x" };
        SettingValue<float> mPostprocessorMaximizedY{ mIndex, "Windows", "postprocessor maximized y" };
        SettingValue<float> mPostprocessorMaximizedW{ mIndex, "Windows", "postprocessor maximized w" };
        SettingValue<float> mPostprocessorMaximizedH{ mIndex, "Windows", "postprocessor maximized h" };
        SettingValue<bool> mPostprocessorMaximized{ mIndex, "Windows", "postprocessor maximized" };
        SettingValue<float> mDebugX{ mIndex, "Windows", "debug x" };
        SettingValue<float> mDebugY{ mIndex, "Windows", "debug y" };
        SettingValue<float> mDebugW{ mIndex, "Windows", "debug w" };
        SettingValue<float> mDebugH{ mIndex, "Windows", "debug h" };
        SettingValue<float> mDebugMaximizedX{ mIndex, "Windows", "debug maximized x" };
        SettingValue<float> mDebugMaximizedY{ mIndex, "Windows", "debug maximized y" };
        SettingValue<float> mDebugMaximizedW{ mIndex, "Windows", "debug maximized w" };
        SettingValue<float> mDebugMaximizedH{ mIndex, "Windows", "debug maximized h" };
        SettingValue<bool> mDebugMaximized{ mIndex, "Windows", "debug maximized" };
    };
}

#endif
