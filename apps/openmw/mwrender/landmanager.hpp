#ifndef OPENMW_COMPONENTS_ESMTERRAIN_LANDMANAGER_H
#define OPENMW_COMPONENTS_ESMTERRAIN_LANDMANAGER_H

#include <osg/Object>

#include <components/resource/resourcemanager.hpp>
#include <components/esmterrain/storage.hpp>

namespace ESM
{
    struct Land;
}

namespace MWRender
{

    class LandManager : public Resource::ResourceManager
    {
    public:
        LandManager(int loadFlags);

        /// @note Will return NULL if not found.
        osg::ref_ptr<ESMTerrain::LandObject> getLand(int x, int y);

        virtual void reportStats(unsigned int frameNumber, osg::Stats* stats) const;

    private:
        int mLoadFlags;
    };

}

#endif
