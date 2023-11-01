#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_CELLS_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_CELLS_H

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
    struct CellsCategory : WithIndex
    {
        using WithIndex::WithIndex;

        SettingValue<bool> mPreloadEnabled{ mIndex, "Cells", "preload enabled" };
        SettingValue<int> mPreloadNumThreads{ mIndex, "Cells", "preload num threads", makeMaxSanitizerInt(1) };
        SettingValue<bool> mPreloadExteriorGrid{ mIndex, "Cells", "preload exterior grid" };
        SettingValue<bool> mPreloadFastTravel{ mIndex, "Cells", "preload fast travel" };
        SettingValue<bool> mPreloadDoors{ mIndex, "Cells", "preload doors" };
        SettingValue<float> mPreloadDistance{ mIndex, "Cells", "preload distance", makeMaxStrictSanitizerFloat(0) };
        SettingValue<bool> mPreloadInstances{ mIndex, "Cells", "preload instances" };
        SettingValue<int> mPreloadCellCacheMin{ mIndex, "Cells", "preload cell cache min", makeMaxSanitizerInt(1) };
        SettingValue<int> mPreloadCellCacheMax{ mIndex, "Cells", "preload cell cache max", makeMaxSanitizerInt(1) };
        SettingValue<float> mPreloadCellExpiryDelay{ mIndex, "Cells", "preload cell expiry delay",
            makeMaxSanitizerFloat(0) };
        SettingValue<float> mPredictionTime{ mIndex, "Cells", "prediction time", makeMaxSanitizerFloat(0) };
        SettingValue<float> mCacheExpiryDelay{ mIndex, "Cells", "cache expiry delay", makeMaxSanitizerFloat(0) };
        SettingValue<float> mTargetFramerate{ mIndex, "Cells", "target framerate", makeMaxStrictSanitizerFloat(0) };
        SettingValue<int> mPointersCacheSize{ mIndex, "Cells", "pointers cache size", makeClampSanitizerInt(40, 1000) };
    };
}

#endif
