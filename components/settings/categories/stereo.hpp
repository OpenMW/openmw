#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_STEREO_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_STEREO_H

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
    struct StereoCategory
    {
        SettingValue<bool> mStereoEnabled{ "Stereo", "stereo enabled" };
        SettingValue<bool> mMultiview{ "Stereo", "multiview" };
        SettingValue<bool> mSharedShadowMaps{ "Stereo", "shared shadow maps" };
        SettingValue<bool> mAllowDisplayListsForMultiview{ "Stereo", "allow display lists for multiview" };
        SettingValue<bool> mUseCustomView{ "Stereo", "use custom view" };
        SettingValue<bool> mUseCustomEyeResolution{ "Stereo", "use custom eye resolution" };
    };
}

#endif
