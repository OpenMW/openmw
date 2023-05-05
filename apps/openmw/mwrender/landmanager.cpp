#include "landmanager.hpp"

#include <osg/Stats>

#include <components/resource/objectcache.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/esmstore.hpp"

namespace MWRender
{

    LandManager::LandManager(int loadFlags)
        : GenericResourceManager<std::pair<int, int>>(nullptr)
        , mLoadFlags(loadFlags)
    {
        mCache = new CacheType;
    }

    osg::ref_ptr<ESMTerrain::LandObject> LandManager::getLand(ESM::ExteriorCellIndex cellIndex)
    {
        if (cellIndex.mWorldspace != ESM::Cell::sDefaultWorldspaceId)
            return osg::ref_ptr<ESMTerrain::LandObject>(nullptr);
        int x = cellIndex.mX;
        int y = cellIndex.mY;
        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(std::make_pair(x, y));
        if (obj)
            return static_cast<ESMTerrain::LandObject*>(obj.get());
        else
        {
            const auto world = MWBase::Environment::get().getWorld();
            if (!world)
                return nullptr;
            const ESM::Land* land = world->getStore().get<ESM::Land>().search(x, y);
            if (!land)
                return nullptr;
            osg::ref_ptr<ESMTerrain::LandObject> landObj(new ESMTerrain::LandObject(land, mLoadFlags));
            mCache->addEntryToObjectCache(std::make_pair(x, y), landObj.get());
            return landObj;
        }
    }

    void LandManager::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        stats->setAttribute(frameNumber, "Land", mCache->getCacheSize());
    }

}
