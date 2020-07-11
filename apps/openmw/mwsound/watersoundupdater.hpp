#ifndef GAME_SOUND_WATERSOUNDUPDATER_H
#define GAME_SOUND_WATERSOUNDUPDATER_H

#include <string>

namespace MWBase
{
    class World;
}

namespace MWWorld
{
    class ConstPtr;
}

namespace MWSound
{
    struct WaterSoundUpdaterSettings
    {
        int mNearWaterRadius;
        int mNearWaterPoints;
        float mNearWaterIndoorTolerance;
        float mNearWaterOutdoorTolerance;
        std::string mNearWaterIndoorID;
        std::string mNearWaterOutdoorID;
    };

    struct WaterSoundUpdate
    {
        std::string mId;
        float mVolume;
    };

    class WaterSoundUpdater
    {
        public:
            explicit WaterSoundUpdater(const WaterSoundUpdaterSettings& settings);

            WaterSoundUpdate update(const MWWorld::ConstPtr& player, const MWBase::World& world) const;

            void setUnderwater(bool value)
            {
                mListenerUnderwater = value;
            }

        private:
            const WaterSoundUpdaterSettings mSettings;
            bool mListenerUnderwater = false;

            float getVolume(const MWWorld::ConstPtr& player, const MWBase::World& world) const;
    };
}

#endif
