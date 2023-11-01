#include "sensormanager.hpp"

#include <components/debug/debuglog.hpp>
#include <components/settings/values.hpp>

namespace MWInput
{
    SensorManager::SensorManager()
        : mRotation()
        , mGyroValues()
        , mGyroUpdateTimer(0.f)
        , mGyroscope(nullptr)
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
        if (!Settings::input().mEnableGyroscope)
            return;

        // Treat setting from config as axes for landscape mode.
        // If the device does not support orientation change, do nothing.
        // Note: in is unclear how to correct axes for devices with non-standart Z axis direction.

        mRotation = osg::Matrixf::identity();

        float angle = 0;

        SDL_DisplayOrientation currentOrientation = SDL_GetDisplayOrientation(Settings::video().mScreen);
        switch (currentOrientation)
        {
            case SDL_ORIENTATION_UNKNOWN:
                break;
            case SDL_ORIENTATION_LANDSCAPE:
                break;
            case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
            {
                angle = osg::PIf;
                break;
            }
            case SDL_ORIENTATION_PORTRAIT:
            {
                angle = -0.5 * osg::PIf;
                break;
            }
            case SDL_ORIENTATION_PORTRAIT_FLIPPED:
            {
                angle = 0.5 * osg::PIf;
                break;
            }
        }

        mRotation.makeRotate(angle, osg::Vec3f(0, 0, 1));
    }

    void SensorManager::updateSensors()
    {
        if (Settings::input().mEnableGyroscope)
        {
            int numSensors = SDL_NumSensors();
            for (int i = 0; i < numSensors; ++i)
            {
                if (SDL_SensorGetDeviceType(i) == SDL_SENSOR_GYRO)
                {
                    // It is unclear how to handle several enabled gyroscopes, so use the first one.
                    // Note: Android registers some gyroscope as two separate sensors, for non-wake-up mode and for
                    // wake-up mode.
                    if (mGyroscope != nullptr)
                    {
                        SDL_SensorClose(mGyroscope);
                        mGyroscope = nullptr;
                        mGyroUpdateTimer = 0.f;
                    }

                    // FIXME: SDL2 does not provide a way to configure a sensor update frequency so far.
                    SDL_Sensor* sensor = SDL_SensorOpen(i);
                    if (sensor == nullptr)
                        Log(Debug::Error)
                            << "Couldn't open sensor " << SDL_SensorGetDeviceName(i) << ": " << SDL_GetError();
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
        }
    }

    void SensorManager::displayOrientationChanged()
    {
        correctGyroscopeAxes();
    }

    void SensorManager::sensorUpdated(const SDL_SensorEvent& arg)
    {
        if (!Settings::input().mEnableGyroscope)
            return;

        SDL_Sensor* sensor = SDL_SensorFromInstanceID(arg.which);
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
                osg::Vec3f gyro(arg.data[0], arg.data[1], arg.data[2]);
                mGyroValues = mRotation * gyro;
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
            mGyroValues = osg::Vec3f();
            mGyroUpdateTimer = 0.f;
        }
    }

    bool SensorManager::isGyroAvailable() const
    {
        return mGyroscope != nullptr;
    }

    std::array<float, 3> SensorManager::getGyroValues() const
    {
        return { mGyroValues.x(), mGyroValues.y(), mGyroValues.z() };
    }
}
