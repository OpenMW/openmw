#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHPROVIDER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHPROVIDER_H

#include "tileposition.hpp"
#include "recastmesh.hpp"
#include "tilecachedrecastmeshmanager.hpp"
#include "version.hpp"

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
        {}

        std::shared_ptr<RecastMesh> getMesh(std::string_view worldspace, const TilePosition& tilePosition) const
        {
            return mImpl.get().getNewMesh(worldspace, tilePosition);
        }

    private:
        std::reference_wrapper<TileCachedRecastMeshManager> mImpl;
    };
}

#endif
