#include "watersoundupdater.hpp"

#include "../mwbase/world.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/ptr.hpp"

#include <components/esm/loadcell.hpp>

#include <osg/Vec3f>

namespace MWSound
{
    WaterSoundUpdater::WaterSoundUpdater(const WaterSoundUpdaterSettings& settings)
        : mSettings(settings)
    {
    }

    WaterSoundUpdate WaterSoundUpdater::update(const MWWorld::ConstPtr& player, const MWBase::World& world) const
    {
        WaterSoundUpdate result;

        result.mId = player.getCell()->isExterior() ? mSettings.mNearWaterOutdoorID : mSettings.mNearWaterIndoorID;
        result.mVolume = std::min(1.0f, getVolume(player, world));

        return result;
    }

    float WaterSoundUpdater::getVolume(const MWWorld::ConstPtr& player, const MWBase::World& world) const
    {
        if (mListenerUnderwater)
            return 1.0f;

        const MWWorld::CellStore& cell = *player.getCell();

        if (!cell.getCell()->hasWater())
            return 0.0f;

        const osg::Vec3f pos = player.getRefData().getPosition().asVec3();
        const float dist = std::abs(cell.getWaterLevel() - pos.z());

        if (cell.isExterior() && dist < mSettings.mNearWaterOutdoorTolerance)
        {
            if (mSettings.mNearWaterPoints <= 1)
                return (mSettings.mNearWaterOutdoorTolerance - dist) / mSettings.mNearWaterOutdoorTolerance;

            const float step = mSettings.mNearWaterRadius * 2.0f / (mSettings.mNearWaterPoints - 1);

            int underwaterPoints = 0;

            for (int x = 0; x < mSettings.mNearWaterPoints; x++)
            {
                for (int y = 0; y < mSettings.mNearWaterPoints; y++)
                {
                    const float terrainX = pos.x() - mSettings.mNearWaterRadius + x * step;
                    const float terrainY = pos.y() - mSettings.mNearWaterRadius + y * step;
                    const float height = world.getTerrainHeightAt(osg::Vec3f(terrainX, terrainY, 0.0f));

                    if (height < 0)
                        underwaterPoints++;
                }
            }

            return underwaterPoints * 2.0f / (mSettings.mNearWaterPoints * mSettings.mNearWaterPoints);
        }

        if (!cell.isExterior() && dist < mSettings.mNearWaterIndoorTolerance)
            return (mSettings.mNearWaterIndoorTolerance - dist) / mSettings.mNearWaterIndoorTolerance;

        return 0.0f;
    }
}
