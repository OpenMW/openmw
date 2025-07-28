#ifndef OPENMW_COMPONENTS_ESMTERRAIN_GRIDSAMPLING_H
#define OPENMW_COMPONENTS_ESMTERRAIN_GRIDSAMPLING_H

#include <components/misc/mathutil.hpp>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

namespace ESMTerrain
{
    inline std::pair<std::size_t, std::size_t> toCellAndLocal(
        std::size_t begin, std::size_t global, std::size_t cellSize)
    {
        // NOLINTBEGIN(clang-analyzer-core.UndefinedBinaryOperatorResult)
        std::size_t cell = global / (cellSize - 1);
        // NOLINTEND(clang-analyzer-core.UndefinedBinaryOperatorResult)
        std::size_t local = global & (cellSize - 2);
        if (global != begin && local == 0)
        {
            --cell;
            local = cellSize - 1;
        }
        return { cell, local };
    }

    template <class F>
    void sampleGrid(
        std::size_t sampleSize, std::size_t beginX, std::size_t beginY, std::size_t endX, std::size_t endY, F&& f)
    {
        std::size_t vertY = 0;
        for (std::size_t y = beginY; y < endY; y += sampleSize)
        {
            std::size_t vertX = 0;
            for (std::size_t x = beginX; x < endX; x += sampleSize)
                f(x, y, vertX++, vertY);
            ++vertY;
        }
    }

    struct CellSample
    {
        int mCellX;
        int mCellY;
        std::size_t mSrcRow;
        std::size_t mSrcCol;
        std::size_t mDstRow;
        std::size_t mDstCol;
    };

    template <class F>
    void sampleCellGridSimple(std::size_t cellSize, std::size_t sampleSize, std::size_t beginX, std::size_t beginY,
        std::size_t endX, std::size_t endY, F&& f)
    {
        assert(cellSize > 1);
        assert(Misc::isPowerOfTwo(cellSize - 1));
        assert(sampleSize != 0);

        sampleGrid(sampleSize, beginX, beginY, endX, endY,
            [&](std::size_t globalX, std::size_t globalY, std::size_t vertX, std::size_t vertY) {
                const auto [cellX, x] = toCellAndLocal(beginX, globalX, cellSize);
                const auto [cellY, y] = toCellAndLocal(beginY, globalY, cellSize);
                f(cellX, cellY, x, y, vertX, vertY);
            });
    }

    template <class F>
    void sampleCellGrid(std::size_t cellSize, std::size_t sampleSize, std::size_t beginX, std::size_t beginY,
        std::size_t distance, F&& f)
    {
        if (cellSize < 2 || !Misc::isPowerOfTwo(cellSize - 1))
            throw std::invalid_argument("Invalid cell size for cell grid sampling: " + std::to_string(cellSize));

        if (sampleSize == 0 || !Misc::isPowerOfTwo(sampleSize))
            throw std::invalid_argument("Invalid sample size for cell grid sampling: " + std::to_string(sampleSize));

        if (distance < 2 || !Misc::isPowerOfTwo(distance - 1))
            throw std::invalid_argument("Invalid count for cell grid sampling: " + std::to_string(distance));

        const std::size_t endX = beginX + distance;
        const std::size_t endY = beginY + distance;

        if (distance < cellSize || sampleSize > cellSize - 1)
            return sampleCellGridSimple(cellSize, sampleSize, beginX, beginY, endX, endY, f);

        const std::size_t beginCellX = beginX / (cellSize - 1);
        const std::size_t beginCellY = beginY / (cellSize - 1);
        const std::size_t endCellX = endX / (cellSize - 1);
        const std::size_t endCellY = endY / (cellSize - 1);

        std::size_t baseVertY = 0;

        for (std::size_t cellY = beginCellY; cellY < endCellY; ++cellY)
        {
            const std::size_t offsetY = cellY * (cellSize - 1);
            const std::size_t globalBeginY = offsetY <= beginY ? beginY : offsetY + sampleSize;
            const std::size_t globalEndY = endY <= offsetY + cellSize ? endY : offsetY + cellSize;

            assert(globalBeginY < globalEndY);

            std::size_t baseVertX = 0;
            std::size_t vertY = baseVertY;

            for (std::size_t cellX = beginCellX; cellX < endCellX; ++cellX)
            {
                const std::size_t offsetX = cellX * (cellSize - 1);
                const std::size_t globalBeginX = offsetX <= beginX ? beginX : offsetX + sampleSize;
                const std::size_t globalEndX = endX <= offsetX + cellSize ? endX : offsetX + cellSize;

                assert(globalBeginX < globalEndX);

                vertY = baseVertY;
                std::size_t vertX = baseVertX;

                sampleGrid(sampleSize, globalBeginX, globalBeginY, globalEndX, globalEndY,
                    [&](std::size_t globalX, std::size_t globalY, std::size_t localVertX, std::size_t localVertY) {
                        vertX = baseVertX + localVertX;
                        vertY = baseVertY + localVertY;
                        f(cellX, cellY, globalX - offsetX, globalY - offsetY, vertX, vertY);
                    });

                baseVertX = vertX + 1;
            }

            baseVertY = vertY + 1;
        }
    }

