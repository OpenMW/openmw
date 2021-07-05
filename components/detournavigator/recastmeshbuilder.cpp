#include "recastmeshbuilder.hpp"
#include "debug.hpp"
#include "settings.hpp"
#include "settingsutils.hpp"
#include "exceptions.hpp"

#include <components/bullethelpers/transformboundingbox.hpp>
#include <components/bullethelpers/processtrianglecallback.hpp>
#include <components/misc/convert.hpp>

#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <BulletCollision/CollisionShapes/btConcaveShape.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <LinearMath/btTransform.h>
#include <LinearMath/btAabbUtil2.h>

#include <algorithm>
#include <cassert>
#include <tuple>
#include <array>

namespace DetourNavigator
{
    using BulletHelpers::makeProcessTriangleCallback;

    namespace
    {
        void optimizeRecastMesh(std::vector<int>& indices, std::vector<float>& vertices)
        {
            std::vector<std::tuple<float, float, float>> uniqueVertices;
            uniqueVertices.reserve(vertices.size() / 3);

            for (std::size_t i = 0, n = vertices.size() / 3; i < n; ++i)
                uniqueVertices.emplace_back(vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]);

            std::sort(uniqueVertices.begin(), uniqueVertices.end());
            const auto end = std::unique(uniqueVertices.begin(), uniqueVertices.end());
            uniqueVertices.erase(end, uniqueVertices.end());

            if (uniqueVertices.size() == vertices.size() / 3)
                return;

            for (std::size_t i = 0, n = indices.size(); i < n; ++i)
            {
                const auto index = indices[i];
                const auto vertex = std::make_tuple(vertices[index * 3], vertices[index * 3 + 1], vertices[index * 3 + 2]);
                const auto it = std::lower_bound(uniqueVertices.begin(), uniqueVertices.end(), vertex);
                assert(it != uniqueVertices.end());
                assert(*it == vertex);
                indices[i] = std::distance(uniqueVertices.begin(), it);
            }

            vertices.resize(uniqueVertices.size() * 3);

            for (std::size_t i = 0, n = uniqueVertices.size(); i < n; ++i)
            {
                vertices[i * 3] = std::get<0>(uniqueVertices[i]);
                vertices[i * 3 + 1] = std::get<1>(uniqueVertices[i]);
                vertices[i * 3 + 2] = std::get<2>(uniqueVertices[i]);
            }
        }
    }

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
                addTriangleVertex(triangle[i - 1]);
            mAreaTypes.push_back(areaType);
        }));
    }

    void RecastMeshBuilder::addObject(const btHeightfieldTerrainShape& shape, const btTransform& transform,
                                      const AreaType areaType)
    {
        return addObject(shape, transform, makeProcessTriangleCallback([&] (btVector3* triangle, int, int)
        {
            for (std::size_t i = 0; i < 3; ++i)
                addTriangleVertex(triangle[i]);
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

    std::shared_ptr<RecastMesh> RecastMeshBuilder::create(std::size_t generation, std::size_t revision) &&
    {
        optimizeRecastMesh(mIndices, mVertices);
        std::sort(mWater.begin(), mWater.end());
        return std::make_shared<RecastMesh>(generation, revision, std::move(mIndices), std::move(mVertices), std::move(mAreaTypes), std::move(mWater));
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

    void RecastMeshBuilder::addTriangleVertex(const btVector3& worldPosition)
    {
        mIndices.push_back(static_cast<int>(mVertices.size() / 3));
        addVertex(worldPosition);
    }

    void RecastMeshBuilder::addVertex(const btVector3& worldPosition)
    {
        const auto navMeshPosition = toNavMeshCoordinates(mSettings, Misc::Convert::makeOsgVec3f(worldPosition));
        mVertices.push_back(navMeshPosition.x());
        mVertices.push_back(navMeshPosition.y());
        mVertices.push_back(navMeshPosition.z());
    }
}
