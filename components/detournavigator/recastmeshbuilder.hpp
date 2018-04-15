#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHBUILDER_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESHBUILDER_H

#include "recastmesh.hpp"
#include "tilebounds.hpp"

class btBoxShape;
class btCollisionShape;
class btCompoundShape;
class btConcaveShape;
class btHeightfieldTerrainShape;
class btTransform;
class btTriangleCallback;
class btVector3;

namespace DetourNavigator
{
    class RecastMeshBuilder
    {
    public:
        RecastMeshBuilder(const Settings& settings, const TileBounds& bounds);

        void addObject(const btCollisionShape& shape, const btTransform& transform);

        void addObject(const btCompoundShape& shape, const btTransform& transform);

        void addObject(const btConcaveShape& shape, const btTransform& transform);

        void addObject(const btHeightfieldTerrainShape& shape, const btTransform& transform);

        void addObject(const btBoxShape& shape, const btTransform& transform);

        std::shared_ptr<RecastMesh> create() const;

        void reset();

    private:
        std::reference_wrapper<const Settings> mSettings;
        TileBounds mBounds;
        std::vector<int> mIndices;
        std::vector<float> mVertices;

        void addObject(const btConcaveShape& shape, const btTransform& transform, btTriangleCallback&& callback);

        void addTriangleVertex(const btVector3& worldPosition);

        void addVertex(const btVector3& worldPosition);
    };
}

#endif
