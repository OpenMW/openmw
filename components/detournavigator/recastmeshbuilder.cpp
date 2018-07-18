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
        const auto indexOffset = int(mVertices.size() / 3);

        for (int vertex = 0; vertex < shape.getNumVertices(); ++vertex)
        {
            btVector3 position;
            shape.getVertex(vertex, position);
            addVertex(transform(position));
        }

        for (int vertex = 0; vertex < 12; ++vertex)
            mAreaTypes.push_back(areaType);

        static const std::array<int, 36> indices {{
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
    }

    std::shared_ptr<RecastMesh> RecastMeshBuilder::create() const
    {
        return std::make_shared<RecastMesh>(mIndices, mVertices, mAreaTypes, mSettings);
    }

    void RecastMeshBuilder::reset()
    {
        mIndices.clear();
        mVertices.clear();
        mAreaTypes.clear();
    }

    void RecastMeshBuilder::addObject(const btConcaveShape& shape, const btTransform& transform,
                                      btTriangleCallback&& callback)
    {
        btVector3 aabbMin;
        btVector3 aabbMax;
        shape.getAabb(btTransform::getIdentity(), aabbMin, aabbMax);
        const btVector3 boundsMinMin(mBounds.mMin.x(), mBounds.mMin.y(), 0);
        const btVector3 boundsMinMax(mBounds.mMin.x(), mBounds.mMax.y(), 0);
        const btVector3 boundsMaxMin(mBounds.mMax.x(), mBounds.mMin.y(), 0);
        const btVector3 boundsMaxMax(mBounds.mMax.x(), mBounds.mMax.y(), 0);
        const auto inversedTransform = transform.inverse();
        const auto localBoundsMinMin = inversedTransform(boundsMinMin);
        const auto localBoundsMinMax = inversedTransform(boundsMinMax);
        const auto localBoundsMaxMin = inversedTransform(boundsMaxMin);
        const auto localBoundsMaxMax = inversedTransform(boundsMaxMax);
        aabbMin.setX(std::min({localBoundsMinMin.x(), localBoundsMinMax.x(),
                               localBoundsMaxMin.x(), localBoundsMaxMax.x()}));
        aabbMin.setY(std::min({localBoundsMinMin.y(), localBoundsMinMax.y(),
                               localBoundsMaxMin.y(), localBoundsMaxMax.y()}));
        aabbMax.setX(std::max({localBoundsMinMin.x(), localBoundsMinMax.x(),
                               localBoundsMaxMin.x(), localBoundsMaxMax.x()}));
        aabbMax.setY(std::max({localBoundsMinMin.y(), localBoundsMinMax.y(),
                               localBoundsMaxMin.y(), localBoundsMaxMax.y()}));
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
