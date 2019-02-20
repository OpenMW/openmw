#include "terrainstorage.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwworld/esmstore.hpp"

#include "landmanager.hpp"

namespace MWRender
{

    TerrainStorage::TerrainStorage(Resource::ResourceSystem* resourceSystem, const std::string& normalMapPattern, const std::string& normalHeightMapPattern, bool autoUseNormalMaps, const std::string& specularMapPattern, bool autoUseSpecularMaps)
        : ESMTerrain::Storage(resourceSystem->getVFS(), normalMapPattern, normalHeightMapPattern, autoUseNormalMaps, specularMapPattern, autoUseSpecularMaps)
        , mLandManager(new LandManager(ESM::Land::DATA_VCLR|ESM::Land::DATA_VHGT|ESM::Land::DATA_VNML|ESM::Land::DATA_VTEX))
        , mResourceSystem(resourceSystem)
    {
        mResourceSystem->addResourceManager(mLandManager.get());
    }

    TerrainStorage::~TerrainStorage()
    {
        mResourceSystem->removeResourceManager(mLandManager.get());
    }

    bool TerrainStorage::hasData(int cellX, int cellY)
    {
        const MWWorld::ESMStore &esmStore =
             MWBase::Environment::get().getWorld()->getStore();

        const ESM::Land* land = esmStore.get<ESM::Land>().search(cellX, cellY);
        return land != nullptr;
    }

    void TerrainStorage::getBounds(float& minX, float& maxX, float& minY, float& maxY)
    {
        minX = 0, minY = 0, maxX = 0, maxY = 0;

        const MWWorld::ESMStore &esmStore =
            MWBase::Environment::get().getWorld()->getStore();

        MWWorld::Store<ESM::Land>::iterator it = esmStore.get<ESM::Land>().begin();
        for (; it != esmStore.get<ESM::Land>().end(); ++it)
        {
            if (it->mX < minX)
                minX = static_cast<float>(it->mX);
            if (it->mX > maxX)
                maxX = static_cast<float>(it->mX);
            if (it->mY < minY)
                minY = static_cast<float>(it->mY);
            if (it->mY > maxY)
                maxY = static_cast<float>(it->mY);
        }

        // since grid coords are at cell origin, we need to add 1 cell
        maxX += 1;
        maxY += 1;
    }

    LandManager *TerrainStorage::getLandManager() const
    {
        return mLandManager.get();
    }

    osg::ref_ptr<const ESMTerrain::LandObject> TerrainStorage::getLand(int cellX, int cellY)
    {
        return mLandManager->getLand(cellX, cellY);
    }

    const ESM::LandTexture* TerrainStorage::getLandTexture(int index, short plugin)
    {
        const MWWorld::ESMStore &esmStore =
            MWBase::Environment::get().getWorld()->getStore();
        return esmStore.get<ESM::LandTexture>().search(index, plugin);
    }


}
