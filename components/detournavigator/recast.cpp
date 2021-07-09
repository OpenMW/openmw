#include "recast.hpp"

#include <Recast.h>
#include <RecastAlloc.h>

#include <cstring>
#include <new>

namespace DetourNavigator
{
    void* permRecastAlloc(std::size_t size)
    {
        void* const result = rcAlloc(size, RC_ALLOC_PERM);
        if (result == nullptr)
            throw std::bad_alloc();
        return result;
    }

    void permRecastAlloc(rcPolyMesh& value)
    {
        permRecastAlloc(value.verts, getVertsLength(value));
        permRecastAlloc(value.polys, getPolysLength(value));
        permRecastAlloc(value.regs, getRegsLength(value));
        permRecastAlloc(value.flags, getFlagsLength(value));
        permRecastAlloc(value.areas, getAreasLength(value));
    }

    void permRecastAlloc(rcPolyMeshDetail& value)
    {
        try
        {
            permRecastAlloc(value.meshes, getMeshesLength(value));
            permRecastAlloc(value.verts, getVertsLength(value));
            permRecastAlloc(value.tris, getTrisLength(value));
        }
        catch (...)
        {
            freePolyMeshDetail(value);
            throw;
        }
    }

    void freePolyMeshDetail(rcPolyMeshDetail& value) noexcept
    {
        rcFree(value.meshes);
        rcFree(value.verts);
        rcFree(value.tris);
    }
}
