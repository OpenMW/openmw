#include "recastmeshbuilder.hpp"
#include "chunkytrimesh.hpp"
#include "settings.hpp"
#include "settingsutils.hpp"

#include <components/bullethelpers/processtrianglecallback.hpp>

#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <BulletCollision/CollisionShapes/btConcaveShape.h>

namespace
{
    osg::Vec3f makeOsgVec3f(const btVector3& value)
    {
        return osg::Vec3f(value.x(), value.y(), value.z());
    }
}

namespace DetourNavigator
{
    using BulletHelpers::makeProcessTriangleCallback;

    RecastMeshBuilder::RecastMeshBuilder(const Settings& settings)
        : mSettings(settings)
    {
    }

    void RecastMeshBuilder::addObject(const btConcaveShape& shape, const btTransform& transform)
    {
        return addObject(shape, makeProcessTriangleCallback([&] (btVector3* triangle, int, int)
        {
            for (std::size_t i = 3; i > 0; --i)
                addVertex(transform(triangle[i - 1]));
        }));
    }

    void RecastMeshBuilder::addObject(const btHeightfieldTerrainShape& shape, const btTransform& transform)
    {
        return addObject(shape, makeProcessTriangleCallback([&] (btVector3* triangle, int, int)
        {
            for (std::size_t i = 0; i < 3; ++i)
                addVertex(transform(triangle[i]));
        }));
    }

    std::shared_ptr<RecastMesh> RecastMeshBuilder::create() const
    {
        return std::make_shared<RecastMesh>(mIndices, mVertices, mSettings);
    }

    void RecastMeshBuilder::reset()
    {
        mIndices.clear();
        mVertices.clear();
    }

    void RecastMeshBuilder::addObject(const btConcaveShape& shape, btTriangleCallback&& callback)
    {
        btVector3 aabbMin;
        btVector3 aabbMax;
        shape.getAabb(btTransform::getIdentity(), aabbMin, aabbMax);
        shape.processAllTriangles(&callback, aabbMin, aabbMax);
    }

    void RecastMeshBuilder::addVertex(const btVector3& worldPosition)
    {
        const auto navMeshPosition = toNavMeshCoordinates(mSettings, makeOsgVec3f(worldPosition));
        mIndices.push_back(static_cast<int>(mIndices.size()));
        mVertices.push_back(navMeshPosition.x());
        mVertices.push_back(navMeshPosition.y());
        mVertices.push_back(navMeshPosition.z());
    }
}
