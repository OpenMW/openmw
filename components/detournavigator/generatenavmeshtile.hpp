#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_GENERATENAVMESHTILE_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_GENERATENAVMESHTILE_H

#include "recastmeshprovider.hpp"
#include "tileposition.hpp"

#include <components/sceneutil/workqueue.hpp>

#include <osg/Vec3f>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace DetourNavigator
{
    class OffMeshConnectionsManager;
    class RecastMesh;
    struct NavMeshTileConsumer;
    struct OffMeshConnection;
    struct PreparedNavMeshData;
    struct Settings;

    struct NavMeshTileInfo
    {
        std::int64_t mTileId;
        std::int64_t mVersion;
    };

    struct NavMeshTileConsumer
    {
        virtual ~NavMeshTileConsumer() = default;

        virtual std::int64_t resolveMeshSource(const MeshSource& source) = 0;

        virtual std::optional<NavMeshTileInfo> find(std::string_view worldspace, const TilePosition& tilePosition,
            const std::vector<std::byte>& input) = 0;

        virtual void ignore(std::string_view worldspace, const TilePosition& tilePosition) = 0;

        virtual void identity(std::string_view worldspace, const TilePosition& tilePosition,
                              std::int64_t tileId) = 0;

        virtual void insert(std::string_view worldspace, const TilePosition& tilePosition,
                            std::int64_t version, const std::vector<std::byte>& input, PreparedNavMeshData& data) = 0;

        virtual void update(std::string_view worldspace, const TilePosition& tilePosition,
                            std::int64_t tileId, std::int64_t version, PreparedNavMeshData& data) = 0;

        virtual void cancel() = 0;
    };

    class GenerateNavMeshTile final : public SceneUtil::WorkItem
    {
    public:
        GenerateNavMeshTile(std::string worldspace, const TilePosition& tilePosition,
                RecastMeshProvider recastMeshProvider, const osg::Vec3f& agentHalfExtents, const Settings& settings,
                std::weak_ptr<NavMeshTileConsumer> consumer);

        void doWork() final;

    private:
        const std::string mWorldspace;
        const TilePosition mTilePosition;
        const RecastMeshProvider mRecastMeshProvider;
        const osg::Vec3f mAgentHalfExtents;
        const Settings& mSettings;
        std::weak_ptr<NavMeshTileConsumer> mConsumer;

        inline void impl() noexcept;
    };
}

#endif
