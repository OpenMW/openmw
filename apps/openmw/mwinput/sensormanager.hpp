#ifndef MWINPUT_MWSENSORMANAGER_H
#define MWINPUT_MWSENSORMANAGER_H

#include <SDL_sensor.h>

#include <components/settings/settings.hpp>
#include <components/sdlutil/events.hpp>

#include "gyroaxis.hpp"

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

        void updateSensors();
        void correctGyroscopeAxes();

        std::array<float, 3> mGyroValues;
        float mGyroUpdateTimer;

        GyroscopeAxis mGyroHAxis;
        GyroscopeAxis mGyroVAxis;

        SDL_Sensor* mGyroscope;

        bool mGuiCursorEnabled;
    };
}
#endif
