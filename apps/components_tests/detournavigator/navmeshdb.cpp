#include "generate.hpp"

#include <components/detournavigator/navmeshdb.hpp>

#include <DetourAlloc.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <limits>
#include <random>

namespace
{
    using namespace testing;
    using namespace DetourNavigator;
    using namespace DetourNavigator::Tests;

    struct Tile
    {
        ESM::RefId mWorldspace;
        TilePosition mTilePosition;
        std::vector<std::byte> mInput;
        std::vector<std::byte> mData;
    };

    struct DetourNavigatorNavMeshDbTest : Test
    {
        NavMeshDb mDb{ ":memory:", std::numeric_limits<std::uint64_t>::max() };
        std::minstd_rand mRandom;

        std::vector<std::byte> generateData()
        {
            std::vector<std::byte> data(32);
            generateRange(data.begin(), data.end(), mRandom);
            return data;
        }

        Tile insertTile(TileId tileId, TileVersion version)
        {
            const ESM::RefId worldspace = ESM::RefId::stringRefId("sys::default");
            const TilePosition tilePosition{ 3, 4 };
            std::vector<std::byte> input = generateData();
            std::vector<std::byte> data = generateData();
            EXPECT_EQ(mDb.insertTile(tileId, worldspace, tilePosition, version, input, data), 1);
            return { worldspace, tilePosition, std::move(input), std::move(data) };
        }
    };

    TEST_F(DetourNavigatorNavMeshDbTest, get_max_tile_id_for_empty_db_should_return_zero)
    {
        EXPECT_EQ(mDb.getMaxTileId(), TileId{ 0 });
    }

