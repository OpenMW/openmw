#include "terrainstorage.hpp"

#include <boost/algorithm/string.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwworld/esmstore.hpp"

namespace MWRender
{

    void TerrainStorage::getBounds(float& minX, float& maxX, float& minY, float& maxY)
    {
        minX = 0, minY = 0, maxX = 0, maxY = 0;

        const MWWorld::ESMStore &esmStore =
            MWBase::Environment::get().getWorld()->getStore();

        MWWorld::Store<ESM::Cell>::iterator it = esmStore.get<ESM::Cell>().extBegin();
        for (; it != esmStore.get<ESM::Cell>().extEnd(); ++it)
        {
            if (it->getGridX() < minX)
                minX = it->getGridX();
            if (it->getGridX() > maxX)
                maxX = it->getGridX();
            if (it->getGridY() < minY)
                minY = it->getGridY();
            if (it->getGridY() > maxY)
                maxY = it->getGridY();
        }

        // since grid coords are at cell origin, we need to add 1 cell
        maxX += 1;
        maxY += 1;
    }

    ESM::Land* TerrainStorage::getLand(int cellX, int cellY)
    {
        const MWWorld::ESMStore &esmStore =
            MWBase::Environment::get().getWorld()->getStore();
        ESM::Land* land = esmStore.get<ESM::Land>().search(cellX, cellY);
        return land;
    }

    const ESM::LandTexture* TerrainStorage::getLandTexture(int index, short plugin)
    {
        const MWWorld::ESMStore &esmStore =
            MWBase::Environment::get().getWorld()->getStore();
        return esmStore.get<ESM::LandTexture>().find(index, plugin);
    }

}
