#include "gyromanager.hpp"

#include "../mwbase/inputmanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/player.hpp"

namespace MWInput
{
    GyroManager::GyroscopeAxis GyroManager::gyroscopeAxisFromString(std::string_view s)
    {
        if (s == "x")
            return GyroscopeAxis::X;
        else if (s == "y")
            return GyroscopeAxis::Y;
        else if (s == "z")
            return GyroscopeAxis::Z;
        else if (s == "-x")
            return GyroscopeAxis::Minus_X;
        else if (s == "-y")
            return GyroscopeAxis::Minus_Y;
        else if (s == "-z")
            return GyroscopeAxis::Minus_Z;

        return GyroscopeAxis::Unknown;
    }

    GyroManager::GyroManager()
        : mEnabled(Settings::Manager::getBool("enable gyroscope", "Input"))
        , mGuiCursorEnabled(true)
        , mSensitivityH(Settings::Manager::getFloat("gyro horizontal sensitivity", "Input"))
        , mSensitivityV(Settings::Manager::getFloat("gyro vertical sensitivity", "Input"))
        , mInputThreshold(Settings::Manager::getFloat("gyro input threshold", "Input"))
        , mAxisH(gyroscopeAxisFromString(Settings::Manager::getString("gyro horizontal axis", "Input")))
        , mAxisV(gyroscopeAxisFromString(Settings::Manager::getString("gyro vertical axis", "Input")))
    {}

    void GyroManager::update(float dt, std::array<float, 3> values) const
    {
        if (!mGuiCursorEnabled)
        {
            float gyroH = getAxisValue(mAxisH, values);
            float gyroV = getAxisValue(mAxisV, values);

            if (gyroH == 0.f && gyroV == 0.f)
                return;

            float rot[3];
            rot[0] = -gyroV * dt * mSensitivityV;
            rot[1] = 0.0f;
            rot[2] = -gyroH * dt * mSensitivityH;

            // Only actually turn player when we're not in vanity mode
            bool playerLooking = MWBase::Environment::get().getInputManager()->getControlSwitch("playerlooking");
            if (!MWBase::Environment::get().getWorld()->vanityRotateCamera(rot) && playerLooking)
            {
                MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
                player.yaw(-rot[2]);
                player.pitch(-rot[0]);
            }
            else if (!playerLooking)
                MWBase::Environment::get().getWorld()->disableDeferredPreviewRotation();

            MWBase::Environment::get().getInputManager()->resetIdleTime();
        }
    }

    void GyroManager::processChangedSettings(const Settings::CategorySettingVector& changed)
    {
        for (const auto& setting : changed)
        {
            if (setting.first != "Input")
                continue;

            if (setting.second == "enable gyroscope")
                mEnabled = Settings::Manager::getBool("enable gyroscope", "Input");
            else if (setting.second == "gyro horizontal sensitivity")
                mSensitivityH = Settings::Manager::getFloat("gyro horizontal sensitivity", "Input");
            else if (setting.second == "gyro vertical sensitivity")
                mSensitivityV = Settings::Manager::getFloat("gyro vertical sensitivity", "Input");
            else if (setting.second == "gyro input threshold")
                mInputThreshold = Settings::Manager::getFloat("gyro input threshold", "Input");
            else if (setting.second == "gyro horizontal axis")
                mAxisH = gyroscopeAxisFromString(Settings::Manager::getString("gyro horizontal axis", "Input"));
            else if (setting.second == "gyro vertical axis")
                mAxisV = gyroscopeAxisFromString(Settings::Manager::getString("gyro vertical axis", "Input"));
        }
    }

    float GyroManager::getAxisValue(GyroscopeAxis axis, std::array<float, 3> values) const
    {
        if (axis == GyroscopeAxis::Unknown)
            return 0;
        float value = values[std::abs(axis) - 1];
        if (axis < 0)
            value *= -1;
        if (std::abs(value) <= mInputThreshold)
            value = 0;
        return value;
    }
}
