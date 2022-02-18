#include "generatenavmeshtile.hpp"

#include "dbrefgeometryobject.hpp"
#include "makenavmesh.hpp"
#include "offmeshconnectionsmanager.hpp"
#include "preparednavmeshdata.hpp"
#include "serialization.hpp"
#include "settings.hpp"
#include "tilecachedrecastmeshmanager.hpp"

#include <components/debug/debuglog.hpp>

#include <osg/Vec3f>
#include <osg/io_utils>

#include <memory>
#include <stdexcept>
#include <vector>
#include <optional>
#include <functional>

namespace DetourNavigator
{
    namespace
    {
        struct Ignore
        {
            std::string_view mWorldspace;
            const TilePosition& mTilePosition;
            std::shared_ptr<NavMeshTileConsumer> mConsumer;

            ~Ignore() noexcept
            {
                if (mConsumer != nullptr)
                    mConsumer->ignore(mWorldspace, mTilePosition);
            }
        };
    }

    GenerateNavMeshTile::GenerateNavMeshTile(std::string worldspace, const TilePosition& tilePosition,
            RecastMeshProvider recastMeshProvider, const osg::Vec3f& agentHalfExtents,
            const DetourNavigator::Settings& settings, std::weak_ptr<NavMeshTileConsumer> consumer)
        : mWorldspace(std::move(worldspace))
        , mTilePosition(tilePosition)
        , mRecastMeshProvider(recastMeshProvider)
        , mAgentHalfExtents(agentHalfExtents)
        , mSettings(settings)
        , mConsumer(std::move(consumer)) {}

    void GenerateNavMeshTile::doWork()
    {
        impl();
    }

    void GenerateNavMeshTile::impl() noexcept
    {
        const auto consumer = mConsumer.lock();

        if (consumer == nullptr)
            return;

        try
        {
            Ignore ignore {mWorldspace, mTilePosition, consumer};

            const std::shared_ptr<RecastMesh> recastMesh = mRecastMeshProvider.getMesh(mWorldspace, mTilePosition);

            if (recastMesh == nullptr || isEmpty(*recastMesh))
                return;

            const std::vector<DbRefGeometryObject> objects = makeDbRefGeometryObjects(recastMesh->getMeshSources(),
                [&] (const MeshSource& v) { return consumer->resolveMeshSource(v); });
            std::vector<std::byte> input = serialize(mSettings.mRecast, *recastMesh, objects);
            const std::optional<NavMeshTileInfo> info = consumer->find(mWorldspace, mTilePosition, input);

            if (info.has_value() && info->mVersion == mSettings.mNavMeshVersion)
            {
                consumer->identity(mWorldspace, mTilePosition, info->mTileId);
                ignore.mConsumer = nullptr;
                return;
            }

            const auto data = prepareNavMeshTileData(*recastMesh, mTilePosition, mAgentHalfExtents, mSettings.mRecast);

            if (data == nullptr)
                return;

            if (info.has_value())
                consumer->update(mWorldspace, mTilePosition, info->mTileId, mSettings.mNavMeshVersion, *data);
            else
                consumer->insert(mWorldspace, mTilePosition, mSettings.mNavMeshVersion, input, *data);

            ignore.mConsumer = nullptr;
        }
        catch (const std::exception& e)
        {
            Log(Debug::Warning) << "Failed to generate navmesh for worldspace \"" << mWorldspace
                                << "\" tile " << mTilePosition << ": " << e.what();
        }
    }
}
