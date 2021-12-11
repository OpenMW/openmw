#include "recastmesh.hpp"
#include "exceptions.hpp"

#include <Recast.h>

namespace DetourNavigator
{
    Mesh::Mesh(std::vector<int>&& indices, std::vector<float>&& vertices, std::vector<AreaType>&& areaTypes)
    {
        if (indices.size() / 3 != areaTypes.size())
           throw InvalidArgument("Number of flags doesn't match number of triangles: triangles="
               + std::to_string(indices.size() / 3) + ", areaTypes=" + std::to_string(areaTypes.size()));
        indices.shrink_to_fit();
        vertices.shrink_to_fit();
        areaTypes.shrink_to_fit();
        mIndices = std::move(indices);
        mVertices = std::move(vertices);
        mAreaTypes = std::move(areaTypes);
    }

    RecastMesh::RecastMesh(std::size_t generation, std::size_t revision, Mesh mesh, std::vector<CellWater> water,
        std::vector<Heightfield> heightfields, std::vector<FlatHeightfield> flatHeightfields,
        std::vector<MeshSource> meshSources)
        : mGeneration(generation)
        , mRevision(revision)
        , mMesh(std::move(mesh))
        , mWater(std::move(water))
        , mHeightfields(std::move(heightfields))
        , mFlatHeightfields(std::move(flatHeightfields))
        , mMeshSources(std::move(meshSources))
    {
        mWater.shrink_to_fit();
        mHeightfields.shrink_to_fit();
        for (Heightfield& v : mHeightfields)
            v.mHeights.shrink_to_fit();
    }
}
