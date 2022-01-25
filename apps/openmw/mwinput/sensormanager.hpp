#ifndef MWINPUT_MWSENSORMANAGER_H
#define MWINPUT_MWSENSORMANAGER_H

#include <SDL_sensor.h>

#include <osg/Matrixf>
#include <osg/Vec3f>

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

        bool isGyroAvailable() const;
        std::array<float, 3> getGyroValues() const;

    private:

        void updateSensors();
        void correctGyroscopeAxes();

        osg::Matrixf mRotation;
        osg::Vec3f mGyroValues;
        float mGyroUpdateTimer;

        SDL_Sensor* mGyroscope;
    };
}
#endif
