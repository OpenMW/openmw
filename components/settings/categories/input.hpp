#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_INPUT_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_INPUT_H

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
    struct InputCategory : WithIndex
    {
        using WithIndex::WithIndex;

        SettingValue<bool> mGrabCursor{ mIndex, "Input", "grab cursor" };
        SettingValue<float> mCameraSensitivity{ mIndex, "Input", "camera sensitivity", makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mCameraYMultiplier{ mIndex, "Input", "camera y multiplier",
            makeMaxStrictSanitizerFloat(0) };
        SettingValue<bool> mInvertXAxis{ mIndex, "Input", "invert x axis" };
        SettingValue<bool> mInvertYAxis{ mIndex, "Input", "invert y axis" };
        SettingValue<bool> mEnableController{ mIndex, "Input", "enable controller" };
        SettingValue<float> mGamepadCursorSpeed{ mIndex, "Input", "gamepad cursor speed",
            makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mJoystickDeadZone{ mIndex, "Input", "joystick dead zone",
            makeClampSanitizerFloat(0, 0.5f) };
        SettingValue<bool> mEnableGyroscope{ mIndex, "Input", "enable gyroscope" };
        SettingValue<std::string> mGyroHorizontalAxis{ mIndex, "Input", "gyro horizontal axis",
            makeEnumSanitizerString({ "x", "y", "z", "-x", "-y", "-z" }) };
        SettingValue<std::string> mGyroVerticalAxis{ mIndex, "Input", "gyro vertical axis",
            makeEnumSanitizerString({ "x", "y", "z", "-x", "-y", "-z" }) };
        SettingValue<float> mGyroInputThreshold{ mIndex, "Input", "gyro input threshold", makeMaxSanitizerFloat(0) };
        SettingValue<float> mGyroHorizontalSensitivity{ mIndex, "Input", "gyro horizontal sensitivity",
            makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mGyroVerticalSensitivity{ mIndex, "Input", "gyro vertical sensitivity",
            makeMaxStrictSanitizerFloat(0) };
    };
}

#endif
