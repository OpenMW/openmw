#include "landmanager.hpp"

#include <osg/Stats>

#include <components/resource/objectcache.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/esmstore.hpp"

namespace MWRender
{

    LandManager::LandManager(int loadFlags)
        : GenericResourceManager<ESM::ExteriorCellLocation>(nullptr)
        , mLoadFlags(loadFlags)
    {
    }

    osg::ref_ptr<ESMTerrain::LandObject> LandManager::getLand(ESM::ExteriorCellLocation cellIndex)
    {
        osg::ref_ptr<osg::Object> obj = mCache->getRefFromObjectCache(cellIndex);
        if (obj)
            return static_cast<ESMTerrain::LandObject*>(obj.get());
        else
        {
            const auto world = MWBase::Environment::get().getWorld();
            if (!world)
                return nullptr;

            if (ESM::isEsm4Ext(cellIndex.mWorldspace))
            {
                const ESM4::Land* land = world->getStore().get<ESM4::Land>().search(cellIndex);
                if (!land)
                    return nullptr;
                osg::ref_ptr<ESMTerrain::LandObject> landObj(new ESMTerrain::LandObject(land, mLoadFlags));
                mCache->addEntryToObjectCache(cellIndex, landObj.get());
                return landObj;
            }
            else
            {
                const ESM::Land* land = world->getStore().get<ESM::Land>().search(cellIndex.mX, cellIndex.mY);
                if (!land)
                    return nullptr;
                osg::ref_ptr<ESMTerrain::LandObject> landObj(new ESMTerrain::LandObject(land, mLoadFlags));
                mCache->addEntryToObjectCache(cellIndex, landObj.get());
                return landObj;
            }
        }
    }

    void LandManager::reportStats(unsigned int frameNumber, osg::Stats* stats) const
    {
        stats->setAttribute(frameNumber, "Land", mCache->getCacheSize());
    }

}
