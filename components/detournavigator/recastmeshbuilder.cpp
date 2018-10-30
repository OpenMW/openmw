#include "recastmeshbuilder.hpp"
#include "chunkytrimesh.hpp"
#include "debug.hpp"
#include "settings.hpp"
#include "settingsutils.hpp"
#include "exceptions.hpp"

#include <components/bullethelpers/processtrianglecallback.hpp>

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletCollision/CollisionShapes/btConcaveShape.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <LinearMath/btTransform.h>

#include <algorithm>

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

    RecastMeshBuilder::RecastMeshBuilder(const Settings& settings, const TileBounds& bounds)
        : mSettings(settings)
        , mBounds(bounds)
    {
        mBounds.mMin /= mSettings.get().mRecastScaleFactor;
        mBounds.mMax /= mSettings.get().mRecastScaleFactor;
    }

    void RecastMeshBuilder::addObject(const btCollisionShape& shape, const btTransform& transform,
                                      const AreaType areaType)
    {
        if (shape.isCompound())
            return addObject(static_cast<const btCompoundShape&>(shape), transform, areaType);
        else if (shape.getShapeType() == TERRAIN_SHAPE_PROXYTYPE)
            return addObject(static_cast<const btHeightfieldTerrainShape&>(shape), transform, areaType);
        else if (shape.isConcave())
            return addObject(static_cast<const btConcaveShape&>(shape), transform, areaType);
        else if (shape.getShapeType() == BOX_SHAPE_PROXYTYPE)
            return addObject(static_cast<const btBoxShape&>(shape), transform, areaType);
        std::ostringstream message;
        message << "Unsupported shape type: " << BroadphaseNativeTypes(shape.getShapeType());
        throw InvalidArgument(message.str());
    }

    void RecastMeshBuilder::addObject(const btCompoundShape& shape, const btTransform& transform,
                                      const AreaType areaType)
    {
        for (int i = 0, num = shape.getNumChildShapes(); i < num; ++i)
            addObject(*shape.getChildShape(i), transform * shape.getChildTransform(i), areaType);
    }

    void RecastMeshBuilder::addObject(const btConcaveShape& shape, const btTransform& transform,
                                      const AreaType areaType)
    {
        return addObject(shape, transform, makeProcessTriangleCallback([&] (btVector3* triangle, int, int)
        {
            for (std::size_t i = 3; i > 0; --i)
                addTriangleVertex(transform(triangle[i - 1]));
            mAreaTypes.push_back(areaType);
        }));
    }

    void RecastMeshBuilder::addObject(const btHeightfieldTerrainShape& shape, const btTransform& transform,
                                      const AreaType areaType)
    {
        return addObject(shape, transform, makeProcessTriangleCallback([&] (btVector3* triangle, int, int)
        {
            for (std::size_t i = 0; i < 3; ++i)
                addTriangleVertex(transform(triangle[i]));
            mAreaTypes.push_back(areaType);
        }));
    }

    void RecastMeshBuilder::addObject(const btBoxShape& shape, const btTransform& transform, const AreaType areaType)
    {
        const auto indexOffset = static_cast<int>(mVertices.size() / 3);

        for (int vertex = 0, count = shape.getNumVertices(); vertex < count; ++vertex)
        {
            btVector3 position;
            shape.getVertex(vertex, position);
            addVertex(transform(position));
        }

        const std::array<int, 36> indices {{
            0, 2, 3,
            3, 1, 0,
            0, 4, 6,
            6, 2, 0,
            0, 1, 5,
            5, 4, 0,
            7, 5, 1,
            1, 3, 7,
            7, 3, 2,
            2, 6, 7,
            7, 6, 4,
            4, 5, 7,
        }};

        std::transform(indices.begin(), indices.end(), std::back_inserter(mIndices),
            [&] (int index) { return index + indexOffset; });

        std::generate_n(std::back_inserter(mAreaTypes), 12, [=] { return areaType; });
    }

    void RecastMeshBuilder::addWater(const int cellSize, const btTransform& transform)
    {
        mWater.push_back(RecastMesh::Water {cellSize, transform});
    }

    std::shared_ptr<RecastMesh> RecastMeshBuilder::create() const
    {
        return std::make_shared<RecastMesh>(mIndices, mVertices, mAreaTypes, mWater, mSettings.get().mTrianglesPerChunk);
    }

    void RecastMeshBuilder::reset()
    {
        mIndices.clear();
        mVertices.clear();
        mAreaTypes.clear();
        mWater.clear();
    }

    void RecastMeshBuilder::addObject(const btConcaveShape& shape, const btTransform& transform,
                                      btTriangleCallback&& callback)
    {
        btVector3 aabbMin;
        btVector3 aabbMax;

        shape.getAabb(btTransform::getIdentity(), aabbMin, aabbMax);

        aabbMin = transform(aabbMin);
        aabbMax = transform(aabbMax);

        aabbMin.setX(std::max(mBounds.mMin.x(), aabbMin.x()));
        aabbMin.setX(std::min(mBounds.mMax.x(), aabbMin.x()));
        aabbMin.setY(std::max(mBounds.mMin.y(), aabbMin.y()));
        aabbMin.setY(std::min(mBounds.mMax.y(), aabbMin.y()));

        aabbMax.setX(std::max(mBounds.mMin.x(), aabbMax.x()));
        aabbMax.setX(std::min(mBounds.mMax.x(), aabbMax.x()));
        aabbMax.setY(std::max(mBounds.mMin.y(), aabbMax.y()));
        aabbMax.setY(std::min(mBounds.mMax.y(), aabbMax.y()));

        const auto inversedTransform = transform.inverse();

        aabbMin = inversedTransform(aabbMin);
        aabbMax = inversedTransform(aabbMax);

        aabbMin.setX(std::min(aabbMin.x(), aabbMax.x()));
        aabbMin.setY(std::min(aabbMin.y(), aabbMax.y()));
        aabbMin.setZ(std::min(aabbMin.z(), aabbMax.z()));

        aabbMax.setX(std::max(aabbMin.x(), aabbMax.x()));
        aabbMax.setY(std::max(aabbMin.y(), aabbMax.y()));
        aabbMax.setZ(std::max(aabbMin.z(), aabbMax.z()));

        shape.processAllTriangles(&callback, aabbMin, aabbMax);
    }

    void RecastMeshBuilder::addTriangleVertex(const btVector3& worldPosition)
    {
        mIndices.push_back(static_cast<int>(mVertices.size() / 3));
        addVertex(worldPosition);
    }

    void RecastMeshBuilder::addVertex(const btVector3& worldPosition)
    {
        const auto navMeshPosition = toNavMeshCoordinates(mSettings, makeOsgVec3f(worldPosition));
        mVertices.push_back(navMeshPosition.x());
        mVertices.push_back(navMeshPosition.y());
        mVertices.push_back(navMeshPosition.z());
    }
}
