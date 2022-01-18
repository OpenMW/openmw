#include "gyromanager.hpp"

#include "../mwbase/inputmanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/player.hpp"

namespace MWInput
{
    GyroManager::GyroManager()
        : mEnabled(Settings::Manager::getBool("enable gyroscope", "Input"))
        , mGuiCursorEnabled(true)
        , mSensitivityH(Settings::Manager::getFloat("gyro horizontal sensitivity", "Input"))
        , mSensitivityV(Settings::Manager::getFloat("gyro vertical sensitivity", "Input"))
        , mInvertH(Settings::Manager::getBool("invert x axis", "Input"))
        , mInvertV(Settings::Manager::getBool("invert y axis", "Input"))
        , mInputThreshold(Settings::Manager::getFloat("gyro input threshold", "Input"))
        , mAxisH(gyroscopeAxisFromString(Settings::Manager::getString("gyro horizontal axis", "Input")))
        , mAxisV(gyroscopeAxisFromString(Settings::Manager::getString("gyro vertical axis", "Input")))
    {};

    void GyroManager::update(float dt, std::array<float, 3> values) const
    {
        if (!mGuiCursorEnabled)
        {
            float gyroH = getAxisValue(mAxisH, values);
            float gyroV = getAxisValue(mAxisV, values);

            if (gyroH == 0 && gyroV == 0)
                return;

            float rot[3];
            rot[0] = -gyroV * dt * mSensitivityV * 4 * (mInvertV ? -1 : 1);
            rot[1] = 0.0f;
            rot[2] = -gyroH * dt * mSensitivityH * 4 * (mInvertH ? -1 : 1);

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
            else if (setting.second == "invert x axis")
                mInvertH = Settings::Manager::getBool("invert x axis", "Input");
            else if (setting.second == "invert y axis")
                mInvertV = Settings::Manager::getBool("invert y axis", "Input");
            else if (setting.second == "gyro input threshold")
                mInputThreshold = Settings::Manager::getFloat("gyro input threshold", "Input");
            else if (setting.second == "gyro horizontal axis")
                mAxisH = gyroscopeAxisFromString(Settings::Manager::getString("gyro horizontal axis", "Input"));
            else if (setting.second == "gyro vertical axis")
                mAxisV = gyroscopeAxisFromString(Settings::Manager::getString("gyro vertical axis", "Input"));
        }
    }

    namespace
    {
        int signum(int x)
        {
            return 0 < x - x < 0;
        }
    }

    float GyroManager::getAxisValue(GyroscopeAxis axis, std::array<float, 3> values) const
    {
        if (axis == GyroscopeAxis::Unknown)
            return 0;
        float value = values[std::abs(axis) - 1] * signum(axis);
        //if (std::abs(value) <= mInputThreshold)
        //    value = 0;
        return value;
    }
}
