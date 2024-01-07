#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_MAP_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_MAP_H

#include <components/misc/constants.hpp>
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
    struct MapCategory : WithIndex
    {
        using WithIndex::WithIndex;

        SettingValue<int> mGlobalMapCellSize{ mIndex, "Map", "global map cell size", makeClampSanitizerInt(1, 50) };
        SettingValue<bool> mLocalMapHudFogOfWar{ mIndex, "Map", "local map hud fog of war" };
        SettingValue<int> mLocalMapResolution{ mIndex, "Map", "local map resolution", makeMaxSanitizerInt(1) };
        SettingValue<int> mLocalMapWidgetSize{ mIndex, "Map", "local map widget size", makeMaxSanitizerInt(1) };
        SettingValue<bool> mGlobal{ mIndex, "Map", "global" };
        SettingValue<bool> mAllowZooming{ mIndex, "Map", "allow zooming" };
        SettingValue<int> mMaxLocalViewingDistance{ mIndex, "Map", "max local viewing distance",
            makeMaxSanitizerInt(Constants::CellGridRadius) };
    };
}

#endif
