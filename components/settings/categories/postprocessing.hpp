#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_POSTPROCESSING_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_POSTPROCESSING_H

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
    struct PostProcessingCategory : WithIndex
    {
        using WithIndex::WithIndex;

        SettingValue<bool> mEnabled{ mIndex, "Post Processing", "enabled" };
        SettingValue<std::vector<std::string>> mChain{ mIndex, "Post Processing", "chain" };
        SettingValue<float> mAutoExposureSpeed{ mIndex, "Post Processing", "auto exposure speed",
            makeMaxStrictSanitizerFloat(0.0001f) };
        SettingValue<bool> mTransparentPostpass{ mIndex, "Post Processing", "transparent postpass" };
    };
}

#endif
