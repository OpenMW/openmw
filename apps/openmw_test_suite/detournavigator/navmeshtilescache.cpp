#include "operators.hpp"

#include <components/detournavigator/navmeshtilescache.hpp>
#include <components/detournavigator/exceptions.hpp>
#include <components/detournavigator/recastmesh.hpp>

#include <LinearMath/btTransform.h>

#include <gtest/gtest.h>

namespace DetourNavigator
{
    static inline bool operator ==(const NavMeshDataRef& lhs, const NavMeshDataRef& rhs)
    {
        return std::make_pair(lhs.mValue, lhs.mSize) == std::make_pair(rhs.mValue, rhs.mSize);
    }
}

namespace
{
    using namespace testing;
    using namespace DetourNavigator;

    struct DetourNavigatorNavMeshTilesCacheTest : Test
    {
        const osg::Vec3f mAgentHalfExtents {1, 2, 3};
        const TilePosition mTilePosition {0, 0};
        const std::size_t mGeneration = 0;
        const std::size_t mRevision = 0;
        const std::vector<int> mIndices {{0, 1, 2}};
        const std::vector<float> mVertices {{0, 0, 0, 1, 0, 0, 1, 1, 0}};
        const std::vector<AreaType> mAreaTypes {1, AreaType_ground};
        const std::vector<RecastMesh::Water> mWater {};
        const std::size_t mTrianglesPerChunk {1};
        const RecastMesh mRecastMesh {mGeneration, mRevision, mIndices, mVertices,
                                      mAreaTypes, mWater, mTrianglesPerChunk};
        const std::vector<OffMeshConnection> mOffMeshConnections {};
        unsigned char* const mData = reinterpret_cast<unsigned char*>(dtAlloc(1, DT_ALLOC_PERM));
        NavMeshData mNavMeshData {mData, 1};

        const size_t cRecastMeshKeySize = mRecastMesh.getIndices().size() * sizeof(int)
            + mRecastMesh.getVertices().size() * sizeof(float)
            + mRecastMesh.getAreaTypes().size() * sizeof(AreaType)
            + mRecastMesh.getWater().size() * sizeof(RecastMesh::Water)
            + mOffMeshConnections.size() * sizeof(OffMeshConnection);

