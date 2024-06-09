#include <components/detournavigator/debug.hpp>
#include <components/detournavigator/gettilespositions.hpp>
#include <components/detournavigator/settings.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
    using namespace testing;
    using namespace DetourNavigator;

    struct CollectTilesPositions
    {
        std::vector<TilePosition>& mTilesPositions;

        void operator()(const TilePosition& value) { mTilesPositions.push_back(value); }
    };

    struct DetourNavigatorGetTilesPositionsTest : Test
    {
        RecastSettings mSettings;
        std::vector<TilePosition> mTilesPositions;
        CollectTilesPositions mCollect{ mTilesPositions };

        DetourNavigatorGetTilesPositionsTest()
        {
            mSettings.mBorderSize = 0;
            mSettings.mCellSize = 0.5;
            mSettings.mRecastScaleFactor = 1;
            mSettings.mTileSize = 64;
        }
    };

    TEST_F(DetourNavigatorGetTilesPositionsTest, for_object_in_single_tile_should_return_one_tile)
    {
        getTilesPositions(makeTilesPositionsRange(osg::Vec2f(2, 2), osg::Vec2f(31, 31), mSettings), mCollect);

        EXPECT_THAT(mTilesPositions, ElementsAre(TilePosition(0, 0)));
    }

    TEST_F(DetourNavigatorGetTilesPositionsTest, for_object_with_x_bounds_in_two_tiles_should_return_two_tiles)
    {
        getTilesPositions(makeTilesPositionsRange(osg::Vec2f(0, 0), osg::Vec2f(32, 31), mSettings), mCollect);

        EXPECT_THAT(mTilesPositions, ElementsAre(TilePosition(0, 0), TilePosition(1, 0)));
    }

    TEST_F(DetourNavigatorGetTilesPositionsTest, for_object_with_y_bounds_in_two_tiles_should_return_two_tiles)
    {
        getTilesPositions(makeTilesPositionsRange(osg::Vec2f(0, 0), osg::Vec2f(31, 32), mSettings), mCollect);

        EXPECT_THAT(mTilesPositions, ElementsAre(TilePosition(0, 0), TilePosition(0, 1)));
    }

    TEST_F(DetourNavigatorGetTilesPositionsTest, tiling_works_only_for_x_and_y_coordinates)
    {
        getTilesPositions(makeTilesPositionsRange(osg::Vec2f(0, 0), osg::Vec2f(31, 31), mSettings), mCollect);

        EXPECT_THAT(mTilesPositions, ElementsAre(TilePosition(0, 0)));
    }

    TEST_F(DetourNavigatorGetTilesPositionsTest, tiling_should_work_with_negative_coordinates)
    {
        getTilesPositions(makeTilesPositionsRange(osg::Vec2f(-31, -31), osg::Vec2f(31, 31), mSettings), mCollect);

        EXPECT_THAT(mTilesPositions,
            ElementsAre(TilePosition(-1, -1), TilePosition(-1, 0), TilePosition(0, -1), TilePosition(0, 0)));
    }

    TEST_F(DetourNavigatorGetTilesPositionsTest, border_size_should_extend_tile_bounds)
    {
        mSettings.mBorderSize = 1;

        getTilesPositions(makeTilesPositionsRange(osg::Vec2f(0, 0), osg::Vec2f(31.5, 31.5), mSettings), mCollect);

        EXPECT_THAT(mTilesPositions,
            ElementsAre(TilePosition(-1, -1), TilePosition(-1, 0), TilePosition(-1, 1), TilePosition(0, -1),
                TilePosition(0, 0), TilePosition(0, 1), TilePosition(1, -1), TilePosition(1, 0), TilePosition(1, 1)));
    }

    TEST_F(DetourNavigatorGetTilesPositionsTest, should_apply_recast_scale_factor)
    {
        mSettings.mRecastScaleFactor = 0.5;

        getTilesPositions(makeTilesPositionsRange(osg::Vec2f(0, 0), osg::Vec2f(32, 32), mSettings), mCollect);

        EXPECT_THAT(mTilesPositions, ElementsAre(TilePosition(0, 0)));
    }

    struct TilesPositionsRangeParams
    {
        TilesPositionsRange mA;
        TilesPositionsRange mB;
        TilesPositionsRange mResult;
    };

    struct DetourNavigatorGetIntersectionTest : TestWithParam<TilesPositionsRangeParams>
    {
    };

    TEST_P(DetourNavigatorGetIntersectionTest, should_return_expected_result)
    {
        EXPECT_EQ(getIntersection(GetParam().mA, GetParam().mB), GetParam().mResult);
        EXPECT_EQ(getIntersection(GetParam().mB, GetParam().mA), GetParam().mResult);
    }

    const TilesPositionsRangeParams getIntersectionParams[] = {
        { .mA = TilesPositionsRange{}, .mB = TilesPositionsRange{}, .mResult = TilesPositionsRange{} },
        {
            .mA = TilesPositionsRange{ .mBegin = TilePosition{ 0, 0 }, .mEnd = TilePosition{ 2, 2 } },
            .mB = TilesPositionsRange{ .mBegin = TilePosition{ 1, 1 }, .mEnd = TilePosition{ 3, 3 } },
            .mResult = TilesPositionsRange{ .mBegin = TilePosition{ 1, 1 }, .mEnd = TilePosition{ 2, 2 } },
        },
        {
            .mA = TilesPositionsRange{ .mBegin = TilePosition{ 0, 0 }, .mEnd = TilePosition{ 1, 1 } },
            .mB = TilesPositionsRange{ .mBegin = TilePosition{ 2, 2 }, .mEnd = TilePosition{ 3, 3 } },
            .mResult = TilesPositionsRange{},
        },
        {
            .mA = TilesPositionsRange{ .mBegin = TilePosition{ 0, 0 }, .mEnd = TilePosition{ 1, 1 } },
            .mB = TilesPositionsRange{ .mBegin = TilePosition{ 1, 1 }, .mEnd = TilePosition{ 2, 2 } },
            .mResult = TilesPositionsRange{ .mBegin = TilePosition{ 1, 1 }, .mEnd = TilePosition{ 1, 1 } },
        },
        {
            .mA = TilesPositionsRange{ .mBegin = TilePosition{ 0, 0 }, .mEnd = TilePosition{ 1, 1 } },
            .mB = TilesPositionsRange{ .mBegin = TilePosition{ 0, 2 }, .mEnd = TilePosition{ 3, 3 } },
            .mResult = TilesPositionsRange{},
        },
        {
            .mA = TilesPositionsRange{ .mBegin = TilePosition{ 0, 0 }, .mEnd = TilePosition{ 1, 1 } },
            .mB = TilesPositionsRange{ .mBegin = TilePosition{ 2, 0 }, .mEnd = TilePosition{ 3, 3 } },
            .mResult = TilesPositionsRange{},
        },
    };

    INSTANTIATE_TEST_SUITE_P(
        GetIntersectionParams, DetourNavigatorGetIntersectionTest, ValuesIn(getIntersectionParams));

    struct DetourNavigatorGetUnionTest : TestWithParam<TilesPositionsRangeParams>
    {
    };

    TEST_P(DetourNavigatorGetUnionTest, should_return_expected_result)
    {
        EXPECT_EQ(getUnion(GetParam().mA, GetParam().mB), GetParam().mResult);
        EXPECT_EQ(getUnion(GetParam().mB, GetParam().mA), GetParam().mResult);
    }

    const TilesPositionsRangeParams getUnionParams[] = {
        { .mA = TilesPositionsRange{}, .mB = TilesPositionsRange{}, .mResult = TilesPositionsRange{} },
        {
            .mA = TilesPositionsRange{ .mBegin = TilePosition{ 0, 0 }, .mEnd = TilePosition{ 2, 2 } },
            .mB = TilesPositionsRange{ .mBegin = TilePosition{ 1, 1 }, .mEnd = TilePosition{ 3, 3 } },
            .mResult = TilesPositionsRange{ .mBegin = TilePosition{ 0, 0 }, .mEnd = TilePosition{ 3, 3 } },
        },
        {
            .mA = TilesPositionsRange{ .mBegin = TilePosition{ 0, 0 }, .mEnd = TilePosition{ 1, 1 } },
            .mB = TilesPositionsRange{ .mBegin = TilePosition{ 1, 1 }, .mEnd = TilePosition{ 2, 2 } },
            .mResult = TilesPositionsRange{ .mBegin = TilePosition{ 0, 0 }, .mEnd = TilePosition{ 2, 2 } },
        },
    };

    INSTANTIATE_TEST_SUITE_P(GetUnionParams, DetourNavigatorGetUnionTest, ValuesIn(getUnionParams));
}
