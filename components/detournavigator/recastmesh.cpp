#include "recastmesh.hpp"
#include "settings.hpp"

namespace DetourNavigator
{
    RecastMesh::RecastMesh(std::vector<int> indices, std::vector<float> vertices, const Settings& settings)
        : mIndices(std::move(indices))
        , mVertices(std::move(vertices))
        , mChunkyTriMesh(mVertices, mIndices, settings.mTrianglesPerChunk)
    {
    }
}
