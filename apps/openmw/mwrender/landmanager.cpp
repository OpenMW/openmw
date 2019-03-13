#include "landmanager.hpp"

#include <osg/Stats>

#include <components/resource/objectcache.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/esmstore.hpp"

namespace MWRender
{

LandManager::LandManager(int loadFlags)
    : GenericResourceManager<std::pair<int, int> >(nullptr)
    , mLoadFlags(loadFlags)
{
    mCache = new CacheType;
}

osg::ref_ptr<ESMTerrain::LandObject> LandManager::getLand(int x, int y)
{
    osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(std::make_pair(x,y));
    if (obj)
        return static_cast<ESMTerrain::LandObject*>(obj.get());
    else
    {
        const ESM::Land* land = MWBase::Environment::get().getWorld()->getStore().get<ESM::Land>().search(x,y);
        if (!land)
            return nullptr;
        osg::ref_ptr<ESMTerrain::LandObject> landObj (new ESMTerrain::LandObject(land, mLoadFlags));
        mCache->addEntryToObjectCache(std::make_pair(x,y), landObj.get());
        return landObj;
    }
}

void LandManager::reportStats(unsigned int frameNumber, osg::Stats *stats) const
{
    stats->setAttribute(frameNumber, "Land", mCache->getCacheSize());
}


}
