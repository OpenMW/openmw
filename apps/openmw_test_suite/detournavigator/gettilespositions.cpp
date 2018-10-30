#include <components/detournavigator/gettilespositions.hpp>
#include <components/detournavigator/debug.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace
{
    using namespace testing;
    using namespace DetourNavigator;

    struct CollectTilesPositions
    {
        std::vector<TilePosition>& mTilesPositions;

        void operator ()(const TilePosition& value)
        {
            mTilesPositions.push_back(value);
        }
    };

    struct DetourNavigatorGetTilesPositionsTest : Test
    {
        Settings mSettings;
        std::vector<TilePosition> mTilesPositions;
        CollectTilesPositions mCollect {mTilesPositions};

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
        getTilesPositions(osg::Vec3f(2, 2, 0), osg::Vec3f(31, 31, 1), mSettings, mCollect);

        EXPECT_THAT(mTilesPositions, ElementsAre(TilePosition(0, 0)));
    }

    TEST_F(DetourNavigatorGetTilesPositionsTest, for_object_with_x_bounds_in_two_tiles_should_return_two_tiles)
    {
        getTilesPositions(osg::Vec3f(0, 0, 0), osg::Vec3f(32, 31, 1), mSettings, mCollect);

        EXPECT_THAT(mTilesPositions, ElementsAre(TilePosition(0, 0), TilePosition(1, 0)));
    }

    TEST_F(DetourNavigatorGetTilesPositionsTest, for_object_with_y_bounds_in_two_tiles_should_return_two_tiles)
    {
        getTilesPositions(osg::Vec3f(0, 0, 0), osg::Vec3f(31, 32, 1), mSettings, mCollect);

        EXPECT_THAT(mTilesPositions, ElementsAre(TilePosition(0, 0), TilePosition(0, 1)));
    }

    TEST_F(DetourNavigatorGetTilesPositionsTest, tiling_works_only_for_x_and_y_coordinates)
    {
        getTilesPositions(osg::Vec3f(0, 0, 0), osg::Vec3f(31, 31, 32), mSettings, mCollect);

        EXPECT_THAT(mTilesPositions, ElementsAre(TilePosition(0, 0)));
    }

    TEST_F(DetourNavigatorGetTilesPositionsTest, tiling_should_work_with_negative_coordinates)
    {
        getTilesPositions(osg::Vec3f(-31, -31, 0), osg::Vec3f(31, 31, 1), mSettings, mCollect);

        EXPECT_THAT(mTilesPositions, ElementsAre(
            TilePosition(-1, -1),
            TilePosition(-1, 0),
            TilePosition(0, -1),
            TilePosition(0, 0)
        ));
    }

    TEST_F(DetourNavigatorGetTilesPositionsTest, border_size_should_extend_tile_bounds)
    {
        mSettings.mBorderSize = 1;

        getTilesPositions(osg::Vec3f(0, 0, 0), osg::Vec3f(31.5, 31.5, 1), mSettings, mCollect);

        EXPECT_THAT(mTilesPositions, ElementsAre(
            TilePosition(-1, -1),
            TilePosition(-1, 0),
            TilePosition(-1, 1),
            TilePosition(0, -1),
            TilePosition(0, 0),
            TilePosition(0, 1),
            TilePosition(1, -1),
            TilePosition(1, 0),
            TilePosition(1, 1)
        ));
    }

    TEST_F(DetourNavigatorGetTilesPositionsTest, should_apply_recast_scale_factor)
    {
        mSettings.mRecastScaleFactor = 0.5;

        getTilesPositions(osg::Vec3f(0, 0, 0), osg::Vec3f(32, 32, 1), mSettings, mCollect);

        EXPECT_THAT(mTilesPositions, ElementsAre(TilePosition(0, 0)));
    }
}
