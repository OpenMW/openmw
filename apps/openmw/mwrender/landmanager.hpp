#ifndef OPENMW_MWRENDER_LANDMANAGER_H
#define OPENMW_MWRENDER_LANDMANAGER_H

#include <osg/Object>

#include <components/esm/exteriorcelllocation.hpp>
#include <components/esmterrain/storage.hpp>
#include <components/resource/resourcemanager.hpp>

namespace ESM
{
    struct Land;
}

namespace ESM4
{
    struct Land;
}

namespace MWRender
{

    class LandManager : public Resource::GenericResourceManager<ESM::ExteriorCellLocation>
    {
    public:
        LandManager(int loadFlags);

        /// @note Will return nullptr if not found.
        osg::ref_ptr<ESMTerrain::LandObject> getLand(ESM::ExteriorCellLocation cellIndex);

        // FIXME: returning a pointer is probably not compatible with the rest of the codebase
        const ESM4::Land *getLandRecord(ESM::ExteriorCellLocation cellIndex) const;

        void reportStats(unsigned int frameNumber, osg::Stats* stats) const override;

    private:
        int mLoadFlags;
    };

}

#endif
