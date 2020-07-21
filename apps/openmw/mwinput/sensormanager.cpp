#include "sensormanager.hpp"

#include <components/debug/debuglog.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"

namespace MWInput
{
    SensorManager::SensorManager()
        : mInvertX(Settings::Manager::getBool("invert x axis", "Input"))
        , mInvertY(Settings::Manager::getBool("invert y axis", "Input"))
        , mGyroXSpeed(0.f)
        , mGyroYSpeed(0.f)
        , mGyroUpdateTimer(0.f)
        , mGyroHSensitivity(Settings::Manager::getFloat("gyro horizontal sensitivity", "Input"))
        , mGyroVSensitivity(Settings::Manager::getFloat("gyro vertical sensitivity", "Input"))
        , mGyroHAxis(GyroscopeAxis::Minus_X)
        , mGyroVAxis(GyroscopeAxis::Y)
        , mGyroInputThreshold(Settings::Manager::getFloat("gyro input threshold", "Input"))
        , mGyroscope(nullptr)
        , mGuiCursorEnabled(true)
    {
        init();
    }

    void SensorManager::init()
    {
        correctGyroscopeAxes();
        updateSensors();
    }

    SensorManager::~SensorManager()
    {
        if (mGyroscope != nullptr)
        {
            SDL_SensorClose(mGyroscope);
            mGyroscope = nullptr;
        }
    }

    SensorManager::GyroscopeAxis SensorManager::mapGyroscopeAxis(const std::string& axis)
    {
        if (axis == "x")
            return GyroscopeAxis::X;
        else if (axis == "y")
            return GyroscopeAxis::Y;
        else if (axis == "z")
            return GyroscopeAxis::Z;
        else if (axis == "-x")
            return GyroscopeAxis::Minus_X;
        else if (axis == "-y")
            return GyroscopeAxis::Minus_Y;
        else if (axis == "-z")
            return GyroscopeAxis::Minus_Z;

        return GyroscopeAxis::Unknown;
    }

    void SensorManager::correctGyroscopeAxes()
    {
        if (!Settings::Manager::getBool("enable gyroscope", "Input"))
            return;

        // Treat setting from config as axes for landscape mode.
        // If the device does not support orientation change, do nothing.
        // Note: in is unclear how to correct axes for devices with non-standart Z axis direction.
        mGyroHAxis = mapGyroscopeAxis(Settings::Manager::getString("gyro horizontal axis", "Input"));
        mGyroVAxis = mapGyroscopeAxis(Settings::Manager::getString("gyro vertical axis", "Input"));

        SDL_DisplayOrientation currentOrientation = SDL_GetDisplayOrientation(Settings::Manager::getInt("screen", "Video"));
        switch (currentOrientation)
        {
            case SDL_ORIENTATION_UNKNOWN:
                return;
            case SDL_ORIENTATION_LANDSCAPE:
                break;
            case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
            {
                mGyroHAxis = GyroscopeAxis(-mGyroHAxis);
                mGyroVAxis = GyroscopeAxis(-mGyroVAxis);

                break;
            }
            case SDL_ORIENTATION_PORTRAIT:
            {
                GyroscopeAxis oldVAxis = mGyroVAxis;
                mGyroVAxis = mGyroHAxis;
                mGyroHAxis = GyroscopeAxis(-oldVAxis);

                break;
            }
            case SDL_ORIENTATION_PORTRAIT_FLIPPED:
            {
                GyroscopeAxis oldVAxis = mGyroVAxis;
                mGyroVAxis = GyroscopeAxis(-mGyroHAxis);
                mGyroHAxis = oldVAxis;

                break;
            }
        }
    }

    void SensorManager::updateSensors()
    {
        if (Settings::Manager::getBool("enable gyroscope", "Input"))
        {
            int numSensors = SDL_NumSensors();
            for (int i = 0; i < numSensors; ++i)
            {
                if (SDL_SensorGetDeviceType(i) == SDL_SENSOR_GYRO)
                {
                    // It is unclear how to handle several enabled gyroscopes, so use the first one.
                    // Note: Android registers some gyroscope as two separate sensors, for non-wake-up mode and for wake-up mode.
                    if (mGyroscope != nullptr)
                    {
                        SDL_SensorClose(mGyroscope);
                        mGyroscope = nullptr;
                        mGyroXSpeed = mGyroYSpeed = 0.f;
                        mGyroUpdateTimer = 0.f;
                    }

                    // FIXME: SDL2 does not provide a way to configure a sensor update frequency so far.
                    SDL_Sensor *sensor = SDL_SensorOpen(i);
                    if (sensor == nullptr)
                        Log(Debug::Error) << "Couldn't open sensor " << SDL_SensorGetDeviceName(i) << ": " << SDL_GetError();
                    else
                    {
                        mGyroscope = sensor;
                        break;
                    }
                }
            }
        }
        else
        {
            if (mGyroscope != nullptr)
            {
                SDL_SensorClose(mGyroscope);
                mGyroscope = nullptr;
                mGyroXSpeed = mGyroYSpeed = 0.f;
                mGyroUpdateTimer = 0.f;
            }
        }
    }

