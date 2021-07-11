#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHBUILDER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHBUILDER_H

#include "recastmesh.hpp"
#include "tilebounds.hpp"

#include <osg/Vec3f>

#include <LinearMath/btTransform.h>

#include <array>
#include <functional>
#include <memory>
#include <tuple>
#include <vector>

class btBoxShape;
class btCollisionShape;
class btCompoundShape;
class btConcaveShape;
class btHeightfieldTerrainShape;
class btTriangleCallback;

namespace DetourNavigator
{
    struct Settings;

    struct RecastMeshTriangle
    {
        AreaType mAreaType;
        std::array<osg::Vec3f, 3> mVertices;

        friend inline bool operator<(const RecastMeshTriangle& lhs, const RecastMeshTriangle& rhs)
        {
            return std::tie(lhs.mAreaType, lhs.mVertices) < std::tie(rhs.mAreaType, rhs.mVertices);
        }
    };

    class RecastMeshBuilder
    {
    public:
        RecastMeshBuilder(const Settings& settings, const TileBounds& bounds);

        void addObject(const btCollisionShape& shape, const btTransform& transform, const AreaType areaType);

        void addObject(const btCompoundShape& shape, const btTransform& transform, const AreaType areaType);

        void addObject(const btConcaveShape& shape, const btTransform& transform, const AreaType areaType);

        void addObject(const btHeightfieldTerrainShape& shape, const btTransform& transform, const AreaType areaType);

        void addObject(const btBoxShape& shape, const btTransform& transform, const AreaType areaType);

        void addWater(const int mCellSize, const btTransform& transform);

        std::shared_ptr<RecastMesh> create(std::size_t generation, std::size_t revision) &&;

    private:
        std::reference_wrapper<const Settings> mSettings;
        TileBounds mBounds;
        std::vector<RecastMeshTriangle> mTriangles;
        std::vector<RecastMesh::Water> mWater;

        void addObject(const btConcaveShape& shape, const btTransform& transform, btTriangleCallback&& callback);

        void addObject(const btHeightfieldTerrainShape& shape, const btTransform& transform, btTriangleCallback&& callback);
    };

    Mesh makeMesh(std::vector<RecastMeshTriangle>&& triangles);
}

#endif
