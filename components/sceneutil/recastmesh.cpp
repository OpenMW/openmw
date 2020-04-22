#include "navmesh.hpp"
#include "detourdebugdraw.hpp"

#include <components/detournavigator/settings.hpp>
#include <components/detournavigator/recastmesh.hpp>

#include <RecastDebugDraw.h>

#include <osg/Group>

namespace
{
    std::vector<float> calculateNormals(const std::vector<float>& vertices, const std::vector<int>& indices)
    {
        std::vector<float> result(indices.size());
        for (std::size_t i = 0, n = indices.size(); i < n; i += 3)
        {
            const float* v0_ptr = &vertices[indices[i] * 3];
            const float* v1_ptr = &vertices[indices[i + 1] * 3];
            const float* v2_ptr = &vertices[indices[i + 2] * 3];
            const osg::Vec3f v0(v0_ptr[0], v0_ptr[1], v0_ptr[2]);
            const osg::Vec3f v1(v1_ptr[0], v1_ptr[1], v1_ptr[2]);
            const osg::Vec3f v2(v2_ptr[0], v2_ptr[1], v2_ptr[2]);
            const osg::Vec3f e0 = v1 - v0;
            const osg::Vec3f e1 = v2 - v0;
            osg::Vec3f normal = e0 ^ e1;
            normal.normalize();
            for (std::size_t j = 0; j < 3; ++j)
                result[i + j] = normal[j];
        }
        return result;
    }
}

namespace SceneUtil
{
    osg::ref_ptr<osg::Group> createRecastMeshGroup(const DetourNavigator::RecastMesh& recastMesh,
        const DetourNavigator::Settings& settings)
    {
        const osg::ref_ptr<osg::Group> group(new osg::Group);
        DebugDraw debugDraw(*group, osg::Vec3f(0, 0, 0), 1.0f / settings.mRecastScaleFactor);
        const auto normals = calculateNormals(recastMesh.getVertices(), recastMesh.getIndices());
        const auto texScale = 1.0f / (settings.mCellSize * 10.0f);
        duDebugDrawTriMesh(&debugDraw, recastMesh.getVertices().data(), recastMesh.getVerticesCount(),
            recastMesh.getIndices().data(), normals.data(), recastMesh.getTrianglesCount(), nullptr, texScale);
        return group;
    }
}
