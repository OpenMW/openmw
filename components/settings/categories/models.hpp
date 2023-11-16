#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_MODELS_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_MODELS_H

#include <components/settings/settingvalue.hpp>

#include <osg/Math>
#include <osg/Vec2f>
#include <osg/Vec3f>

#include <cstdint>
#include <string>
#include <string_view>

namespace Settings
{
    struct ModelsCategory : WithIndex
    {
        using WithIndex::WithIndex;

        SettingValue<bool> mLoadUnsupportedNifFiles{ mIndex, "Models", "load unsupported nif files" };
        SettingValue<std::string> mXbaseanim{ mIndex, "Models", "xbaseanim" };
        SettingValue<std::string> mBaseanim{ mIndex, "Models", "baseanim" };
        SettingValue<std::string> mXbaseanim1st{ mIndex, "Models", "xbaseanim1st" };
        SettingValue<std::string> mBaseanimkna{ mIndex, "Models", "baseanimkna" };
        SettingValue<std::string> mBaseanimkna1st{ mIndex, "Models", "baseanimkna1st" };
        SettingValue<std::string> mXbaseanimfemale{ mIndex, "Models", "xbaseanimfemale" };
        SettingValue<std::string> mBaseanimfemale{ mIndex, "Models", "baseanimfemale" };
        SettingValue<std::string> mBaseanimfemale1st{ mIndex, "Models", "baseanimfemale1st" };
        SettingValue<std::string> mWolfskin{ mIndex, "Models", "wolfskin" };
        SettingValue<std::string> mWolfskin1st{ mIndex, "Models", "wolfskin1st" };
        SettingValue<std::string> mXargonianswimkna{ mIndex, "Models", "xargonianswimkna" };
        SettingValue<std::string> mXbaseanimkf{ mIndex, "Models", "xbaseanimkf" };
        SettingValue<std::string> mXbaseanim1stkf{ mIndex, "Models", "xbaseanim1stkf" };
        SettingValue<std::string> mXbaseanimfemalekf{ mIndex, "Models", "xbaseanimfemalekf" };
        SettingValue<std::string> mXargonianswimknakf{ mIndex, "Models", "xargonianswimknakf" };
        SettingValue<std::string> mSkyatmosphere{ mIndex, "Models", "skyatmosphere" };
        SettingValue<std::string> mSkyclouds{ mIndex, "Models", "skyclouds" };
        SettingValue<std::string> mSkynight01{ mIndex, "Models", "skynight01" };
        SettingValue<std::string> mSkynight02{ mIndex, "Models", "skynight02" };
        SettingValue<std::string> mWeatherashcloud{ mIndex, "Models", "weatherashcloud" };
        SettingValue<std::string> mWeatherblightcloud{ mIndex, "Models", "weatherblightcloud" };
        SettingValue<std::string> mWeathersnow{ mIndex, "Models", "weathersnow" };
        SettingValue<std::string> mWeatherblizzard{ mIndex, "Models", "weatherblizzard" };
        SettingValue<bool> mWriteNifDebugLog{ mIndex, "Models", "write nif debug log" };
    };
}

#endif
