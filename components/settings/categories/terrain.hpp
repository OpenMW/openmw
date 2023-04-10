#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_TERRAIN_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_TERRAIN_H

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
    struct TerrainCategory
    {
        SettingValue<bool> mDistantTerrain{ "Terrain", "distant terrain" };
        SettingValue<float> mLodFactor{ "Terrain", "lod factor", makeMaxStrictSanitizerFloat(0) };
        SettingValue<int> mVertexLodMod{ "Terrain", "vertex lod mod" };
        SettingValue<int> mCompositeMapLevel{ "Terrain", "composite map level", makeMaxSanitizerInt(-3) };
        SettingValue<int> mCompositeMapResolution{ "Terrain", "composite map resolution", makeMaxSanitizerInt(1) };
        SettingValue<float> mMaxCompositeGeometrySize{ "Terrain", "max composite geometry size",
            makeMaxSanitizerFloat(1) };
        SettingValue<bool> mDebugChunks{ "Terrain", "debug chunks" };
        SettingValue<bool> mObjectPaging{ "Terrain", "object paging" };
        SettingValue<bool> mObjectPagingActiveGrid{ "Terrain", "object paging active grid" };
        SettingValue<float> mObjectPagingMergeFactor{ "Terrain", "object paging merge factor",
            makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mObjectPagingMinSize{ "Terrain", "object paging min size", makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mObjectPagingMinSizeMergeFactor{ "Terrain", "object paging min size merge factor",
            makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mObjectPagingMinSizeCostMultiplier{ "Terrain", "object paging min size cost multiplier",
            makeMaxStrictSanitizerFloat(0) };
    };
}

#endif
