#include "operators.hpp"

#include <components/detournavigator/settingsutils.hpp>

#include <gtest/gtest.h>

namespace
{
    using namespace testing;
    using namespace DetourNavigator;

    struct DetourNavigatorGetTilePositionTest : Test
    {
        Settings mSettings;

        DetourNavigatorGetTilePositionTest()
        {
            mSettings.mCellSize = 0.5;
            mSettings.mTileSize = 64;
        }
    };

    TEST_F(DetourNavigatorGetTilePositionTest, for_zero_coordinates_should_return_zero_tile_position)
    {
        EXPECT_EQ(getTilePosition(mSettings, osg::Vec3f(0, 0, 0)), TilePosition(0, 0));
    }

    TEST_F(DetourNavigatorGetTilePositionTest, tile_size_should_be_multiplied_by_cell_size)
    {
        EXPECT_EQ(getTilePosition(mSettings, osg::Vec3f(32, 0, 0)), TilePosition(1, 0));
    }

    TEST_F(DetourNavigatorGetTilePositionTest, tile_position_calculates_by_floor)
    {
        EXPECT_EQ(getTilePosition(mSettings, osg::Vec3f(31, 0, 0)), TilePosition(0, 0));
    }

    TEST_F(DetourNavigatorGetTilePositionTest, tile_position_depends_on_x_and_z_coordinates)
    {
        EXPECT_EQ(getTilePosition(mSettings, osg::Vec3f(32, 64, 128)), TilePosition(1, 4));
    }

    TEST_F(DetourNavigatorGetTilePositionTest, tile_position_works_for_negative_coordinates)
    {
        EXPECT_EQ(getTilePosition(mSettings, osg::Vec3f(-31, 0, -32)), TilePosition(-1, -1));
    }

    struct DetourNavigatorMakeTileBoundsTest : Test
    {
        Settings mSettings;

        DetourNavigatorMakeTileBoundsTest()
        {
            mSettings.mCellSize = 0.5;
            mSettings.mTileSize = 64;
        }
    };

    TEST_F(DetourNavigatorMakeTileBoundsTest, tile_bounds_depend_on_tile_size_and_cell_size)
    {
        EXPECT_EQ(makeTileBounds(mSettings, TilePosition(0, 0)), (TileBounds {osg::Vec2f(0, 0), osg::Vec2f(32, 32)}));
    }

    TEST_F(DetourNavigatorMakeTileBoundsTest, tile_bounds_are_multiplied_by_tile_position)
    {
        EXPECT_EQ(makeTileBounds(mSettings, TilePosition(1, 2)), (TileBounds {osg::Vec2f(32, 64), osg::Vec2f(64, 96)}));
    }
}
