#include "terrainstorage.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwworld/esmstore.hpp"

namespace MWRender
{

    Ogre::AxisAlignedBox TerrainStorage::getBounds()
    {
        int minX = 0, minY = 0, maxX = 0, maxY = 0;

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

        return Ogre::AxisAlignedBox(minX, minY, 0, maxX, maxY, 0);
    }

    ESM::Land* TerrainStorage::getLand(int cellX, int cellY)
    {
        const MWWorld::ESMStore &esmStore =
            MWBase::Environment::get().getWorld()->getStore();
        ESM::Land* land = esmStore.get<ESM::Land>().search(cellX, cellY);
        // Load the data we are definitely going to need
        int mask = ESM::Land::DATA_VHGT | ESM::Land::DATA_VNML | ESM::Land::DATA_VCLR | ESM::Land::DATA_VTEX;
        if (land && !land->isDataLoaded(mask))
            land->loadData(mask);
        return land;
    }

    const ESM::LandTexture* TerrainStorage::getLandTexture(int index, short plugin)
    {
        const MWWorld::ESMStore &esmStore =
            MWBase::Environment::get().getWorld()->getStore();
        return esmStore.get<ESM::LandTexture>().find(index, plugin);
    }

}
