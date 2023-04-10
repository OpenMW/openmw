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
    struct InputCategory
    {
        SettingValue<bool> mGrabCursor{ "Input", "grab cursor" };
        SettingValue<float> mCameraSensitivity{ "Input", "camera sensitivity", makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mCameraYMultiplier{ "Input", "camera y multiplier", makeMaxStrictSanitizerFloat(0) };
        SettingValue<bool> mInvertXAxis{ "Input", "invert x axis" };
        SettingValue<bool> mInvertYAxis{ "Input", "invert y axis" };
        SettingValue<bool> mEnableController{ "Input", "enable controller" };
        SettingValue<float> mGamepadCursorSpeed{ "Input", "gamepad cursor speed", makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mJoystickDeadZone{ "Input", "joystick dead zone", makeClampSanitizerFloat(0, 0.5f) };
        SettingValue<bool> mEnableGyroscope{ "Input", "enable gyroscope" };
        SettingValue<std::string> mGyroHorizontalAxis{ "Input", "gyro horizontal axis",
            makeEnumSanitizerString({ "x", "y", "z", "-x", "-y", "-z" }) };
        SettingValue<std::string> mGyroVerticalAxis{ "Input", "gyro vertical axis",
            makeEnumSanitizerString({ "x", "y", "z", "-x", "-y", "-z" }) };
        SettingValue<float> mGyroInputThreshold{ "Input", "gyro input threshold", makeMaxSanitizerFloat(0) };
        SettingValue<float> mGyroHorizontalSensitivity{ "Input", "gyro horizontal sensitivity",
            makeMaxStrictSanitizerFloat(0) };
        SettingValue<float> mGyroVerticalSensitivity{ "Input", "gyro vertical sensitivity",
            makeMaxStrictSanitizerFloat(0) };
    };
}

#endif
