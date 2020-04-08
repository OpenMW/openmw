#ifndef MWINPUT_MWSENSORMANAGER_H
#define MWINPUT_MWSENSORMANAGER_H

#include <SDL_sensor.h>

#include <components/settings/settings.hpp>
#include <components/sdlutil/events.hpp>

namespace SDLUtil
{
    class InputWrapper;
}

namespace MWWorld
{
    class Player;
}

namespace MWInput
{
    class SensorManager : public SDLUtil::SensorListener
    {
    public:
        SensorManager();

        virtual ~SensorManager();

        void init();

        void clear();

        bool update(float dt, bool isCursorEnabled, bool isTurningEnabled);

    public:
        virtual void sensorUpdated(const SDL_SensorEvent &arg);
        virtual void displayOrientationChanged();
        void processChangedSettings(const Settings::CategorySettingVector& changed);

    private:
        enum GyroscopeAxis
        {
            Unknown = 0,
            X = 1,
            Y = 2,
            Z = 3,
            Minus_X = -1,
            Minus_Y = -2,
            Minus_Z = -3
        };

        bool mInvertX;
        bool mInvertY;

        float mGyroXSpeed;
        float mGyroYSpeed;
        float mGyroUpdateTimer;

        float mGyroHSensitivity;
        float mGyroVSensitivity;
        GyroscopeAxis mGyroHAxis;
        GyroscopeAxis mGyroVAxis;
        float mGyroInputThreshold;

    private:

        void updateSensors();
        void correctGyroscopeAxes();
        GyroscopeAxis mapGyroscopeAxis(const std::string& axis);
        float getGyroAxisSpeed(GyroscopeAxis axis, const SDL_SensorEvent &arg) const;
        SDL_Sensor* mGyroscope;
    };
}
#endif
