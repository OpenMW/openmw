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

    void copyPolyMesh(const rcPolyMesh& src, rcPolyMesh& dst)
    {
        dst.nverts = src.nverts;
        dst.npolys = src.npolys;
        dst.maxpolys = src.maxpolys;
        dst.nvp = src.nvp;
        rcVcopy(dst.bmin, src.bmin);
        rcVcopy(dst.bmax, src.bmax);
        dst.cs = src.cs;
        dst.ch = src.ch;
        dst.borderSize = src.borderSize;
        dst.maxEdgeError = src.maxEdgeError;
        permRecastAlloc(dst);
        std::memcpy(dst.verts, src.verts, getVertsLength(src) * sizeof(*dst.verts));
        std::memcpy(dst.polys, src.polys, getPolysLength(src) * sizeof(*dst.polys));
        std::memcpy(dst.regs, src.regs, getRegsLength(src) * sizeof(*dst.regs));
        std::memcpy(dst.flags, src.flags, getFlagsLength(src) * sizeof(*dst.flags));
        std::memcpy(dst.areas, src.areas, getAreasLength(src) * sizeof(*dst.areas));
    }

    void copyPolyMeshDetail(const rcPolyMeshDetail& src, rcPolyMeshDetail& dst)
    {
        dst.nmeshes = src.nmeshes;
        dst.nverts = src.nverts;
        dst.ntris = src.ntris;
        permRecastAlloc(dst);
        std::memcpy(dst.meshes, src.meshes, getMeshesLength(src) * sizeof(*dst.meshes));
        std::memcpy(dst.verts, src.verts, getVertsLength(src) * sizeof(*dst.verts));
        std::memcpy(dst.tris, src.tris, getTrisLength(src) * sizeof(*dst.tris));
    }
}
