#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_MODELS_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_MODELS_H

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
    struct ModelsCategory
    {
        SettingValue<bool> mLoadUnsupportedNifFiles{ "Models", "load unsupported nif files" };
        SettingValue<std::string> mXbaseanim{ "Models", "xbaseanim" };
        SettingValue<std::string> mBaseanim{ "Models", "baseanim" };
        SettingValue<std::string> mXbaseanim1st{ "Models", "xbaseanim1st" };
        SettingValue<std::string> mBaseanimkna{ "Models", "baseanimkna" };
        SettingValue<std::string> mBaseanimkna1st{ "Models", "baseanimkna1st" };
        SettingValue<std::string> mXbaseanimfemale{ "Models", "xbaseanimfemale" };
        SettingValue<std::string> mBaseanimfemale{ "Models", "baseanimfemale" };
        SettingValue<std::string> mBaseanimfemale1st{ "Models", "baseanimfemale1st" };
        SettingValue<std::string> mWolfskin{ "Models", "wolfskin" };
        SettingValue<std::string> mWolfskin1st{ "Models", "wolfskin1st" };
        SettingValue<std::string> mXargonianswimkna{ "Models", "xargonianswimkna" };
        SettingValue<std::string> mXbaseanimkf{ "Models", "xbaseanimkf" };
        SettingValue<std::string> mXbaseanim1stkf{ "Models", "xbaseanim1stkf" };
        SettingValue<std::string> mXbaseanimfemalekf{ "Models", "xbaseanimfemalekf" };
        SettingValue<std::string> mXargonianswimknakf{ "Models", "xargonianswimknakf" };
        SettingValue<std::string> mSkyatmosphere{ "Models", "skyatmosphere" };
        SettingValue<std::string> mSkyclouds{ "Models", "skyclouds" };
        SettingValue<std::string> mSkynight01{ "Models", "skynight01" };
        SettingValue<std::string> mSkynight02{ "Models", "skynight02" };
        SettingValue<std::string> mWeatherashcloud{ "Models", "weatherashcloud" };
        SettingValue<std::string> mWeatherblightcloud{ "Models", "weatherblightcloud" };
        SettingValue<std::string> mWeathersnow{ "Models", "weathersnow" };
        SettingValue<std::string> mWeatherblizzard{ "Models", "weatherblizzard" };
        SettingValue<bool> mWriteNifDebugLog{ "Models", "write nif debug log" };
    };
}

#endif
