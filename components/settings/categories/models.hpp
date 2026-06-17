#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_MODELS_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_MODELS_H

#include <components/settings/settingvalue.hpp>
#include <components/vfs/pathutil.hpp>

namespace Settings
{
    struct ModelsCategory : WithIndex
    {
        using WithIndex::WithIndex;

        SettingValue<bool> mLoadUnsupportedNifFiles{ mIndex, "Models", "load unsupported nif files" };
        SettingValue<VFS::Path::Normalized> mXbaseanim{ mIndex, "Models", "xbaseanim" };
        SettingValue<VFS::Path::Normalized> mBaseanim{ mIndex, "Models", "baseanim" };
        SettingValue<VFS::Path::Normalized> mXbaseanim1st{ mIndex, "Models", "xbaseanim1st" };
        SettingValue<VFS::Path::Normalized> mBaseanimkna{ mIndex, "Models", "baseanimkna" };
        SettingValue<VFS::Path::Normalized> mBaseanimkna1st{ mIndex, "Models", "baseanimkna1st" };
        SettingValue<VFS::Path::Normalized> mXbaseanimfemale{ mIndex, "Models", "xbaseanimfemale" };
        SettingValue<VFS::Path::Normalized> mBaseanimfemale{ mIndex, "Models", "baseanimfemale" };
        SettingValue<VFS::Path::Normalized> mBaseanimfemale1st{ mIndex, "Models", "baseanimfemale1st" };
        SettingValue<VFS::Path::Normalized> mWolfskin{ mIndex, "Models", "wolfskin" };
        SettingValue<VFS::Path::Normalized> mWolfskin1st{ mIndex, "Models", "wolfskin1st" };
        SettingValue<VFS::Path::Normalized> mXargonianswimkna{ mIndex, "Models", "xargonianswimkna" };
        SettingValue<VFS::Path::Normalized> mXbaseanimkf{ mIndex, "Models", "xbaseanimkf" };
        SettingValue<VFS::Path::Normalized> mXbaseanim1stkf{ mIndex, "Models", "xbaseanim1stkf" };
        SettingValue<VFS::Path::Normalized> mXbaseanimfemalekf{ mIndex, "Models", "xbaseanimfemalekf" };
        SettingValue<VFS::Path::Normalized> mXargonianswimknakf{ mIndex, "Models", "xargonianswimknakf" };
        SettingValue<VFS::Path::Normalized> mSkyatmosphere{ mIndex, "Models", "skyatmosphere" };
        SettingValue<VFS::Path::Normalized> mSkyclouds{ mIndex, "Models", "skyclouds" };
        SettingValue<VFS::Path::Normalized> mSkynight01{ mIndex, "Models", "skynight01" };
        SettingValue<VFS::Path::Normalized> mSkynight02{ mIndex, "Models", "skynight02" };
        SettingValue<VFS::Path::Normalized> mWeatherashcloud{ mIndex, "Models", "weatherashcloud" };
        SettingValue<VFS::Path::Normalized> mWeatherblightcloud{ mIndex, "Models", "weatherblightcloud" };
        SettingValue<VFS::Path::Normalized> mWeathersnow{ mIndex, "Models", "weathersnow" };
        SettingValue<VFS::Path::Normalized> mWeatherblizzard{ mIndex, "Models", "weatherblizzard" };
    };
}

#endif