        const size_t cRecastMeshWithWaterKeySize = cRecastMeshKeySize + sizeof(RecastMesh::Water);
    };

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, get_for_empty_cache_should_return_empty_value)
    {
        const std::size_t maxSize = 0;
        NavMeshTilesCache cache(maxSize);

        EXPECT_FALSE(cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, set_for_not_enought_cache_size_should_return_empty_value)
    {
        const std::size_t maxSize = 0;
        NavMeshTilesCache cache(maxSize);

        EXPECT_FALSE(cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections,
                               std::move(mNavMeshData)));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, set_should_return_cached_value)
    {
        const std::size_t navMeshDataSize = 1;
        const std::size_t navMeshKeySize = cRecastMeshKeySize;
        const std::size_t maxSize = navMeshDataSize + 2 * navMeshKeySize;
        NavMeshTilesCache cache(maxSize);

        const auto result = cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections,
                                      std::move(mNavMeshData));
        ASSERT_TRUE(result);
        EXPECT_EQ(result.get(), (NavMeshDataRef {mData, 1}));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, set_existing_element_should_throw_exception)
    {
        const std::size_t navMeshDataSize = 1;
        const std::size_t navMeshKeySize = cRecastMeshKeySize;
        const std::size_t maxSize = 2 * (navMeshDataSize + 2 * navMeshKeySize);
        NavMeshTilesCache cache(maxSize);
        const auto anotherData = reinterpret_cast<unsigned char*>(dtAlloc(1, DT_ALLOC_PERM));
        NavMeshData anotherNavMeshData {anotherData, 1};

        cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections, std::move(mNavMeshData));
        EXPECT_THROW(
            cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections, std::move(anotherNavMeshData)),
            InvalidArgument
        );
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, get_should_return_cached_value)
    {
        const std::size_t navMeshDataSize = 1;
        const std::size_t navMeshKeySize = cRecastMeshKeySize;
        const std::size_t maxSize = navMeshDataSize + 2 * navMeshKeySize;
        NavMeshTilesCache cache(maxSize);

        cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections, std::move(mNavMeshData));
        const auto result = cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections);
        ASSERT_TRUE(result);
        EXPECT_EQ(result.get(), (NavMeshDataRef {mData, 1}));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, get_for_cache_miss_by_agent_half_extents_should_return_empty_value)
    {
        const std::size_t maxSize = 1;
        NavMeshTilesCache cache(maxSize);
        const osg::Vec3f unexsistentAgentHalfExtents {1, 1, 1};

        cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections, std::move(mNavMeshData));
        EXPECT_FALSE(cache.get(unexsistentAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, get_for_cache_miss_by_tile_position_should_return_empty_value)
    {
        const std::size_t maxSize = 1;
        NavMeshTilesCache cache(maxSize);
        const TilePosition unexistentTilePosition {1, 1};

        cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections, std::move(mNavMeshData));
        EXPECT_FALSE(cache.get(mAgentHalfExtents, unexistentTilePosition, mRecastMesh, mOffMeshConnections));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, get_for_cache_miss_by_recast_mesh_should_return_empty_value)
    {
        const std::size_t maxSize = 1;
        NavMeshTilesCache cache(maxSize);
        const std::vector<RecastMesh::Water> water {1, RecastMesh::Water {1, btTransform::getIdentity()}};
        const RecastMesh unexistentRecastMesh {mGeneration, mRevision, mIndices, mVertices,
            mAreaTypes, water, mTrianglesPerChunk};

        cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections, std::move(mNavMeshData));
        EXPECT_FALSE(cache.get(mAgentHalfExtents, mTilePosition, unexistentRecastMesh, mOffMeshConnections));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, set_should_replace_unused_value)
    {
        const std::size_t navMeshDataSize = 1;
        const std::size_t navMeshKeySize = cRecastMeshWithWaterKeySize;
        const std::size_t maxSize = navMeshDataSize + 2 * navMeshKeySize;
        NavMeshTilesCache cache(maxSize);

        const std::vector<RecastMesh::Water> water {1, RecastMesh::Water {1, btTransform::getIdentity()}};
        const RecastMesh anotherRecastMesh {mGeneration, mRevision, mIndices, mVertices,
            mAreaTypes, water, mTrianglesPerChunk};
        const auto anotherData = reinterpret_cast<unsigned char*>(dtAlloc(1, DT_ALLOC_PERM));
        NavMeshData anotherNavMeshData {anotherData, 1};

        cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections, std::move(mNavMeshData));
        const auto result = cache.set(mAgentHalfExtents, mTilePosition, anotherRecastMesh, mOffMeshConnections,
                                      std::move(anotherNavMeshData));
        ASSERT_TRUE(result);
        EXPECT_EQ(result.get(), (NavMeshDataRef {anotherData, 1}));
        EXPECT_FALSE(cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, set_should_not_replace_used_value)
    {
        const std::size_t navMeshDataSize = 1;
        const std::size_t navMeshKeySize = cRecastMeshKeySize;
        const std::size_t maxSize = navMeshDataSize + 2 * navMeshKeySize;
        NavMeshTilesCache cache(maxSize);

        const std::vector<RecastMesh::Water> water {1, RecastMesh::Water {1, btTransform::getIdentity()}};
        const RecastMesh anotherRecastMesh {mGeneration, mRevision, mIndices, mVertices,
            mAreaTypes, water, mTrianglesPerChunk};
        const auto anotherData = reinterpret_cast<unsigned char*>(dtAlloc(1, DT_ALLOC_PERM));
        NavMeshData anotherNavMeshData {anotherData, 1};

        const auto value = cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections,
                                     std::move(mNavMeshData));
        EXPECT_FALSE(cache.set(mAgentHalfExtents, mTilePosition, anotherRecastMesh, mOffMeshConnections,
                               std::move(anotherNavMeshData)));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, set_should_replace_unused_least_recently_set_value)
    {
        const std::size_t navMeshDataSize = 1;
        const std::size_t navMeshKeySize = cRecastMeshWithWaterKeySize;
        const std::size_t maxSize = 2 * (navMeshDataSize + 2 * navMeshKeySize);
        NavMeshTilesCache cache(maxSize);

        const std::vector<RecastMesh::Water> leastRecentlySetWater {1, RecastMesh::Water {1, btTransform::getIdentity()}};
        const RecastMesh leastRecentlySetRecastMesh {mGeneration, mRevision, mIndices, mVertices,
            mAreaTypes, leastRecentlySetWater, mTrianglesPerChunk};
        const auto leastRecentlySetData = reinterpret_cast<unsigned char*>(dtAlloc(1, DT_ALLOC_PERM));
        NavMeshData leastRecentlySetNavMeshData {leastRecentlySetData, 1};

        const std::vector<RecastMesh::Water> mostRecentlySetWater {1, RecastMesh::Water {2, btTransform::getIdentity()}};
        const RecastMesh mostRecentlySetRecastMesh {mGeneration, mRevision, mIndices, mVertices,
            mAreaTypes, mostRecentlySetWater, mTrianglesPerChunk};
        const auto mostRecentlySetData = reinterpret_cast<unsigned char*>(dtAlloc(1, DT_ALLOC_PERM));
        NavMeshData mostRecentlySetNavMeshData {mostRecentlySetData, 1};

        ASSERT_TRUE(cache.set(mAgentHalfExtents, mTilePosition, leastRecentlySetRecastMesh, mOffMeshConnections,
                              std::move(leastRecentlySetNavMeshData)));
        ASSERT_TRUE(cache.set(mAgentHalfExtents, mTilePosition, mostRecentlySetRecastMesh, mOffMeshConnections,
                              std::move(mostRecentlySetNavMeshData)));

        const auto result = cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections,
                                      std::move(mNavMeshData));
        EXPECT_EQ(result.get(), (NavMeshDataRef {mData, 1}));

        EXPECT_FALSE(cache.get(mAgentHalfExtents, mTilePosition, leastRecentlySetRecastMesh, mOffMeshConnections));
        EXPECT_TRUE(cache.get(mAgentHalfExtents, mTilePosition, mostRecentlySetRecastMesh, mOffMeshConnections));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, set_should_replace_unused_least_recently_used_value)
    {
        const std::size_t navMeshDataSize = 1;
        const std::size_t navMeshKeySize = cRecastMeshWithWaterKeySize;
        const std::size_t maxSize = 2 * (navMeshDataSize + 2 * navMeshKeySize);
        NavMeshTilesCache cache(maxSize);

        const std::vector<RecastMesh::Water> leastRecentlyUsedWater {1, RecastMesh::Water {1, btTransform::getIdentity()}};
        const RecastMesh leastRecentlyUsedRecastMesh {mGeneration, mRevision, mIndices, mVertices,
            mAreaTypes, leastRecentlyUsedWater, mTrianglesPerChunk};
        const auto leastRecentlyUsedData = reinterpret_cast<unsigned char*>(dtAlloc(1, DT_ALLOC_PERM));
        NavMeshData leastRecentlyUsedNavMeshData {leastRecentlyUsedData, 1};

        const std::vector<RecastMesh::Water> mostRecentlyUsedWater {1, RecastMesh::Water {2, btTransform::getIdentity()}};
        const RecastMesh mostRecentlyUsedRecastMesh {mGeneration, mRevision, mIndices, mVertices,
            mAreaTypes, mostRecentlyUsedWater, mTrianglesPerChunk};
        const auto mostRecentlyUsedData = reinterpret_cast<unsigned char*>(dtAlloc(1, DT_ALLOC_PERM));
        NavMeshData mostRecentlyUsedNavMeshData {mostRecentlyUsedData, 1};

        cache.set(mAgentHalfExtents, mTilePosition, leastRecentlyUsedRecastMesh, mOffMeshConnections,
                  std::move(leastRecentlyUsedNavMeshData));
        cache.set(mAgentHalfExtents, mTilePosition, mostRecentlyUsedRecastMesh, mOffMeshConnections,
                  std::move(mostRecentlyUsedNavMeshData));

        {
            const auto value = cache.get(mAgentHalfExtents, mTilePosition, leastRecentlyUsedRecastMesh, mOffMeshConnections);
            ASSERT_TRUE(value);
            ASSERT_EQ(value.get(), (NavMeshDataRef {leastRecentlyUsedData, 1}));
        }

        {
            const auto value = cache.get(mAgentHalfExtents, mTilePosition, mostRecentlyUsedRecastMesh, mOffMeshConnections);
            ASSERT_TRUE(value);
            ASSERT_EQ(value.get(), (NavMeshDataRef {mostRecentlyUsedData, 1}));
        }

        const auto result = cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections,
                                      std::move(mNavMeshData));
        EXPECT_EQ(result.get(), (NavMeshDataRef {mData, 1}));

        EXPECT_FALSE(cache.get(mAgentHalfExtents, mTilePosition, leastRecentlyUsedRecastMesh, mOffMeshConnections));
        EXPECT_TRUE(cache.get(mAgentHalfExtents, mTilePosition, mostRecentlyUsedRecastMesh, mOffMeshConnections));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, set_should_not_replace_unused_least_recently_used_value_when_item_does_not_not_fit_cache_max_size)
    {
        const std::size_t navMeshDataSize = 1;
        const std::size_t navMeshKeySize = cRecastMeshKeySize;
        const std::size_t maxSize = 2 * (navMeshDataSize + 2 * navMeshKeySize);
        NavMeshTilesCache cache(maxSize);

        const std::vector<RecastMesh::Water> water {1, RecastMesh::Water {1, btTransform::getIdentity()}};
        const RecastMesh tooLargeRecastMesh {mGeneration, mRevision, mIndices, mVertices, mAreaTypes, water, mTrianglesPerChunk};
        const auto tooLargeData = reinterpret_cast<unsigned char*>(dtAlloc(2, DT_ALLOC_PERM));
        NavMeshData tooLargeNavMeshData {tooLargeData, 2};

        cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections, std::move(mNavMeshData));
        EXPECT_FALSE(cache.set(mAgentHalfExtents, mTilePosition, tooLargeRecastMesh, mOffMeshConnections,
                               std::move(tooLargeNavMeshData)));
        EXPECT_TRUE(cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, set_should_not_replace_unused_least_recently_used_value_when_item_does_not_not_fit_size_of_unused_items)
    {
        const std::size_t navMeshDataSize = 1;
        const std::size_t navMeshKeySize1 = cRecastMeshKeySize;
        const std::size_t navMeshKeySize2 = cRecastMeshWithWaterKeySize;
        const std::size_t maxSize = 2 * navMeshDataSize + 2 * navMeshKeySize1 + 2 * navMeshKeySize2;
        NavMeshTilesCache cache(maxSize);

        const std::vector<RecastMesh::Water> anotherWater {1, RecastMesh::Water {1, btTransform::getIdentity()}};
        const RecastMesh anotherRecastMesh {mGeneration, mRevision, mIndices, mVertices, mAreaTypes, anotherWater, mTrianglesPerChunk};
        const auto anotherData = reinterpret_cast<unsigned char*>(dtAlloc(1, DT_ALLOC_PERM));
        NavMeshData anotherNavMeshData {anotherData, 1};

        const std::vector<RecastMesh::Water> tooLargeWater {1, RecastMesh::Water {2, btTransform::getIdentity()}};
        const RecastMesh tooLargeRecastMesh {mGeneration, mRevision, mIndices, mVertices,
            mAreaTypes, tooLargeWater, mTrianglesPerChunk};
        const auto tooLargeData = reinterpret_cast<unsigned char*>(dtAlloc(2, DT_ALLOC_PERM));
        NavMeshData tooLargeNavMeshData {tooLargeData, 2};

        const auto value = cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections,
                                     std::move(mNavMeshData));
        ASSERT_TRUE(value);
        ASSERT_TRUE(cache.set(mAgentHalfExtents, mTilePosition, anotherRecastMesh, mOffMeshConnections,
                              std::move(anotherNavMeshData)));
        EXPECT_FALSE(cache.set(mAgentHalfExtents, mTilePosition, tooLargeRecastMesh, mOffMeshConnections,
                               std::move(tooLargeNavMeshData)));
        EXPECT_TRUE(cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections));
        EXPECT_TRUE(cache.get(mAgentHalfExtents, mTilePosition, anotherRecastMesh, mOffMeshConnections));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, release_used_after_set_then_used_by_get_item_should_left_this_item_available)
    {
        const std::size_t navMeshDataSize = 1;
        const std::size_t navMeshKeySize = cRecastMeshKeySize;
        const std::size_t maxSize = navMeshDataSize + 2 * navMeshKeySize;
        NavMeshTilesCache cache(maxSize);

        const std::vector<RecastMesh::Water> water {1, RecastMesh::Water {1, btTransform::getIdentity()}};
        const RecastMesh anotherRecastMesh {mGeneration, mRevision, mIndices, mVertices,
            mAreaTypes, water, mTrianglesPerChunk};
        const auto anotherData = reinterpret_cast<unsigned char*>(dtAlloc(1, DT_ALLOC_PERM));
        NavMeshData anotherNavMeshData {anotherData, 1};

        const auto firstCopy = cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections, std::move(mNavMeshData));
        ASSERT_TRUE(firstCopy);
        {
            const auto secondCopy = cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections);
            ASSERT_TRUE(secondCopy);
        }
        EXPECT_FALSE(cache.set(mAgentHalfExtents, mTilePosition, anotherRecastMesh, mOffMeshConnections,
                               std::move(anotherNavMeshData)));
        EXPECT_TRUE(cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections));
    }

    TEST_F(DetourNavigatorNavMeshTilesCacheTest, release_twice_used_item_should_left_this_item_available)
    {
        const std::size_t navMeshDataSize = 1;
        const std::size_t navMeshKeySize = cRecastMeshKeySize;
        const std::size_t maxSize = navMeshDataSize + 2 * navMeshKeySize;
        NavMeshTilesCache cache(maxSize);

        const std::vector<RecastMesh::Water> water {1, RecastMesh::Water {1, btTransform::getIdentity()}};
        const RecastMesh anotherRecastMesh {mGeneration, mRevision, mIndices, mVertices, mAreaTypes, water, mTrianglesPerChunk};
        const auto anotherData = reinterpret_cast<unsigned char*>(dtAlloc(1, DT_ALLOC_PERM));
        NavMeshData anotherNavMeshData {anotherData, 1};

        cache.set(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections, std::move(mNavMeshData));
        const auto firstCopy = cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections);
        ASSERT_TRUE(firstCopy);
        {
            const auto secondCopy = cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections);
            ASSERT_TRUE(secondCopy);
        }
        EXPECT_FALSE(cache.set(mAgentHalfExtents, mTilePosition, anotherRecastMesh, mOffMeshConnections,
                               std::move(anotherNavMeshData)));
        EXPECT_TRUE(cache.get(mAgentHalfExtents, mTilePosition, mRecastMesh, mOffMeshConnections));
    }
}
