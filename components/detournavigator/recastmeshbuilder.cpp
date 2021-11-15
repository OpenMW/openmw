#include "recastmeshbuilder.hpp"
#include "debug.hpp"
#include "exceptions.hpp"

#include <components/bullethelpers/transformboundingbox.hpp>
#include <components/bullethelpers/processtrianglecallback.hpp>
#include <components/misc/convert.hpp>
#include <components/debug/debuglog.hpp>

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletCollision/CollisionShapes/btConcaveShape.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <LinearMath/btTransform.h>
#include <LinearMath/btAabbUtil2.h>

#include <algorithm>
#include <cassert>
#include <array>
#include <vector>
#include <sstream>

namespace DetourNavigator
{
    using BulletHelpers::makeProcessTriangleCallback;

    namespace
    {
        RecastMeshTriangle makeRecastMeshTriangle(const btVector3* vertices, const AreaType areaType)
        {
            RecastMeshTriangle result;
            result.mAreaType = areaType;
            for (std::size_t i = 0; i < 3; ++i)
                result.mVertices[i] = Misc::Convert::toOsg(vertices[i]);
            return result;
        }

        TileBounds maxCellTileBounds(int size, const osg::Vec3f& shift)
        {
            const float halfCellSize = static_cast<float>(size) / 2;
            return TileBounds {
                osg::Vec2f(shift.x() - halfCellSize, shift.y() - halfCellSize),
                osg::Vec2f(shift.x() + halfCellSize, shift.y() + halfCellSize)
            };
        }
    }

    Mesh makeMesh(std::vector<RecastMeshTriangle>&& triangles, const osg::Vec3f& shift)
    {
        std::vector<osg::Vec3f> uniqueVertices;
        uniqueVertices.reserve(3 * triangles.size());

        for (const RecastMeshTriangle& triangle : triangles)
            for (const osg::Vec3f& vertex : triangle.mVertices)
                uniqueVertices.push_back(vertex);

        std::sort(uniqueVertices.begin(), uniqueVertices.end());
        uniqueVertices.erase(std::unique(uniqueVertices.begin(), uniqueVertices.end()), uniqueVertices.end());

        std::vector<int> indices;
        indices.reserve(3 * triangles.size());
        std::vector<AreaType> areaTypes;
        areaTypes.reserve(triangles.size());

        for (const RecastMeshTriangle& triangle : triangles)
        {
            areaTypes.push_back(triangle.mAreaType);

            for (const osg::Vec3f& vertex : triangle.mVertices)
            {
                const auto it = std::lower_bound(uniqueVertices.begin(), uniqueVertices.end(), vertex);
                assert(it != uniqueVertices.end());
                assert(*it == vertex);
                indices.push_back(static_cast<int>(it - uniqueVertices.begin()));
            }
        }

        triangles.clear();

        std::vector<float> vertices;
        vertices.reserve(3 * uniqueVertices.size());

        for (const osg::Vec3f& vertex : uniqueVertices)
        {
            vertices.push_back(vertex.x() + shift.x());
            vertices.push_back(vertex.y() + shift.y());
            vertices.push_back(vertex.z() + shift.z());
        }

        return Mesh(std::move(indices), std::move(vertices), std::move(areaTypes));
    }

    Mesh makeMesh(const Heightfield& heightfield)
    {
        using BulletHelpers::makeProcessTriangleCallback;
        using Misc::Convert::toOsg;

        constexpr int upAxis = 2;
        constexpr bool flipQuadEdges = false;
#if BT_BULLET_VERSION < 310
        std::vector<btScalar> heights(heightfield.mHeights.begin(), heightfield.mHeights.end());
        btHeightfieldTerrainShape shape(static_cast<int>(heightfield.mHeights.size() / heightfield.mLength),
            static_cast<int>(heightfield.mLength), heights.data(), 1,
            heightfield.mMinHeight, heightfield.mMaxHeight, upAxis, PHY_FLOAT, flipQuadEdges
        );
#else
        btHeightfieldTerrainShape shape(static_cast<int>(heightfield.mHeights.size() / heightfield.mLength),
            static_cast<int>(heightfield.mLength), heightfield.mHeights.data(),
            heightfield.mMinHeight, heightfield.mMaxHeight, upAxis, flipQuadEdges);
#endif
        shape.setLocalScaling(btVector3(heightfield.mScale, heightfield.mScale, 1));
        btVector3 aabbMin;
        btVector3 aabbMax;
        shape.getAabb(btTransform::getIdentity(), aabbMin, aabbMax);
        std::vector<RecastMeshTriangle> triangles;
        auto callback = makeProcessTriangleCallback([&] (btVector3* vertices, int, int)
        {
            triangles.emplace_back(makeRecastMeshTriangle(vertices, AreaType_ground));
        });
        shape.processAllTriangles(&callback, aabbMin, aabbMax);
        const osg::Vec2f shift = (osg::Vec2f(aabbMax.x(), aabbMax.y()) - osg::Vec2f(aabbMin.x(), aabbMin.y())) * 0.5;
        return makeMesh(std::move(triangles), heightfield.mShift + osg::Vec3f(shift.x(), shift.y(), 0));
    }

