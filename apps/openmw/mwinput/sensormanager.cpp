#include "sensormanager.hpp"

#include <components/debug/debuglog.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"

namespace MWInput
{
    SensorManager::SensorManager()
        : mGyroValues()
        , mGyroUpdateTimer(0.f)
        , mGyroHAxis(GyroscopeAxis::Minus_X)
        , mGyroVAxis(GyroscopeAxis::Y)
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

    void SensorManager::correctGyroscopeAxes()
    {
        if (!Settings::Manager::getBool("enable gyroscope", "Input"))
            return;

        // Treat setting from config as axes for landscape mode.
        // If the device does not support orientation change, do nothing.
        // Note: in is unclear how to correct axes for devices with non-standart Z axis direction.
        mGyroHAxis = gyroscopeAxisFromString(Settings::Manager::getString("gyro horizontal axis", "Input"));
        mGyroVAxis = gyroscopeAxisFromString(Settings::Manager::getString("gyro vertical axis", "Input"));

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
                mGyroUpdateTimer = 0.f;
            }
        }
    }

    void SensorManager::processChangedSettings(const Settings::CategorySettingVector& changed)
    {
        for (const auto& setting : changed)
        {
            if (setting.first == "Input" && setting.second == "enable gyroscope")
                init();

            if (setting.first == "Input" && setting.second == "gyro horizontal axis")
                correctGyroscopeAxes();

            if (setting.first == "Input" && setting.second == "gyro vertical axis")
                correctGyroscopeAxes();
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
                mGyroValues[0] = arg.data[0];
                mGyroValues[1] = arg.data[1];
                mGyroValues[2] = arg.data[2];
                mGyroUpdateTimer = 0.f;

                break;
        }
        default:
            break;
        }
    }

    void SensorManager::update(float dt)
    {
        mGyroUpdateTimer += dt;
        if (mGyroUpdateTimer > 0.5f)
        {
            // More than half of second passed since the last gyroscope update.
            // A device more likely was disconnected or switched to the sleep mode.
            // Reset current rotation speed and wait for update.
            mGyroValues = { 0, 0, 0 };
            mGyroUpdateTimer = 0.f;
        }
    }

    bool SensorManager::isGyroAvailable() const
    {
        return mGyroscope != nullptr;
    }

    std::array<float, 3> SensorManager::getGyroValues() const
    {
        return mGyroValues;
    }
}
