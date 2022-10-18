#ifndef GAME_SOUND_WATERSOUNDUPDATER_H
#define GAME_SOUND_WATERSOUNDUPDATER_H

#include "components/esm/refid.hpp"
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
        ESM::RefId mNearWaterIndoorID;
        ESM::RefId mNearWaterOutdoorID;
    };

    struct WaterSoundUpdate
    {
        ESM::RefId mId;
        float mVolume;
    };

    class WaterSoundUpdater
    {
    public:
        explicit WaterSoundUpdater(const WaterSoundUpdaterSettings& settings);

        WaterSoundUpdate update(const MWWorld::ConstPtr& player, const MWBase::World& world) const;

        void setUnderwater(bool value) { mListenerUnderwater = value; }

    private:
        const WaterSoundUpdaterSettings mSettings;
        bool mListenerUnderwater = false;

        float getVolume(const MWWorld::ConstPtr& player, const MWBase::World& world) const;
    };
}

#endif