    RecastMeshBuilder::RecastMeshBuilder(const TileBounds& bounds) noexcept
        : mBounds(bounds)
    {
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
        return addObject(shape, transform, makeProcessTriangleCallback([&] (btVector3* vertices, int, int)
        {
            RecastMeshTriangle triangle = makeRecastMeshTriangle(vertices, areaType);
            std::reverse(triangle.mVertices.begin(), triangle.mVertices.end());
            mTriangles.emplace_back(triangle);
        }));
    }

    void RecastMeshBuilder::addObject(const btHeightfieldTerrainShape& shape, const btTransform& transform,
                                      const AreaType areaType)
    {
        addObject(shape, transform, makeProcessTriangleCallback([&] (btVector3* vertices, int, int)
        {
            mTriangles.emplace_back(makeRecastMeshTriangle(vertices, areaType));
        }));
    }

    void RecastMeshBuilder::addObject(const btBoxShape& shape, const btTransform& transform, const AreaType areaType)
    {
        constexpr std::array<int, 36> indices {{
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

        for (std::size_t i = 0; i < indices.size(); i += 3)
        {
            std::array<btVector3, 3> vertices;
            for (std::size_t j = 0; j < 3; ++j)
            {
                btVector3 position;
                shape.getVertex(indices[i + j], position);
                vertices[j] = transform(position);
            }
            mTriangles.emplace_back(makeRecastMeshTriangle(vertices.data(), areaType));
        }
    }

    void RecastMeshBuilder::addWater(const int cellSize, const osg::Vec3f& shift)
    {
        mWater.push_back(Cell {cellSize, shift});
    }

    void RecastMeshBuilder::addHeightfield(int cellSize, const osg::Vec3f& shift, float height)
    {
        if (const auto intersection = getIntersection(mBounds, maxCellTileBounds(cellSize, shift)))
            mFlatHeightfields.emplace_back(FlatHeightfield {*intersection, height + shift.z()});
    }

    void RecastMeshBuilder::addHeightfield(int cellSize, const osg::Vec3f& shift, const float* heights,
        std::size_t size, float minHeight, float maxHeight)
    {
        const auto intersection = getIntersection(mBounds, maxCellTileBounds(cellSize, shift));
        if (!intersection.has_value())
            return;
        const float stepSize = static_cast<float>(cellSize) / (size - 1);
        const int halfCellSize = cellSize / 2;
        const auto local = [&] (float v, float shift) { return (v - shift + halfCellSize) / stepSize; };
        const auto index = [&] (float v, int add) { return std::clamp<int>(static_cast<int>(v) + add, 0, size); };
        const std::size_t minX = index(std::round(local(intersection->mMin.x(), shift.x())), -1);
        const std::size_t minY = index(std::round(local(intersection->mMin.y(), shift.y())), -1);
        const std::size_t maxX = index(std::round(local(intersection->mMax.x(), shift.x())), 1);
        const std::size_t maxY = index(std::round(local(intersection->mMax.y(), shift.y())), 1);
        const std::size_t endX = std::min(maxX + 1, size);
        const std::size_t endY = std::min(maxY + 1, size);
        const std::size_t sliceSize = (endX - minX) * (endY - minY);
        if (sliceSize == 0)
            return;
        std::vector<float> tileHeights;
        tileHeights.reserve(sliceSize);
        for (std::size_t y = minY; y < endY; ++y)
            for (std::size_t x = minX; x < endX; ++x)
                tileHeights.push_back(heights[x + y * size]);
        Heightfield heightfield;
        heightfield.mBounds = *intersection;
        heightfield.mLength = static_cast<std::uint8_t>(endY - minY);
        heightfield.mMinHeight = minHeight;
        heightfield.mMaxHeight = maxHeight;
        heightfield.mShift = shift + osg::Vec3f(minX, minY, 0) * stepSize - osg::Vec3f(halfCellSize, halfCellSize, 0);
        heightfield.mScale = stepSize;
        heightfield.mHeights = std::move(tileHeights);
        mHeightfields.push_back(std::move(heightfield));
    }

    std::shared_ptr<RecastMesh> RecastMeshBuilder::create(std::size_t generation, std::size_t revision) &&
    {
        std::sort(mTriangles.begin(), mTriangles.end());
        std::sort(mWater.begin(), mWater.end());
        Mesh mesh = makeMesh(std::move(mTriangles));
        return std::make_shared<RecastMesh>(generation, revision, std::move(mesh), std::move(mWater),
                                            std::move(mHeightfields), std::move(mFlatHeightfields));
    }

    void RecastMeshBuilder::addObject(const btConcaveShape& shape, const btTransform& transform,
                                      btTriangleCallback&& callback)
    {
        btVector3 aabbMin;
        btVector3 aabbMax;

        shape.getAabb(btTransform::getIdentity(), aabbMin, aabbMax);

        const btVector3 boundsMin(mBounds.mMin.x(), mBounds.mMin.y(),
            -std::numeric_limits<btScalar>::max() * std::numeric_limits<btScalar>::epsilon());
        const btVector3 boundsMax(mBounds.mMax.x(), mBounds.mMax.y(),
            std::numeric_limits<btScalar>::max() * std::numeric_limits<btScalar>::epsilon());

        auto wrapper = makeProcessTriangleCallback([&] (btVector3* triangle, int partId, int triangleIndex)
        {
            std::array<btVector3, 3> transformed;
            for (std::size_t i = 0; i < 3; ++i)
                transformed[i] = transform(triangle[i]);
            if (TestTriangleAgainstAabb2(transformed.data(), boundsMin, boundsMax))
                callback.processTriangle(transformed.data(), partId, triangleIndex);
        });

        shape.processAllTriangles(&wrapper, aabbMin, aabbMax);
    }

    void RecastMeshBuilder::addObject(const btHeightfieldTerrainShape& shape, const btTransform& transform,
                                      btTriangleCallback&& callback)
    {
        using BulletHelpers::transformBoundingBox;

        btVector3 aabbMin;
        btVector3 aabbMax;

        shape.getAabb(btTransform::getIdentity(), aabbMin, aabbMax);

        transformBoundingBox(transform, aabbMin, aabbMax);

        aabbMin.setX(std::max(static_cast<btScalar>(mBounds.mMin.x()), aabbMin.x()));
        aabbMin.setX(std::min(static_cast<btScalar>(mBounds.mMax.x()), aabbMin.x()));
        aabbMin.setY(std::max(static_cast<btScalar>(mBounds.mMin.y()), aabbMin.y()));
        aabbMin.setY(std::min(static_cast<btScalar>(mBounds.mMax.y()), aabbMin.y()));

        aabbMax.setX(std::max(static_cast<btScalar>(mBounds.mMin.x()), aabbMax.x()));
        aabbMax.setX(std::min(static_cast<btScalar>(mBounds.mMax.x()), aabbMax.x()));
        aabbMax.setY(std::max(static_cast<btScalar>(mBounds.mMin.y()), aabbMax.y()));
        aabbMax.setY(std::min(static_cast<btScalar>(mBounds.mMax.y()), aabbMax.y()));

        transformBoundingBox(transform.inverse(), aabbMin, aabbMax);

        auto wrapper = makeProcessTriangleCallback([&] (btVector3* triangle, int partId, int triangleIndex)
        {
            std::array<btVector3, 3> transformed;
            for (std::size_t i = 0; i < 3; ++i)
                transformed[i] = transform(triangle[i]);
            callback.processTriangle(transformed.data(), partId, triangleIndex);
        });

        shape.processAllTriangles(&wrapper, aabbMin, aabbMax);
    }
}
