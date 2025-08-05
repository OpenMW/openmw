#include "generatenavmeshtile.hpp"

#include "dbrefgeometryobject.hpp"
#include "makenavmesh.hpp"
#include "preparednavmeshdata.hpp"
#include "serialization.hpp"
#include "settings.hpp"

#include <components/debug/debuglog.hpp>

#include <osg/io_utils>

#include <memory>
#include <optional>
#include <vector>

namespace DetourNavigator
{
    namespace
    {
        struct Ignore
        {
            ESM::RefId mWorldspace;
            const TilePosition& mTilePosition;
            std::shared_ptr<NavMeshTileConsumer> mConsumer;

            ~Ignore() noexcept
            {
                if (mConsumer != nullptr)
                    mConsumer->ignore(mWorldspace, mTilePosition);
            }
        };
    }

    GenerateNavMeshTile::GenerateNavMeshTile(ESM::RefId worldspace, const TilePosition& tilePosition,
        RecastMeshProvider recastMeshProvider, const AgentBounds& agentBounds,
        const DetourNavigator::Settings& settings, std::weak_ptr<NavMeshTileConsumer> consumer)
        : mWorldspace(worldspace)
        , mTilePosition(tilePosition)
        , mRecastMeshProvider(recastMeshProvider)
        , mAgentBounds(agentBounds)
        , mSettings(settings)
        , mConsumer(std::move(consumer))
    {
    }

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
            Ignore ignore{ mWorldspace, mTilePosition, consumer };

            const std::shared_ptr<RecastMesh> recastMesh = mRecastMeshProvider.getMesh(mWorldspace, mTilePosition);

            if (recastMesh == nullptr || isEmpty(*recastMesh))
                return;

            const std::vector<DbRefGeometryObject> objects = makeDbRefGeometryObjects(
                recastMesh->getMeshSources(), [&](const MeshSource& v) { return consumer->resolveMeshSource(v); });
            std::vector<std::byte> input = serialize(mSettings.mRecast, mAgentBounds, *recastMesh, objects);
            const std::optional<NavMeshTileInfo> info = consumer->find(mWorldspace, mTilePosition, input);

            if (info.has_value() && info->mVersion == navMeshFormatVersion)
            {
                consumer->identity(mWorldspace, mTilePosition, info->mTileId);
                ignore.mConsumer = nullptr;
                return;
            }

            const std::unique_ptr<PreparedNavMeshData> data
                = prepareNavMeshTileData(*recastMesh, mWorldspace, mTilePosition, mAgentBounds, mSettings.mRecast);

            if (data == nullptr)
                return;

            if (info.has_value())
                consumer->update(mWorldspace, mTilePosition, info->mTileId, navMeshFormatVersion, *data);
            else
                consumer->insert(mWorldspace, mTilePosition, navMeshFormatVersion, input, *data);

            ignore.mConsumer = nullptr;
        }
        catch (const std::exception& e)
        {
            Log(Debug::Warning) << "Failed to generate navmesh for worldspace \"" << mWorldspace << "\" tile "
                                << mTilePosition << ": " << e.what();
            consumer->cancel(e.what());
        }
    }
}