    inline int getBlendmapSize(float size, int textureSize)
    {
        return static_cast<int>(textureSize * size) + 1;
    }

    inline std::pair<std::size_t, std::size_t> getBlendmapLocalRange(
        int cell, int minCell, int textureSize, int min, int max)
    {
        const int cellMin = (cell - minCell) * textureSize;
        const int cellMax = cellMin + textureSize - 1;
        const std::size_t begin = min > cellMin ? min - cellMin : 0;
        const std::size_t end = cellMax < max ? textureSize : max - cellMin + 1;
        assert(begin < end);
        assert(static_cast<int>(end) <= textureSize);
        return { begin, end };
    }

    template <class F>
    void sampleBlendmaps(float size, float minX, float minY, int textureSize, F&& f)
    {
        if (size <= 0)
            throw std::invalid_argument("Invalid size for blendmap sampling: " + std::to_string(size));

        if (textureSize <= 0)
            throw std::invalid_argument("Invalid texture size for blendmap sampling: " + std::to_string(textureSize));

        int minCellX = static_cast<int>(std::floor(minX));
        const int minCellY = static_cast<int>(std::floor(minY));
        int minRow = static_cast<int>((minX - minCellX) * (textureSize + 1)) - 1;
        const int minCol = static_cast<int>((minY - minCellY) * (textureSize + 1));

        if (minRow < 0)
        {
            --minCellX;
            minRow += textureSize;
        }

        const int maxRow = minRow + static_cast<int>(textureSize * size);
        const int maxCol = minCol + static_cast<int>(textureSize * size);
        const int maxCellX = minCellX + maxRow / textureSize;
        const int maxCellY = minCellY + maxCol / textureSize;

        std::size_t baseDstCol = 0;
        for (int cellY = minCellY; cellY <= maxCellY; ++cellY)
        {
            std::size_t baseDstRow = 0;

            const auto [localBeginCol, localEndCol]
                = getBlendmapLocalRange(cellY, minCellY, textureSize, minCol, maxCol);
            const std::size_t colCount = localEndCol - localBeginCol;

            for (int cellX = minCellX; cellX <= maxCellX; ++cellX)
            {
                const auto [localBeginRow, localEndRow]
                    = getBlendmapLocalRange(cellX, minCellX, textureSize, minRow, maxRow);
                const std::size_t rowCount = localEndRow - localBeginRow;

                for (std::size_t col = 0; col < colCount; ++col)
                    for (std::size_t row = 0; row < rowCount; ++row)
                        f(CellSample{
                            .mCellX = cellX,
                            .mCellY = cellY,
                            .mSrcRow = localBeginRow + row,
                            .mSrcCol = localBeginCol + col,
                            .mDstRow = baseDstRow + row,
                            .mDstCol = baseDstCol + col,
                        });

                baseDstRow += rowCount;
            }

            baseDstCol += colCount;
        }
    }
}

#endif
