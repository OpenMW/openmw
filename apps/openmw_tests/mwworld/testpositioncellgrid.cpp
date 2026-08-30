#include <apps/openmw/mwworld/positioncellgrid.hpp>

#include <components/esm/util.hpp>
#include <components/esm3/loadcell.hpp>

#include <gtest/gtest.h>

namespace MWWorld
{
    namespace
    {
        const osg::Vec4i grid(-1, -1, 2, 2);
        const osg::Vec3f player(100.f, 200.f, 300.f);

        TEST(TerrainPreloadTest, KeepsNearbyCamera)
        {
            const osg::Vec3f camera = player + osg::Vec3f(0.f, 1.f - terrainPreloadMergeDistance, 0.f);
            const std::vector<PositionCellGrid> positions
                = terrainPreloadPositions(player, player, camera, true, grid, ESM::Cell::sDefaultWorldspaceId);
            ASSERT_EQ(positions.size(), 1u);
            EXPECT_EQ(positions[0].mPosition, player);
            EXPECT_EQ(positions[0].mCellBounds, grid);
        }

        TEST(TerrainPreloadTest, IgnoresAttachedCamera)
        {
            const osg::Vec3f camera = player + osg::Vec3f(0.f, 0.f, terrainPreloadMergeDistance * 4.f);
            const std::vector<PositionCellGrid> positions
                = terrainPreloadPositions(player, player, camera, false, grid, ESM::Cell::sDefaultWorldspaceId);
            ASSERT_EQ(positions.size(), 1u);
            EXPECT_EQ(positions[0].mPosition, player);
        }

        TEST(TerrainPreloadTest, AddsDetachedCamera)
        {
            const osg::Vec3f camera = player + osg::Vec3f(0.f, 0.f, terrainPreloadMergeDistance * 4.f);
            const std::vector<PositionCellGrid> positions
                = terrainPreloadPositions(player, player, camera, true, grid, ESM::Cell::sDefaultWorldspaceId);
            ASSERT_EQ(positions.size(), 2u);
            EXPECT_EQ(positions[0].mPosition, player);
            EXPECT_EQ(positions[1].mCellBounds, grid);
        }

        TEST(TerrainPreloadTest, SnapsDetachedCamera)
        {
            const osg::Vec3f first = player + osg::Vec3f(0.f, 0.f, terrainPreloadMergeDistance * 4.f);
            const osg::Vec3f second
                = first + osg::Vec3f(terrainPreloadMergeDistance / 8.f, terrainPreloadMergeDistance / 8.f, 0.f);
            const std::vector<PositionCellGrid> one
                = terrainPreloadPositions(player, player, first, true, grid, ESM::Cell::sDefaultWorldspaceId);
            const std::vector<PositionCellGrid> two
                = terrainPreloadPositions(player, player, second, true, grid, ESM::Cell::sDefaultWorldspaceId);
            ASSERT_EQ(one.size(), 2u);
            ASSERT_EQ(two.size(), 2u);
            // Small orbits keep the same preload anchor.
            EXPECT_EQ(one[1].mPosition, two[1].mPosition);
        }

        TEST(TerrainPreloadTest, IgnoresDistantCamera)
        {
            const float cellSize = static_cast<float>(ESM::getCellSize(ESM::Cell::sDefaultWorldspaceId));
            const osg::Vec3f camera = player + osg::Vec3f(cellSize, 0.f, 0.f);
            const std::vector<PositionCellGrid> positions
                = terrainPreloadPositions(player, player, camera, true, grid, ESM::Cell::sDefaultWorldspaceId);
            ASSERT_EQ(positions.size(), 1u);
            EXPECT_EQ(positions[0].mPosition, player);
        }
    }
}
