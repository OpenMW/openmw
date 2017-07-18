#include "landmanager.hpp"

#include <osg/Stats>

#include <sstream>

#include <components/resource/objectcache.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/esmstore.hpp"

namespace MWRender
{

LandManager::LandManager(int loadFlags)
    : ResourceManager(NULL)
    , mLoadFlags(loadFlags)
{
}

osg::ref_ptr<ESMTerrain::LandObject> LandManager::getLand(int x, int y)
{
    std::ostringstream id;
    id << x << " " << y;
    std::string idstr = id.str();

    osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(idstr);
    if (obj)
        return static_cast<ESMTerrain::LandObject*>(obj.get());
    else
    {
        const ESM::Land* land = MWBase::Environment::get().getWorld()->getStore().get<ESM::Land>().search(x,y);
        if (!land)
            return NULL;
        osg::ref_ptr<ESMTerrain::LandObject> landObj (new ESMTerrain::LandObject(land, mLoadFlags));
        mCache->addEntryToObjectCache(idstr, landObj.get());
        return landObj;
    }
}

void LandManager::reportStats(unsigned int frameNumber, osg::Stats *stats) const
{
    stats->setAttribute(frameNumber, "Land", mCache->getCacheSize());
}


}
