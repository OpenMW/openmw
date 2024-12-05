#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_STEREO_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_STEREO_H

#include <components/settings/settingvalue.hpp>

#include <osg/Math>
#include <osg/Vec2f>
#include <osg/Vec3f>

#include <cstdint>
#include <string>
#include <string_view>

namespace Settings
{
    struct StereoCategory : WithIndex
    {
        using WithIndex::WithIndex;

        SettingValue<bool> mStereoEnabled{ mIndex, "Stereo", "stereo enabled" };
        SettingValue<bool> mMultiview{ mIndex, "Stereo", "multiview" };
        SettingValue<bool> mSharedShadowMaps{ mIndex, "Stereo", "shared shadow maps" };
        SettingValue<bool> mAllowDisplayListsForMultiview{ mIndex, "Stereo", "allow display lists for multiview" };
        SettingValue<bool> mUseCustomView{ mIndex, "Stereo", "use custom view" };
        SettingValue<bool> mUseCustomEyeResolution{ mIndex, "Stereo", "use custom eye resolution" };
    };
}

#endif
