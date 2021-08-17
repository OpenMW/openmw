#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECAST_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECAST_H

#include <Recast.h>

#include <cstddef>

namespace DetourNavigator
{
    constexpr std::size_t getVertsLength(const rcPolyMesh& value) noexcept
    {
        return 3 * static_cast<std::size_t>(value.nverts);
    }

    constexpr std::size_t getPolysLength(const rcPolyMesh& value) noexcept
    {
        return 2 * static_cast<std::size_t>(value.maxpolys * value.nvp);
    }

    constexpr std::size_t getRegsLength(const rcPolyMesh& value) noexcept
    {
        return static_cast<std::size_t>(value.maxpolys);
    }

    constexpr std::size_t getFlagsLength(const rcPolyMesh& value) noexcept
    {
        return static_cast<std::size_t>(value.npolys);
    }

    constexpr std::size_t getAreasLength(const rcPolyMesh& value) noexcept
    {
        return static_cast<std::size_t>(value.maxpolys);
    }

    constexpr std::size_t getMeshesLength(const rcPolyMeshDetail& value) noexcept
    {
        return 4 * static_cast<std::size_t>(value.nmeshes);
    }

    constexpr std::size_t getVertsLength(const rcPolyMeshDetail& value) noexcept
    {
        return 3 * static_cast<std::size_t>(value.nverts);
    }

    constexpr std::size_t getTrisLength(const rcPolyMeshDetail& value) noexcept
    {
        return 4 * static_cast<std::size_t>(value.ntris);
    }
}

#endif
