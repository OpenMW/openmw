#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_STEREOVIEW_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_STEREOVIEW_H

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
    struct StereoViewCategory : WithIndex
    {
        using WithIndex::WithIndex;

        SettingValue<int> mEyeResolutionX{ mIndex, "Stereo View", "eye resolution x", makeMaxSanitizerInt(1) };
        SettingValue<int> mEyeResolutionY{ mIndex, "Stereo View", "eye resolution y", makeMaxSanitizerInt(1) };
        SettingValue<float> mLeftEyeOffsetX{ mIndex, "Stereo View", "left eye offset x" };
        SettingValue<float> mLeftEyeOffsetY{ mIndex, "Stereo View", "left eye offset y" };
        SettingValue<float> mLeftEyeOffsetZ{ mIndex, "Stereo View", "left eye offset z" };
        SettingValue<double> mLeftEyeOrientationX{ mIndex, "Stereo View", "left eye orientation x",
            makeClampSanitizerDouble(-1, 1) };
        SettingValue<double> mLeftEyeOrientationY{ mIndex, "Stereo View", "left eye orientation y",
            makeClampSanitizerDouble(-1, 1) };
        SettingValue<double> mLeftEyeOrientationZ{ mIndex, "Stereo View", "left eye orientation z",
            makeClampSanitizerDouble(-1, 1) };
        SettingValue<double> mLeftEyeOrientationW{ mIndex, "Stereo View", "left eye orientation w",
            makeClampSanitizerDouble(-1, 1) };
        SettingValue<float> mLeftEyeFovLeft{ mIndex, "Stereo View", "left eye fov left",
            makeClampSanitizerFloat(-osg::PIf, osg::PIf) };
        SettingValue<float> mLeftEyeFovRight{ mIndex, "Stereo View", "left eye fov right",
            makeClampSanitizerFloat(-osg::PIf, osg::PIf) };
        SettingValue<float> mLeftEyeFovUp{ mIndex, "Stereo View", "left eye fov up",
            makeClampSanitizerFloat(-osg::PIf, osg::PIf) };
        SettingValue<float> mLeftEyeFovDown{ mIndex, "Stereo View", "left eye fov down",
            makeClampSanitizerFloat(-osg::PIf, osg::PIf) };
        SettingValue<float> mRightEyeOffsetX{ mIndex, "Stereo View", "right eye offset x" };
        SettingValue<float> mRightEyeOffsetY{ mIndex, "Stereo View", "right eye offset y" };
        SettingValue<float> mRightEyeOffsetZ{ mIndex, "Stereo View", "right eye offset z" };
        SettingValue<double> mRightEyeOrientationX{ mIndex, "Stereo View", "right eye orientation x",
            makeClampSanitizerDouble(-1, 1) };
        SettingValue<double> mRightEyeOrientationY{ mIndex, "Stereo View", "right eye orientation y",
            makeClampSanitizerDouble(-1, 1) };
        SettingValue<double> mRightEyeOrientationZ{ mIndex, "Stereo View", "right eye orientation z",
            makeClampSanitizerDouble(-1, 1) };
        SettingValue<double> mRightEyeOrientationW{ mIndex, "Stereo View", "right eye orientation w",
            makeClampSanitizerDouble(-1, 1) };
        SettingValue<float> mRightEyeFovLeft{ mIndex, "Stereo View", "right eye fov left",
            makeClampSanitizerFloat(-osg::PIf, osg::PIf) };
        SettingValue<float> mRightEyeFovRight{ mIndex, "Stereo View", "right eye fov right",
            makeClampSanitizerFloat(-osg::PIf, osg::PIf) };
        SettingValue<float> mRightEyeFovUp{ mIndex, "Stereo View", "right eye fov up",
            makeClampSanitizerFloat(-osg::PIf, osg::PIf) };
        SettingValue<float> mRightEyeFovDown{ mIndex, "Stereo View", "right eye fov down",
            makeClampSanitizerFloat(-osg::PIf, osg::PIf) };
    };
}

#endif
