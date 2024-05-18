#include "recastmesh.hpp"

#include "detourdebugdraw.hpp"

#include <components/detournavigator/recastmesh.hpp>
#include <components/detournavigator/recastmeshbuilder.hpp>
#include <components/detournavigator/settings.hpp>

#include <RecastDebugDraw.h>

#include <osg/Group>
#include <osg/Material>
#include <osg/PolygonOffset>

#include <algorithm>
#include <vector>

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
        const DetourNavigator::RecastSettings& settings, const osg::ref_ptr<osg::StateSet>& debugDrawStateSet)
    {
        using namespace DetourNavigator;

        const osg::ref_ptr<osg::Group> group(new osg::Group);

        DebugDraw debugDraw(*group, debugDrawStateSet, osg::Vec3f(0, 0, 0), 1.0f);
        const DetourNavigator::Mesh& mesh = recastMesh.getMesh();
        std::vector<int> indices = mesh.getIndices();
        std::vector<float> vertices = mesh.getVertices();

        for (const Heightfield& heightfield : recastMesh.getHeightfields())
        {
            const Mesh heightfieldMesh = makeMesh(heightfield);
            const int indexShift = static_cast<int>(vertices.size() / 3);
            std::copy(heightfieldMesh.getVertices().begin(), heightfieldMesh.getVertices().end(),
                std::back_inserter(vertices));
            std::transform(heightfieldMesh.getIndices().begin(), heightfieldMesh.getIndices().end(),
                std::back_inserter(indices), [&](int index) { return index + indexShift; });
        }

        for (std::size_t i = 0; i < vertices.size(); i += 3)
            std::swap(vertices[i + 1], vertices[i + 2]);

        const auto normals = calculateNormals(vertices, indices);
        const auto texScale = 1.0f / (settings.mCellSize * 10.0f);
        duDebugDrawTriMeshSlope(&debugDraw, vertices.data(), static_cast<int>(vertices.size() / 3), indices.data(),
            normals.data(), static_cast<int>(indices.size() / 3), settings.mMaxSlope, texScale);

        return group;
    }
}