    void SensorManager::processChangedSettings(const Settings::CategorySettingVector& changed)
    {
        for (const auto& setting : changed)
        {
            if (setting.first == "Input" && setting.second == "invert x axis")
                mInvertX = Settings::Manager::getBool("invert x axis", "Input");

            if (setting.first == "Input" && setting.second == "invert y axis")
                mInvertY = Settings::Manager::getBool("invert y axis", "Input");

            if (setting.first == "Input" && setting.second == "gyro horizontal sensitivity")
                mGyroHSensitivity = Settings::Manager::getFloat("gyro horizontal sensitivity", "Input");

            if (setting.first == "Input" && setting.second == "gyro vertical sensitivity")
                mGyroVSensitivity = Settings::Manager::getFloat("gyro vertical sensitivity", "Input");

            if (setting.first == "Input" && setting.second == "enable gyroscope")
                init();

            if (setting.first == "Input" && setting.second == "gyro horizontal axis")
                correctGyroscopeAxes();

            if (setting.first == "Input" && setting.second == "gyro vertical axis")
                correctGyroscopeAxes();

            if (setting.first == "Input" && setting.second == "gyro input threshold")
                mGyroInputThreshold = Settings::Manager::getFloat("gyro input threshold", "Input");
        }
    }

    float SensorManager::getGyroAxisSpeed(GyroscopeAxis axis, const SDL_SensorEvent &arg) const
    {
        switch (axis)
        {
            case GyroscopeAxis::X:
            case GyroscopeAxis::Y:
            case GyroscopeAxis::Z:
                return std::abs(arg.data[0]) >= mGyroInputThreshold ? arg.data[axis-1] : 0.f;
            case GyroscopeAxis::Minus_X:
            case GyroscopeAxis::Minus_Y:
            case GyroscopeAxis::Minus_Z:
                return std::abs(arg.data[0]) >= mGyroInputThreshold ? -arg.data[std::abs(axis)-1] : 0.f;
            default:
                return 0.f;
        }
    }

    void SensorManager::displayOrientationChanged()
    {
        correctGyroscopeAxes();
    }

    void SensorManager::sensorUpdated(const SDL_SensorEvent &arg)
    {
        if (!Settings::Manager::getBool("enable gyroscope", "Input"))
            return;

        SDL_Sensor *sensor = SDL_SensorFromInstanceID(arg.which);
        if (!sensor)
        {
            Log(Debug::Info) << "Couldn't get sensor for sensor event";
            return;
        }

        switch (SDL_SensorGetType(sensor))
        {
            case SDL_SENSOR_ACCEL:
                break;
            case SDL_SENSOR_GYRO:
            {
                mGyroXSpeed = getGyroAxisSpeed(mGyroHAxis, arg);
                mGyroYSpeed = getGyroAxisSpeed(mGyroVAxis, arg);
                mGyroUpdateTimer = 0.f;

                break;
        }
        default:
            break;
        }
    }

    void SensorManager::update(float dt)
    {
        if (mGyroXSpeed == 0.f && mGyroYSpeed == 0.f)
            return;

        if (mGyroUpdateTimer > 0.5f)
        {
            // More than half of second passed since the last gyroscope update.
            // A device more likely was disconnected or switched to the sleep mode.
            // Reset current rotation speed and wait for update.
            mGyroXSpeed = 0.f;
            mGyroYSpeed = 0.f;
            mGyroUpdateTimer = 0.f;
            return;
        }

        mGyroUpdateTimer += dt;

        if (!mGuiCursorEnabled)
        {
            float rot[3];
            rot[0] = -mGyroYSpeed * dt * mGyroVSensitivity * 4 * (mInvertY ? -1 : 1);
            rot[1] = 0.0f;
            rot[2] = -mGyroXSpeed * dt * mGyroHSensitivity * 4 * (mInvertX ? -1 : 1);

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
}