    TEST_F(DetourNavigatorNavMeshDbTest, inserted_tile_should_be_found_by_key)
    {
        const TileId tileId{ 146 };
        const TileVersion version{ 1 };
        const auto [worldspace, tilePosition, input, data] = insertTile(tileId, version);
        const auto result = mDb.findTile(worldspace, tilePosition, input);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->mTileId, tileId);
        EXPECT_EQ(result->mVersion, version);
    }

    TEST_F(DetourNavigatorNavMeshDbTest, inserted_tile_should_change_max_tile_id)
    {
        insertTile(TileId{ 53 }, TileVersion{ 1 });
        EXPECT_EQ(mDb.getMaxTileId(), TileId{ 53 });
    }

    TEST_F(DetourNavigatorNavMeshDbTest, updated_tile_should_change_data)
    {
        const TileId tileId{ 13 };
        const TileVersion version{ 1 };
        auto [worldspace, tilePosition, input, data] = insertTile(tileId, version);
        generateRange(data.begin(), data.end(), mRandom);
        ASSERT_EQ(mDb.updateTile(tileId, version, data), 1);
        const auto row = mDb.getTileData(worldspace, tilePosition, input);
        ASSERT_TRUE(row.has_value());
        EXPECT_EQ(row->mTileId, tileId);
        EXPECT_EQ(row->mVersion, version);
        ASSERT_FALSE(row->mData.empty());
        EXPECT_EQ(row->mData, data);
    }

    TEST_F(DetourNavigatorNavMeshDbTest, on_inserted_duplicate_should_throw_exception)
    {
        const TileId tileId{ 53 };
        const TileVersion version{ 1 };
        const ESM::RefId worldspace = ESM::RefId::stringRefId("sys::default");
        const TilePosition tilePosition{ 3, 4 };
        const std::vector<std::byte> input = generateData();
        const std::vector<std::byte> data = generateData();
        ASSERT_EQ(mDb.insertTile(tileId, worldspace, tilePosition, version, input, data), 1);
        EXPECT_THROW(mDb.insertTile(tileId, worldspace, tilePosition, version, input, data), std::runtime_error);
    }

    TEST_F(DetourNavigatorNavMeshDbTest, inserted_duplicate_leaves_db_in_correct_state)
    {
        const TileId tileId{ 53 };
        const TileVersion version{ 1 };
        const ESM::RefId worldspace = ESM::RefId::stringRefId("sys::default");
        const TilePosition tilePosition{ 3, 4 };
        const std::vector<std::byte> input = generateData();
        const std::vector<std::byte> data = generateData();
        ASSERT_EQ(mDb.insertTile(tileId, worldspace, tilePosition, version, input, data), 1);
        EXPECT_THROW(mDb.insertTile(tileId, worldspace, tilePosition, version, input, data), std::runtime_error);
        EXPECT_NO_THROW(insertTile(TileId{ 54 }, version));
    }

    TEST_F(DetourNavigatorNavMeshDbTest, delete_tiles_at_should_remove_all_tiles_with_given_worldspace_and_position)
    {
        const TileVersion version{ 1 };
        const ESM::RefId worldspace = ESM::RefId::stringRefId("sys::default");
        const TilePosition tilePosition{ 3, 4 };
        const std::vector<std::byte> input1 = generateData();
        const std::vector<std::byte> input2 = generateData();
        const std::vector<std::byte> data = generateData();
        ASSERT_EQ(mDb.insertTile(TileId{ 53 }, worldspace, tilePosition, version, input1, data), 1);
        ASSERT_EQ(mDb.insertTile(TileId{ 54 }, worldspace, tilePosition, version, input2, data), 1);
        ASSERT_EQ(mDb.deleteTilesAt(worldspace, tilePosition), 2);
        EXPECT_FALSE(mDb.findTile(worldspace, tilePosition, input1).has_value());
        EXPECT_FALSE(mDb.findTile(worldspace, tilePosition, input2).has_value());
    }

    TEST_F(DetourNavigatorNavMeshDbTest, delete_tiles_at_except_should_leave_tile_with_given_id)
    {
        const TileId leftTileId{ 53 };
        const TileId removedTileId{ 54 };
        const TileVersion version{ 1 };
        const ESM::RefId worldspace = ESM::RefId::stringRefId("sys::default");
        const TilePosition tilePosition{ 3, 4 };
        const std::vector<std::byte> leftInput = generateData();
        const std::vector<std::byte> removedInput = generateData();
        const std::vector<std::byte> data = generateData();
        ASSERT_EQ(mDb.insertTile(leftTileId, worldspace, tilePosition, version, leftInput, data), 1);
        ASSERT_EQ(mDb.insertTile(removedTileId, worldspace, tilePosition, version, removedInput, data), 1);
        ASSERT_EQ(mDb.deleteTilesAtExcept(worldspace, tilePosition, leftTileId), 1);
        const auto left = mDb.findTile(worldspace, tilePosition, leftInput);
        ASSERT_TRUE(left.has_value());
        EXPECT_EQ(left->mTileId, leftTileId);
        EXPECT_FALSE(mDb.findTile(worldspace, tilePosition, removedInput).has_value());
    }

    TEST_F(DetourNavigatorNavMeshDbTest, delete_tiles_outside_range_should_leave_tiles_inside_given_rectangle)
    {
        TileId tileId{ 1 };
        const TileVersion version{ 1 };
        const ESM::RefId worldspace = ESM::RefId::stringRefId("sys::default");
        const std::vector<std::byte> input = generateData();
        const std::vector<std::byte> data = generateData();
        for (int x = -2; x <= 2; ++x)
        {
            for (int y = -2; y <= 2; ++y)
            {
                ASSERT_EQ(mDb.insertTile(tileId, worldspace, TilePosition{ x, y }, version, input, data), 1);
                ++tileId;
            }
        }
        const TilesPositionsRange range{ TilePosition{ -1, -1 }, TilePosition{ 2, 2 } };
        ASSERT_EQ(mDb.deleteTilesOutsideRange(worldspace, range), 16);
        for (int x = -2; x <= 2; ++x)
            for (int y = -2; y <= 2; ++y)
                ASSERT_EQ(mDb.findTile(worldspace, TilePosition{ x, y }, input).has_value(),
                    -1 <= x && x <= 1 && -1 <= y && y <= 1)
                    << "x=" << x << " y=" << y;
    }

    TEST_F(DetourNavigatorNavMeshDbTest, should_support_file_size_limit)
    {
        mDb = NavMeshDb(":memory:", 4096);
        const auto f = [&] {
            for (std::int64_t i = 1; i <= 100; ++i)
                insertTile(TileId{ i }, TileVersion{ 1 });
        };
        EXPECT_THROW(f(), std::runtime_error);
    }
}
