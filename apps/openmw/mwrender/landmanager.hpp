#ifndef OPENMW_COMPONENTS_ESMTERRAIN_LANDMANAGER_H
#define OPENMW_COMPONENTS_ESMTERRAIN_LANDMANAGER_H

#include <osg/Object>

#include <components/resource/objectcache.hpp>
#include <components/resource/resourcemanager.hpp>
#include <components/esmterrain/storage.hpp>

namespace ESM
{
    struct Land;
}

namespace MWRender
{

    class LandManager : public Resource::GenericResourceManager<std::pair<int, int> >
    {
    public:
        LandManager(int loadFlags);

        /// @note Will return nullptr if not found.
        osg::ref_ptr<ESMTerrain::LandObject> getLand(int x, int y);

        virtual void reportStats(unsigned int frameNumber, osg::Stats* stats) const;

    private:
        int mLoadFlags;
    };

}

#endif
