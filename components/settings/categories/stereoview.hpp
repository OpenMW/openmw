#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_STEREOVIEW_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_STEREOVIEW_H

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
    struct StereoViewCategory
    {
        SettingValue<int> mEyeResolutionX{ "Stereo View", "eye resolution x", makeMaxSanitizerInt(1) };
        SettingValue<int> mEyeResolutionY{ "Stereo View", "eye resolution y", makeMaxSanitizerInt(1) };
        SettingValue<double> mLeftEyeOffsetX{ "Stereo View", "left eye offset x" };
        SettingValue<double> mLeftEyeOffsetY{ "Stereo View", "left eye offset y" };
        SettingValue<double> mLeftEyeOffsetZ{ "Stereo View", "left eye offset z" };
        SettingValue<double> mLeftEyeOrientationX{ "Stereo View", "left eye orientation x",
            makeClampSanitizerDouble(-1, 1) };
        SettingValue<double> mLeftEyeOrientationY{ "Stereo View", "left eye orientation y",
            makeClampSanitizerDouble(-1, 1) };
        SettingValue<double> mLeftEyeOrientationZ{ "Stereo View", "left eye orientation z",
            makeClampSanitizerDouble(-1, 1) };
        SettingValue<double> mLeftEyeOrientationW{ "Stereo View", "left eye orientation w",
            makeClampSanitizerDouble(-1, 1) };
        SettingValue<double> mLeftEyeFovLeft{ "Stereo View", "left eye fov left",
            makeClampSanitizerDouble(-osg::PI, osg::PI) };
        SettingValue<double> mLeftEyeFovRight{ "Stereo View", "left eye fov right",
            makeClampSanitizerDouble(-osg::PI, osg::PI) };
        SettingValue<double> mLeftEyeFovUp{ "Stereo View", "left eye fov up",
            makeClampSanitizerDouble(-osg::PI, osg::PI) };
        SettingValue<double> mLeftEyeFovDown{ "Stereo View", "left eye fov down",
            makeClampSanitizerDouble(-osg::PI, osg::PI) };
        SettingValue<double> mRightEyeOffsetX{ "Stereo View", "right eye offset x" };
        SettingValue<double> mRightEyeOffsetY{ "Stereo View", "right eye offset y" };
        SettingValue<double> mRightEyeOffsetZ{ "Stereo View", "right eye offset z" };
        SettingValue<double> mRightEyeOrientationX{ "Stereo View", "right eye orientation x",
            makeClampSanitizerDouble(-1, 1) };
        SettingValue<double> mRightEyeOrientationY{ "Stereo View", "right eye orientation y",
            makeClampSanitizerDouble(-1, 1) };
        SettingValue<double> mRightEyeOrientationZ{ "Stereo View", "right eye orientation z",
            makeClampSanitizerDouble(-1, 1) };
        SettingValue<double> mRightEyeOrientationW{ "Stereo View", "right eye orientation w",
            makeClampSanitizerDouble(-1, 1) };
        SettingValue<double> mRightEyeFovLeft{ "Stereo View", "right eye fov left",
            makeClampSanitizerDouble(-osg::PI, osg::PI) };
        SettingValue<double> mRightEyeFovRight{ "Stereo View", "right eye fov right",
            makeClampSanitizerDouble(-osg::PI, osg::PI) };
        SettingValue<double> mRightEyeFovUp{ "Stereo View", "right eye fov up",
            makeClampSanitizerDouble(-osg::PI, osg::PI) };
        SettingValue<double> mRightEyeFovDown{ "Stereo View", "right eye fov down",
            makeClampSanitizerDouble(-osg::PI, osg::PI) };
    };
}

#endif
