#ifndef MWINPUT_GYROMANAGER
#define MWINPUT_GYROMANAGER

#include <components/settings/settings.hpp>

namespace MWInput
{
    class GyroManager
    {
        public:
            GyroManager();

            bool isEnabled() const { return mEnabled; }

            void update(float dt, std::array<float, 3> values) const;

            void processChangedSettings(const Settings::CategorySettingVector& changed);

            void setGuiCursorEnabled(bool enabled) { mGuiCursorEnabled = enabled; }

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

            static GyroscopeAxis gyroscopeAxisFromString(std::string_view s);

            bool mEnabled;
            bool mGuiCursorEnabled;
            float mSensitivityH;
            float mSensitivityV;
            float mInputThreshold;
            GyroscopeAxis mAxisH;
            GyroscopeAxis mAxisV;

            float getAxisValue(GyroscopeAxis axis, std::array<float, 3> values) const;
    };
}

#endif // !MWINPUT_GYROMANAGER
