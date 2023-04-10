#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_MAP_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_MAP_H

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
    struct MapCategory
    {
        SettingValue<int> mGlobalMapCellSize{ "Map", "global map cell size", makeMaxSanitizerInt(1) };
        SettingValue<int> mLocalMapHudWidgetSize{ "Map", "local map hud widget size", makeMaxSanitizerInt(1) };
        SettingValue<bool> mLocalMapHudFogOfWar{ "Map", "local map hud fog of war" };
        SettingValue<int> mLocalMapResolution{ "Map", "local map resolution", makeMaxSanitizerInt(1) };
        SettingValue<int> mLocalMapWidgetSize{ "Map", "local map widget size", makeMaxSanitizerInt(1) };
        SettingValue<bool> mGlobal{ "Map", "global" };
        SettingValue<bool> mAllowZooming{ "Map", "allow zooming" };
        SettingValue<int> mMaxLocalViewingDistance{ "Map", "max local viewing distance", makeMaxSanitizerInt(1) };
    };
}

#endif
