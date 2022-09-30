#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_POSTPROCESSING_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_POSTPROCESSING_H

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
    struct PostProcessingCategory
    {
        SettingValue<bool> mEnabled{ "Post Processing", "enabled" };
        SettingValue<std::string> mChain{ "Post Processing", "chain" };
        SettingValue<float> mAutoExposureSpeed{ "Post Processing", "auto exposure speed",
            makeMaxStrictSanitizerFloat(0.0001f) };
        SettingValue<bool> mTransparentPostpass{ "Post Processing", "transparent postpass" };
    };
}

#endif
