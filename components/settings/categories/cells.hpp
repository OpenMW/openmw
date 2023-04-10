#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_CELLS_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_CELLS_H

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
    struct CellsCategory
    {
        SettingValue<bool> mPreloadEnabled{ "Cells", "preload enabled" };
        SettingValue<int> mPreloadNumThreads{ "Cells", "preload num threads", makeMaxSanitizerInt(1) };
        SettingValue<bool> mPreloadExteriorGrid{ "Cells", "preload exterior grid" };
        SettingValue<bool> mPreloadFastTravel{ "Cells", "preload fast travel" };
        SettingValue<bool> mPreloadDoors{ "Cells", "preload doors" };
        SettingValue<float> mPreloadDistance{ "Cells", "preload distance", makeMaxStrictSanitizerFloat(0) };
        SettingValue<bool> mPreloadInstances{ "Cells", "preload instances" };
        SettingValue<int> mPreloadCellCacheMin{ "Cells", "preload cell cache min", makeMaxSanitizerInt(1) };
        SettingValue<int> mPreloadCellCacheMax{ "Cells", "preload cell cache max", makeMaxSanitizerInt(1) };
        SettingValue<float> mPreloadCellExpiryDelay{ "Cells", "preload cell expiry delay", makeMaxSanitizerFloat(0) };
        SettingValue<float> mPredictionTime{ "Cells", "prediction time", makeMaxSanitizerFloat(0) };
        SettingValue<float> mCacheExpiryDelay{ "Cells", "cache expiry delay", makeMaxSanitizerFloat(0) };
        SettingValue<float> mTargetFramerate{ "Cells", "target framerate", makeMaxStrictSanitizerFloat(0) };
        SettingValue<int> mPointersCacheSize{ "Cells", "pointers cache size", makeClampSanitizerInt(40, 1000) };
    };
}

#endif
