#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_CAMERA_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_CAMERA_H

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
    struct CameraCategory
    {
        SettingValue<float> mNearClip{ "Camera", "near clip", makeMaxSanitizerFloat(0.005f) };
        SettingValue<bool> mSmallFeatureCulling{ "Camera", "small feature culling" };
        SettingValue<float> mSmallFeatureCullingPixelSize{ "Camera", "small feature culling pixel size",
            makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mViewingDistance{ "Camera", "viewing distance", makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mFieldOfView{ "Camera", "field of view", makeClampSanitizerFloat(1, 179) };
        SettingValue<float> mFirstPersonFieldOfView{ "Camera", "first person field of view",
            makeClampSanitizerFloat(1, 179) };
        SettingValue<bool> mReverseZ{ "Camera", "reverse z" };
    };
}

#endif
