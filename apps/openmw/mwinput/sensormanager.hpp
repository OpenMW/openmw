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

        void update(float dt);

        void sensorUpdated(const SDL_SensorEvent &arg) override;
        void displayOrientationChanged() override;
        void processChangedSettings(const Settings::CategorySettingVector& changed);

        void setGuiCursorEnabled(bool enabled) { mGuiCursorEnabled = enabled; }

        bool isGyroAvailable() const;
        std::array<float, 3> getGyroValues() const;

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

        void updateSensors();
        void correctGyroscopeAxes();
        GyroscopeAxis mapGyroscopeAxis(const std::string& axis);
        float getGyroAxisSpeed(GyroscopeAxis axis) const;

        bool mInvertX;
        bool mInvertY;

        std::array<float, 3> mGyroValues;
        float mGyroUpdateTimer;

        float mGyroHSensitivity;
        float mGyroVSensitivity;
        GyroscopeAxis mGyroHAxis;
        GyroscopeAxis mGyroVAxis;
        float mGyroInputThreshold;

        SDL_Sensor* mGyroscope;

        bool mGuiCursorEnabled;
    };
}
#endif
