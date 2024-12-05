#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHPROVIDER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHPROVIDER_H

#include "recastmesh.hpp"
#include "tilecachedrecastmeshmanager.hpp"
#include "tileposition.hpp"

#include <functional>
#include <memory>

namespace DetourNavigator
{
    class RecastMesh;

    class RecastMeshProvider
    {
    public:
        RecastMeshProvider(TileCachedRecastMeshManager& impl)
            : mImpl(impl)
        {
        }

        std::shared_ptr<RecastMesh> getMesh(ESM::RefId worldspace, const TilePosition& tilePosition) const
        {
            return mImpl.get().getNewMesh(worldspace, tilePosition);
        }

    private:
        std::reference_wrapper<TileCachedRecastMeshManager> mImpl;
    };
}

#endif
