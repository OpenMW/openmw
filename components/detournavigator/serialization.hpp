#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_SERIALIZATION_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_SERIALIZATION_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace DetourNavigator
{
    class RecastMesh;
    struct DbRefGeometryObject;
    struct PreparedNavMeshData;
    struct RecastSettings;

    constexpr char recastMeshMagic[] = {'r', 'c', 's', 't'};
    constexpr std::uint32_t recastMeshVersion = 1;

    constexpr char preparedNavMeshDataMagic[] = {'p', 'n', 'a', 'v'};
    constexpr std::uint32_t preparedNavMeshDataVersion = 1;

    std::vector<std::byte> serialize(const RecastSettings& settings, const RecastMesh& value,
        const std::vector<DbRefGeometryObject>& dbRefGeometryObjects);

    std::vector<std::byte> serialize(const PreparedNavMeshData& value);

    bool deserialize(const std::vector<std::byte>& data, PreparedNavMeshData& value);
}

#endif
