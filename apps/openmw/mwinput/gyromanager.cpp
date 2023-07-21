#include "gyromanager.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/player.hpp"

#include <components/settings/values.hpp>

namespace MWInput
{
    namespace
    {
        float getAxisValue(Settings::GyroscopeAxis axis, float threshold, std::array<float, 3> values)
        {
            const float value = [&] {
                switch (axis)
                {
                    case Settings::GyroscopeAxis::X:
                        return values[0];
                    case Settings::GyroscopeAxis::Y:
                        return values[1];
                    case Settings::GyroscopeAxis::Z:
                        return values[2];
                    case Settings::GyroscopeAxis::MinusX:
                        return -values[0];
                    case Settings::GyroscopeAxis::MinusY:
                        return -values[1];
                    case Settings::GyroscopeAxis::MinusZ:
                        return -values[2];
                };
                return 0.0f;
            }();
            if (std::abs(value) <= threshold)
                return 0;
            return value;
        }
    }

    void GyroManager::update(float dt, std::array<float, 3> values) const
    {
        if (mGuiCursorEnabled)
            return;

        const float threshold = Settings::input().mGyroInputThreshold;
        const float gyroH = getAxisValue(Settings::input().mGyroHorizontalAxis, threshold, values);
        const float gyroV = getAxisValue(Settings::input().mGyroVerticalAxis, threshold, values);

        if (gyroH == 0.f && gyroV == 0.f)
            return;

        const float rot[3] = {
            -gyroV * dt * Settings::input().mGyroVerticalSensitivity,
            0.0f,
            -gyroH * dt * Settings::input().mGyroHorizontalSensitivity,
        };

        // Only actually turn player when we're not in vanity mode
        const bool playerLooking = MWBase::Environment::get().getInputManager()->getControlSwitch("playerlooking");
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
