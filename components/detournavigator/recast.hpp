#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECAST_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECAST_H

#include <Recast.h>
#include <RecastAlloc.h>

#include <cstddef>
#include <type_traits>

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

    void* permRecastAlloc(std::size_t size);

    template <class T>
    inline void permRecastAlloc(T*& values, std::size_t size)
    {
        static_assert(std::is_arithmetic_v<T>);
        values = new (permRecastAlloc(size * sizeof(T))) T[size];
    }

    void permRecastAlloc(rcPolyMesh& value);

    void permRecastAlloc(rcPolyMeshDetail& value);

    void freePolyMeshDetail(rcPolyMeshDetail& value) noexcept;

    void copyPolyMesh(const rcPolyMesh& src, rcPolyMesh& dst);

    void copyPolyMeshDetail(const rcPolyMeshDetail& src, rcPolyMeshDetail& dst);
}

#endif
