#include <components/esmterrain/gridsampling.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ESMTerrain
{
    namespace
    {
        using namespace testing;

        struct Sample
        {
            std::size_t mCellX = 0;
            std::size_t mCellY = 0;
            std::size_t mLocalX = 0;
            std::size_t mLocalY = 0;
            std::size_t mVertexX = 0;
            std::size_t mVertexY = 0;
        };

        auto tie(const Sample& v)
        {
            return std::tie(v.mCellX, v.mCellY, v.mLocalX, v.mLocalY, v.mVertexX, v.mVertexY);
        }

        bool operator==(const Sample& l, const Sample& r)
        {
            return tie(l) == tie(r);
        }

        std::ostream& operator<<(std::ostream& stream, const Sample& v)
        {
            return stream << "Sample{.mCellX = " << v.mCellX << ", .mCellY = " << v.mCellY
                          << ", .mLocalX = " << v.mLocalX << ", .mLocalY = " << v.mLocalY
                          << ", .mVertexX = " << v.mVertexX << ", .mVertexY = " << v.mVertexY << "}";
        }

        struct Collect
        {
            std::vector<Sample>& mSamples;

            void operator()(std::size_t cellX, std::size_t cellY, std::size_t localX, std::size_t localY,
                std::size_t vertexX, std::size_t vertexY)
            {
                mSamples.push_back(Sample{
                    .mCellX = cellX,
                    .mCellY = cellY,
                    .mLocalX = localX,
                    .mLocalY = localY,
                    .mVertexX = vertexX,
                    .mVertexY = vertexY,
                });
            }
        };

        TEST(ESMTerrainSampleCellGrid, doesNotSupportCellSizeLessThanTwo)
        {
            const std::size_t cellSize = 2;
            EXPECT_THROW(sampleCellGrid(cellSize, 0, 0, 0, 0, [](auto...) {}), std::invalid_argument);
        }

        TEST(ESMTerrainSampleCellGrid, doesNotSupportCellSizeMinusOneNotPowerOfTwo)
        {
            const std::size_t cellSize = 4;
            EXPECT_THROW(sampleCellGrid(cellSize, 0, 0, 0, 0, [](auto...) {}), std::invalid_argument);
        }

        TEST(ESMTerrainSampleCellGrid, doesNotSupportZeroSampleSize)
        {
            const std::size_t cellSize = 1;
            const std::size_t sampleSize = 0;
            EXPECT_THROW(sampleCellGrid(cellSize, sampleSize, 0, 0, 0, [](auto...) {}), std::invalid_argument);
        }

        TEST(ESMTerrainSampleCellGrid, doesNotSupportSampleSizeNotPowerOfTwo)
        {
            const std::size_t cellSize = 1;
            const std::size_t sampleSize = 3;
            EXPECT_THROW(sampleCellGrid(cellSize, sampleSize, 0, 0, 0, [](auto...) {}), std::invalid_argument);
        }

        TEST(ESMTerrainSampleCellGrid, doesNotSupportCountLessThanTwo)
        {
            const std::size_t cellSize = 1;
            const std::size_t sampleSize = 1;
            const std::size_t distance = 2;
            EXPECT_THROW(sampleCellGrid(cellSize, sampleSize, 0, 0, distance, [](auto...) {}), std::invalid_argument);
        }

        TEST(ESMTerrainSampleCellGrid, doesNotSupportCountMinusOneNotPowerOfTwo)
        {
            const std::size_t cellSize = 1;
            const std::size_t sampleSize = 1;
            const std::size_t distance = 4;
            EXPECT_THROW(sampleCellGrid(cellSize, sampleSize, 0, 0, distance, [](auto...) {}), std::invalid_argument);
        }

        TEST(ESMTerrainSampleCellGrid, sampleSizeOneShouldProduceNumberOfSamplesEqualToCellSize)
        {
            const std::size_t cellSize = 3;
            const std::size_t sampleSize = 1;
            const std::size_t beginX = 0;
            const std::size_t beginY = 0;
            const std::size_t distance = 3;
            std::vector<Sample> samples;
            sampleCellGrid(cellSize, sampleSize, beginX, beginY, distance, Collect{ samples });
            EXPECT_THAT(samples,
                ElementsAre( //
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 0, .mLocalY = 0, .mVertexX = 0, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 1, .mLocalY = 0, .mVertexX = 1, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 2, .mLocalY = 0, .mVertexX = 2, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 0, .mLocalY = 1, .mVertexX = 0, .mVertexY = 1 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 1, .mLocalY = 1, .mVertexX = 1, .mVertexY = 1 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 2, .mLocalY = 1, .mVertexX = 2, .mVertexY = 1 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 0, .mLocalY = 2, .mVertexX = 0, .mVertexY = 2 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 1, .mLocalY = 2, .mVertexX = 1, .mVertexY = 2 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 2, .mLocalY = 2, .mVertexX = 2, .mVertexY = 2 }));
        }

        TEST(ESMTerrainSampleCellGrid, countShouldLimitScope)
        {
            const std::size_t cellSize = 3;
            const std::size_t sampleSize = 1;
            const std::size_t beginX = 0;
            const std::size_t beginY = 0;
            const std::size_t distance = 2;
            std::vector<Sample> samples;
            sampleCellGrid(cellSize, sampleSize, beginX, beginY, distance, Collect{ samples });
            EXPECT_THAT(samples,
                ElementsAre( //
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 0, .mLocalY = 0, .mVertexX = 0, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 1, .mLocalY = 0, .mVertexX = 1, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 0, .mLocalY = 1, .mVertexX = 0, .mVertexY = 1 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 1, .mLocalY = 1, .mVertexX = 1, .mVertexY = 1 }));
        }

        TEST(ESMTerrainSampleCellGrid, beginXAndCountShouldLimitScope)
        {
            const std::size_t cellSize = 3;
            const std::size_t sampleSize = 1;
            const std::size_t beginX = 1;
            const std::size_t beginY = 0;
            const std::size_t distance = 2;
            std::vector<Sample> samples;
            sampleCellGrid(cellSize, sampleSize, beginX, beginY, distance, Collect{ samples });
            EXPECT_THAT(samples,
                ElementsAre( //
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 1, .mLocalY = 0, .mVertexX = 0, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 2, .mLocalY = 0, .mVertexX = 1, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 1, .mLocalY = 1, .mVertexX = 0, .mVertexY = 1 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 2, .mLocalY = 1, .mVertexX = 1, .mVertexY = 1 }));
        }

        TEST(ESMTerrainSampleCellGrid, beginYAndCountShouldLimitScope)
        {
            const std::size_t cellSize = 3;
            const std::size_t sampleSize = 1;
            const std::size_t beginX = 0;
            const std::size_t beginY = 1;
            const std::size_t distance = 2;
            std::vector<Sample> samples;
            sampleCellGrid(cellSize, sampleSize, beginX, beginY, distance, Collect{ samples });
            EXPECT_THAT(samples,
                ElementsAre( //
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 0, .mLocalY = 1, .mVertexX = 0, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 1, .mLocalY = 1, .mVertexX = 1, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 0, .mLocalY = 2, .mVertexX = 0, .mVertexY = 1 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 1, .mLocalY = 2, .mVertexX = 1, .mVertexY = 1 }));
        }

        TEST(ESMTerrainSampleCellGrid, beginAndCountShouldLimitScope)
        {
            const std::size_t cellSize = 3;
            const std::size_t sampleSize = 1;
            const std::size_t beginX = 1;
            const std::size_t beginY = 1;
            const std::size_t distance = 2;
            std::vector<Sample> samples;
            sampleCellGrid(cellSize, sampleSize, beginX, beginY, distance, Collect{ samples });
            EXPECT_THAT(samples,
                ElementsAre( //
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 1, .mLocalY = 1, .mVertexX = 0, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 2, .mLocalY = 1, .mVertexX = 1, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 1, .mLocalY = 2, .mVertexX = 0, .mVertexY = 1 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 2, .mLocalY = 2, .mVertexX = 1, .mVertexY = 1 }));
        }

        TEST(ESMTerrainSampleCellGrid, beginAndCountShouldLimitScopeInTheMiddleOfCell)
        {
            const std::size_t cellSize = 5;
            const std::size_t sampleSize = 1;
            const std::size_t beginX = 1;
            const std::size_t beginY = 1;
            const std::size_t distance = 2;
            std::vector<Sample> samples;
            sampleCellGrid(cellSize, sampleSize, beginX, beginY, distance, Collect{ samples });
            EXPECT_THAT(samples,
                ElementsAre( //
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 1, .mLocalY = 1, .mVertexX = 0, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 2, .mLocalY = 1, .mVertexX = 1, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 1, .mLocalY = 2, .mVertexX = 0, .mVertexY = 1 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 2, .mLocalY = 2, .mVertexX = 1, .mVertexY = 1 }));
        }

        TEST(ESMTerrainSampleCellGrid, beginXWithCountLessThanCellSizeShouldLimitScopeAcrossCellBorder)
        {
            const std::size_t cellSize = 5;
            const std::size_t sampleSize = 1;
            const std::size_t beginX = 3;
            const std::size_t beginY = 0;
            const std::size_t distance = 3;
            std::vector<Sample> samples;
            sampleCellGrid(cellSize, sampleSize, beginX, beginY, distance, Collect{ samples });
            EXPECT_THAT(samples,
                ElementsAre( //
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 3, .mLocalY = 0, .mVertexX = 0, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 4, .mLocalY = 0, .mVertexX = 1, .mVertexY = 0 },
                    Sample{ .mCellX = 1, .mCellY = 0, .mLocalX = 1, .mLocalY = 0, .mVertexX = 2, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 3, .mLocalY = 1, .mVertexX = 0, .mVertexY = 1 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 4, .mLocalY = 1, .mVertexX = 1, .mVertexY = 1 },
                    Sample{ .mCellX = 1, .mCellY = 0, .mLocalX = 1, .mLocalY = 1, .mVertexX = 2, .mVertexY = 1 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 3, .mLocalY = 2, .mVertexX = 0, .mVertexY = 2 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 4, .mLocalY = 2, .mVertexX = 1, .mVertexY = 2 },
                    Sample{ .mCellX = 1, .mCellY = 0, .mLocalX = 1, .mLocalY = 2, .mVertexX = 2, .mVertexY = 2 }));
        }

        TEST(ESMTerrainSampleCellGrid, beginXWithCountEqualToCellSizeShouldLimitScopeAcrossCellBorder)
        {
            const std::size_t cellSize = 3;
            const std::size_t sampleSize = 1;
            const std::size_t beginX = 1;
            const std::size_t beginY = 0;
            const std::size_t distance = 3;
            std::vector<Sample> samples;
            sampleCellGrid(cellSize, sampleSize, beginX, beginY, distance, Collect{ samples });
            EXPECT_THAT(samples,
                ElementsAre( //
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 1, .mLocalY = 0, .mVertexX = 0, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 2, .mLocalY = 0, .mVertexX = 1, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 1, .mLocalY = 1, .mVertexX = 0, .mVertexY = 1 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 2, .mLocalY = 1, .mVertexX = 1, .mVertexY = 1 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 1, .mLocalY = 2, .mVertexX = 0, .mVertexY = 2 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 2, .mLocalY = 2, .mVertexX = 1, .mVertexY = 2 },
                    Sample{ .mCellX = 1, .mCellY = 0, .mLocalX = 1, .mLocalY = 0, .mVertexX = 2, .mVertexY = 0 },
                    Sample{ .mCellX = 1, .mCellY = 0, .mLocalX = 1, .mLocalY = 1, .mVertexX = 2, .mVertexY = 1 },
                    Sample{ .mCellX = 1, .mCellY = 0, .mLocalX = 1, .mLocalY = 2, .mVertexX = 2, .mVertexY = 2 }));
        }

        TEST(ESMTerrainSampleCellGrid, beginXWithCountGreaterThanCellSizeShouldLimitScopeAcrossCellBorder)
        {
            const std::size_t cellSize = 3;
            const std::size_t sampleSize = 1;
            const std::size_t beginX = 1;
            const std::size_t beginY = 0;
            const std::size_t distance = 5;
            std::vector<Sample> samples;
            sampleCellGrid(cellSize, sampleSize, beginX, beginY, distance, Collect{ samples });
            EXPECT_THAT(samples,
                ElementsAre( //
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 1, .mLocalY = 0, .mVertexX = 0, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 2, .mLocalY = 0, .mVertexX = 1, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 1, .mLocalY = 1, .mVertexX = 0, .mVertexY = 1 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 2, .mLocalY = 1, .mVertexX = 1, .mVertexY = 1 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 1, .mLocalY = 2, .mVertexX = 0, .mVertexY = 2 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 2, .mLocalY = 2, .mVertexX = 1, .mVertexY = 2 },
                    Sample{ .mCellX = 1, .mCellY = 0, .mLocalX = 1, .mLocalY = 0, .mVertexX = 2, .mVertexY = 0 },
                    Sample{ .mCellX = 1, .mCellY = 0, .mLocalX = 2, .mLocalY = 0, .mVertexX = 3, .mVertexY = 0 },
                    Sample{ .mCellX = 1, .mCellY = 0, .mLocalX = 1, .mLocalY = 1, .mVertexX = 2, .mVertexY = 1 },
                    Sample{ .mCellX = 1, .mCellY = 0, .mLocalX = 2, .mLocalY = 1, .mVertexX = 3, .mVertexY = 1 },
                    Sample{ .mCellX = 1, .mCellY = 0, .mLocalX = 1, .mLocalY = 2, .mVertexX = 2, .mVertexY = 2 },
                    Sample{ .mCellX = 1, .mCellY = 0, .mLocalX = 2, .mLocalY = 2, .mVertexX = 3, .mVertexY = 2 },
                    Sample{ .mCellX = 2, .mCellY = 0, .mLocalX = 1, .mLocalY = 0, .mVertexX = 4, .mVertexY = 0 },
                    Sample{ .mCellX = 2, .mCellY = 0, .mLocalX = 1, .mLocalY = 1, .mVertexX = 4, .mVertexY = 1 },
                    Sample{ .mCellX = 2, .mCellY = 0, .mLocalX = 1, .mLocalY = 2, .mVertexX = 4, .mVertexY = 2 },
                    Sample{ .mCellX = 0, .mCellY = 1, .mLocalX = 1, .mLocalY = 1, .mVertexX = 0, .mVertexY = 3 },
                    Sample{ .mCellX = 0, .mCellY = 1, .mLocalX = 2, .mLocalY = 1, .mVertexX = 1, .mVertexY = 3 },
                    Sample{ .mCellX = 0, .mCellY = 1, .mLocalX = 1, .mLocalY = 2, .mVertexX = 0, .mVertexY = 4 },
                    Sample{ .mCellX = 0, .mCellY = 1, .mLocalX = 2, .mLocalY = 2, .mVertexX = 1, .mVertexY = 4 },
                    Sample{ .mCellX = 1, .mCellY = 1, .mLocalX = 1, .mLocalY = 1, .mVertexX = 2, .mVertexY = 3 },
                    Sample{ .mCellX = 1, .mCellY = 1, .mLocalX = 2, .mLocalY = 1, .mVertexX = 3, .mVertexY = 3 },
                    Sample{ .mCellX = 1, .mCellY = 1, .mLocalX = 1, .mLocalY = 2, .mVertexX = 2, .mVertexY = 4 },
                    Sample{ .mCellX = 1, .mCellY = 1, .mLocalX = 2, .mLocalY = 2, .mVertexX = 3, .mVertexY = 4 },
                    Sample{ .mCellX = 2, .mCellY = 1, .mLocalX = 1, .mLocalY = 1, .mVertexX = 4, .mVertexY = 3 },
                    Sample{ .mCellX = 2, .mCellY = 1, .mLocalX = 1, .mLocalY = 2, .mVertexX = 4, .mVertexY = 4 }));
        }

        TEST(ESMTerrainSampleCellGrid, sampleSizeGreaterThanOneShouldSkipPoints)
        {
            const std::size_t cellSize = 3;
            const std::size_t sampleSize = 2;
            const std::size_t beginX = 0;
            const std::size_t beginY = 0;
            const std::size_t distance = 3;
            std::vector<Sample> samples;
            sampleCellGrid(cellSize, sampleSize, beginX, beginY, distance, Collect{ samples });
            EXPECT_THAT(samples,
                ElementsAre( //
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 0, .mLocalY = 0, .mVertexX = 0, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 2, .mLocalY = 0, .mVertexX = 1, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 0, .mLocalY = 2, .mVertexX = 0, .mVertexY = 1 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 2, .mLocalY = 2, .mVertexX = 1, .mVertexY = 1 }));
        }

        TEST(ESMTerrainSampleCellGrid, shouldGroupByCell)
        {
            const std::size_t cellSize = 3;
            const std::size_t sampleSize = 2;
            const std::size_t beginX = 0;
            const std::size_t beginY = 0;
            const std::size_t distance = 5;
            std::vector<Sample> samples;
            sampleCellGrid(cellSize, sampleSize, beginX, beginY, distance, Collect{ samples });
            EXPECT_THAT(samples,
                ElementsAre( //
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 0, .mLocalY = 0, .mVertexX = 0, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 2, .mLocalY = 0, .mVertexX = 1, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 0, .mLocalY = 2, .mVertexX = 0, .mVertexY = 1 },
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 2, .mLocalY = 2, .mVertexX = 1, .mVertexY = 1 },
                    Sample{ .mCellX = 1, .mCellY = 0, .mLocalX = 2, .mLocalY = 0, .mVertexX = 2, .mVertexY = 0 },
                    Sample{ .mCellX = 1, .mCellY = 0, .mLocalX = 2, .mLocalY = 2, .mVertexX = 2, .mVertexY = 1 },
                    Sample{ .mCellX = 0, .mCellY = 1, .mLocalX = 0, .mLocalY = 2, .mVertexX = 0, .mVertexY = 2 },
                    Sample{ .mCellX = 0, .mCellY = 1, .mLocalX = 2, .mLocalY = 2, .mVertexX = 1, .mVertexY = 2 },
                    Sample{ .mCellX = 1, .mCellY = 1, .mLocalX = 2, .mLocalY = 2, .mVertexX = 2, .mVertexY = 2 }));
        }

        TEST(ESMTerrainSampleCellGrid, sampleSizeGreaterThanCellSizeShouldPickSinglePointPerCell)
        {
            const std::size_t cellSize = 3;
            const std::size_t sampleSize = 4;
            const std::size_t beginX = 0;
            const std::size_t beginY = 0;
            const std::size_t distance = 9;
            std::vector<Sample> samples;
            sampleCellGrid(cellSize, sampleSize, beginX, beginY, distance, Collect{ samples });
            EXPECT_THAT(samples,
                ElementsAre( //
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 0, .mLocalY = 0, .mVertexX = 0, .mVertexY = 0 },
                    Sample{ .mCellX = 1, .mCellY = 0, .mLocalX = 2, .mLocalY = 0, .mVertexX = 1, .mVertexY = 0 },
                    Sample{ .mCellX = 3, .mCellY = 0, .mLocalX = 2, .mLocalY = 0, .mVertexX = 2, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 1, .mLocalX = 0, .mLocalY = 2, .mVertexX = 0, .mVertexY = 1 },
                    Sample{ .mCellX = 1, .mCellY = 1, .mLocalX = 2, .mLocalY = 2, .mVertexX = 1, .mVertexY = 1 },
                    Sample{ .mCellX = 3, .mCellY = 1, .mLocalX = 2, .mLocalY = 2, .mVertexX = 2, .mVertexY = 1 },
                    Sample{ .mCellX = 0, .mCellY = 3, .mLocalX = 0, .mLocalY = 2, .mVertexX = 0, .mVertexY = 2 },
                    Sample{ .mCellX = 1, .mCellY = 3, .mLocalX = 2, .mLocalY = 2, .mVertexX = 1, .mVertexY = 2 },
                    Sample{ .mCellX = 3, .mCellY = 3, .mLocalX = 2, .mLocalY = 2, .mVertexX = 2, .mVertexY = 2 }));
        }

        TEST(ESMTerrainSampleCellGrid, sampleSizeGreaterThan2CellSizeShouldSkipCells)
        {
            const std::size_t cellSize = 3;
            const std::size_t sampleSize = 8;
            const std::size_t beginX = 0;
            const std::size_t beginY = 0;
            const std::size_t distance = 9;
            std::vector<Sample> samples;
            sampleCellGrid(cellSize, sampleSize, beginX, beginY, distance, Collect{ samples });
            EXPECT_THAT(samples,
                ElementsAre( //
                    Sample{ .mCellX = 0, .mCellY = 0, .mLocalX = 0, .mLocalY = 0, .mVertexX = 0, .mVertexY = 0 },
                    Sample{ .mCellX = 3, .mCellY = 0, .mLocalX = 2, .mLocalY = 0, .mVertexX = 1, .mVertexY = 0 },
                    Sample{ .mCellX = 0, .mCellY = 3, .mLocalX = 0, .mLocalY = 2, .mVertexX = 0, .mVertexY = 1 },
                    Sample{ .mCellX = 3, .mCellY = 3, .mLocalX = 2, .mLocalY = 2, .mVertexX = 1, .mVertexY = 1 }));
        }

        auto tie(const CellSample& v)
        {
            return std::tie(v.mCellX, v.mCellY, v.mSrcRow, v.mSrcCol, v.mDstRow, v.mDstCol);
        }
    }

    static bool operator==(const CellSample& l, const CellSample& r)
    {
        return tie(l) == tie(r);
    }

    static std::ostream& operator<<(std::ostream& stream, const CellSample& v)
    {
        return stream << "CellSample{.mCellX = " << v.mCellX << ", .mCellY = " << v.mCellY
                      << ", .mSrcRow = " << v.mSrcRow << ", .mSrcCol = " << v.mSrcCol << ", .mDstRow = " << v.mDstRow
                      << ", .mDstCol = " << v.mDstCol << "}";
    }

    namespace
    {
        struct CollectCellSamples
        {
            std::vector<CellSample>& mSamples;

            void operator()(const CellSample& value) { mSamples.push_back(value); }
        };

        TEST(ESMTerrainSampleBlendmaps, doesNotSupportNotPositiveSize)
        {
            const float size = 0;
            EXPECT_THROW(sampleBlendmaps(size, 0, 0, 0, [](auto...) {}), std::invalid_argument);
        }

        TEST(ESMTerrainSampleBlendmaps, doesNotSupportNotPositiveTextureSize)
        {
            const float size = 1;
            const int textureSize = 0;
            EXPECT_THROW(sampleBlendmaps(size, 0, 0, textureSize, [](auto...) {}), std::invalid_argument);
        }

        TEST(ESMTerrainSampleBlendmaps, shouldDecrementBeginRow)
        {
            const float size = 0.125f;
            const float minX = 0.125f;
            const float minY = 0.125f;
            const int textureSize = 8;
            std::vector<CellSample> samples;
            sampleBlendmaps(size, minX, minY, textureSize, CollectCellSamples{ samples });
            EXPECT_THAT(samples,
                ElementsAre( //
                    CellSample{ .mCellX = 0, .mCellY = 0, .mSrcRow = 0, .mSrcCol = 1, .mDstRow = 0, .mDstCol = 0 },
                    CellSample{ .mCellX = 0, .mCellY = 0, .mSrcRow = 1, .mSrcCol = 1, .mDstRow = 1, .mDstCol = 0 },
                    CellSample{ .mCellX = 0, .mCellY = 0, .mSrcRow = 0, .mSrcCol = 2, .mDstRow = 0, .mDstCol = 1 },
                    CellSample{ .mCellX = 0, .mCellY = 0, .mSrcRow = 1, .mSrcCol = 2, .mDstRow = 1, .mDstCol = 1 }));
        }

        TEST(ESMTerrainSampleBlendmaps, shouldDecrementBeginRowOverCellBorder)
        {
            const float size = 0.125f;
            const float minX = 0;
            const float minY = 0;
            const int textureSize = 8;
            std::vector<CellSample> samples;
            sampleBlendmaps(size, minX, minY, textureSize, CollectCellSamples{ samples });
            EXPECT_THAT(samples,
                ElementsAre( //
                    CellSample{ .mCellX = -1, .mCellY = 0, .mSrcRow = 7, .mSrcCol = 0, .mDstRow = 0, .mDstCol = 0 },
                    CellSample{ .mCellX = -1, .mCellY = 0, .mSrcRow = 7, .mSrcCol = 1, .mDstRow = 0, .mDstCol = 1 },
                    CellSample{ .mCellX = 0, .mCellY = 0, .mSrcRow = 0, .mSrcCol = 0, .mDstRow = 1, .mDstCol = 0 },
                    CellSample{ .mCellX = 0, .mCellY = 0, .mSrcRow = 0, .mSrcCol = 1, .mDstRow = 1, .mDstCol = 1 }));
        }

        TEST(ESMTerrainSampleBlendmaps, shouldSupportNegativeCoordinates)
        {
            const float size = 0.125f;
            const float minX = -0.5f;
            const float minY = -0.5f;
            const int textureSize = 8;
            std::vector<CellSample> samples;
            sampleBlendmaps(size, minX, minY, textureSize, CollectCellSamples{ samples });
            EXPECT_THAT(samples,
                ElementsAre( //
                    CellSample{ .mCellX = -1, .mCellY = -1, .mSrcRow = 3, .mSrcCol = 4, .mDstRow = 0, .mDstCol = 0 },
                    CellSample{ .mCellX = -1, .mCellY = -1, .mSrcRow = 4, .mSrcCol = 4, .mDstRow = 1, .mDstCol = 0 },
                    CellSample{ .mCellX = -1, .mCellY = -1, .mSrcRow = 3, .mSrcCol = 5, .mDstRow = 0, .mDstCol = 1 },
                    CellSample{ .mCellX = -1, .mCellY = -1, .mSrcRow = 4, .mSrcCol = 5, .mDstRow = 1, .mDstCol = 1 }));
        }

        TEST(ESMTerrainSampleBlendmaps, shouldCoverMultipleCells)
        {
            const float size = 2;
            const float minX = -1.5f;
            const float minY = -1.5f;
            const int textureSize = 2;
            std::vector<CellSample> samples;
            sampleBlendmaps(size, minX, minY, textureSize, CollectCellSamples{ samples });
            EXPECT_THAT(samples,
                ElementsAre( //
                    CellSample{ .mCellX = -2, .mCellY = -2, .mSrcRow = 0, .mSrcCol = 1, .mDstRow = 0, .mDstCol = 0 },
                    CellSample{ .mCellX = -2, .mCellY = -2, .mSrcRow = 1, .mSrcCol = 1, .mDstRow = 1, .mDstCol = 0 },
                    CellSample{ .mCellX = -1, .mCellY = -2, .mSrcRow = 0, .mSrcCol = 1, .mDstRow = 2, .mDstCol = 0 },
                    CellSample{ .mCellX = -1, .mCellY = -2, .mSrcRow = 1, .mSrcCol = 1, .mDstRow = 3, .mDstCol = 0 },
                    CellSample{ .mCellX = 0, .mCellY = -2, .mSrcRow = 0, .mSrcCol = 1, .mDstRow = 4, .mDstCol = 0 },
                    CellSample{ .mCellX = -2, .mCellY = -1, .mSrcRow = 0, .mSrcCol = 0, .mDstRow = 0, .mDstCol = 1 },
                    CellSample{ .mCellX = -2, .mCellY = -1, .mSrcRow = 1, .mSrcCol = 0, .mDstRow = 1, .mDstCol = 1 },
                    CellSample{ .mCellX = -2, .mCellY = -1, .mSrcRow = 0, .mSrcCol = 1, .mDstRow = 0, .mDstCol = 2 },
                    CellSample{ .mCellX = -2, .mCellY = -1, .mSrcRow = 1, .mSrcCol = 1, .mDstRow = 1, .mDstCol = 2 },
                    CellSample{ .mCellX = -1, .mCellY = -1, .mSrcRow = 0, .mSrcCol = 0, .mDstRow = 2, .mDstCol = 1 },
                    CellSample{ .mCellX = -1, .mCellY = -1, .mSrcRow = 1, .mSrcCol = 0, .mDstRow = 3, .mDstCol = 1 },
                    CellSample{ .mCellX = -1, .mCellY = -1, .mSrcRow = 0, .mSrcCol = 1, .mDstRow = 2, .mDstCol = 2 },
                    CellSample{ .mCellX = -1, .mCellY = -1, .mSrcRow = 1, .mSrcCol = 1, .mDstRow = 3, .mDstCol = 2 },
                    CellSample{ .mCellX = 0, .mCellY = -1, .mSrcRow = 0, .mSrcCol = 0, .mDstRow = 4, .mDstCol = 1 },
                    CellSample{ .mCellX = 0, .mCellY = -1, .mSrcRow = 0, .mSrcCol = 1, .mDstRow = 4, .mDstCol = 2 },
                    CellSample{ .mCellX = -2, .mCellY = 0, .mSrcRow = 0, .mSrcCol = 0, .mDstRow = 0, .mDstCol = 3 },
                    CellSample{ .mCellX = -2, .mCellY = 0, .mSrcRow = 1, .mSrcCol = 0, .mDstRow = 1, .mDstCol = 3 },
                    CellSample{ .mCellX = -2, .mCellY = 0, .mSrcRow = 0, .mSrcCol = 1, .mDstRow = 0, .mDstCol = 4 },
                    CellSample{ .mCellX = -2, .mCellY = 0, .mSrcRow = 1, .mSrcCol = 1, .mDstRow = 1, .mDstCol = 4 },
                    CellSample{ .mCellX = -1, .mCellY = 0, .mSrcRow = 0, .mSrcCol = 0, .mDstRow = 2, .mDstCol = 3 },
                    CellSample{ .mCellX = -1, .mCellY = 0, .mSrcRow = 1, .mSrcCol = 0, .mDstRow = 3, .mDstCol = 3 },
                    CellSample{ .mCellX = -1, .mCellY = 0, .mSrcRow = 0, .mSrcCol = 1, .mDstRow = 2, .mDstCol = 4 },
                    CellSample{ .mCellX = -1, .mCellY = 0, .mSrcRow = 1, .mSrcCol = 1, .mDstRow = 3, .mDstCol = 4 },
                    CellSample{ .mCellX = 0, .mCellY = 0, .mSrcRow = 0, .mSrcCol = 0, .mDstRow = 4, .mDstCol = 3 },
                    CellSample{ .mCellX = 0, .mCellY = 0, .mSrcRow = 0, .mSrcCol = 1, .mDstRow = 4, .mDstCol = 4 }));
        }
    }
}
