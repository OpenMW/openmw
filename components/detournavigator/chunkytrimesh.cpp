#include "chunkytrimesh.hpp"
#include "exceptions.hpp"

#include <osg/Vec2f>

#include <algorithm>

namespace DetourNavigator
{
    namespace
    {
        struct BoundsItem
        {
            Rect mBounds;
            std::ptrdiff_t mOffset;
            unsigned char mAreaTypes;
        };

        template <std::size_t axis>
        struct LessBoundsItem
        {
            bool operator ()(const BoundsItem& lhs, const BoundsItem& rhs) const
            {
                return lhs.mBounds.mMinBound[axis] < rhs.mBounds.mMinBound[axis];
            }
        };

        void calcExtends(const std::vector<BoundsItem>& items, const std::size_t imin, const std::size_t imax,
                         Rect& bounds)
        {
            bounds = items[imin].mBounds;

            std::for_each(
                items.begin() + static_cast<std::ptrdiff_t>(imin) + 1,
                items.begin() + static_cast<std::ptrdiff_t>(imax),
                [&] (const BoundsItem& item)
                {
                    for (int i = 0; i < 2; ++i)
                    {
                        bounds.mMinBound[i] = std::min(bounds.mMinBound[i], item.mBounds.mMinBound[i]);
                        bounds.mMaxBound[i] = std::max(bounds.mMaxBound[i], item.mBounds.mMaxBound[i]);
                    }
                });
        }

        void subdivide(std::vector<BoundsItem>& items, const std::size_t imin, const std::size_t imax,
            const std::size_t trisPerChunk, const std::vector<int>& inIndices, const std::vector<AreaType>& inAreaTypes,
            std::size_t& curNode, std::vector<ChunkyTriMeshNode>& nodes, std::size_t& curTri,
            std::vector<int>& outIndices, std::vector<AreaType>& outAreaTypes)
        {
            const auto inum = imax - imin;
            const auto icur = curNode;

            if (curNode > nodes.size())
                return;

            ChunkyTriMeshNode& node = nodes[curNode++];

            if (inum <= trisPerChunk)
            {
                // Leaf
                calcExtends(items, imin, imax, node.mBounds);

                // Copy triangles.
                node.mOffset = static_cast<std::ptrdiff_t>(curTri);
                node.mSize = inum;

                for (std::size_t i = imin; i < imax; ++i)
                {
                    std::copy(
                        inIndices.begin() + items[i].mOffset * 3,
                        inIndices.begin() + items[i].mOffset * 3 + 3,
                        outIndices.begin() + static_cast<std::ptrdiff_t>(curTri) * 3
                    );
                    outAreaTypes[curTri] = inAreaTypes[static_cast<std::size_t>(items[i].mOffset)];
                    curTri++;
                }
            }
            else
            {
                // Split
                calcExtends(items, imin, imax, node.mBounds);

                if (node.mBounds.mMaxBound.x() - node.mBounds.mMinBound.x()
                    >= node.mBounds.mMaxBound.y() - node.mBounds.mMinBound.y())
                {
                    // Sort along x-axis
                    std::sort(
                        items.begin() + static_cast<std::ptrdiff_t>(imin),
                        items.begin() + static_cast<std::ptrdiff_t>(imax),
                        LessBoundsItem<0> {}
                    );
                }
                else
                {
                    // Sort along y-axis
                    std::sort(
                        items.begin() + static_cast<std::ptrdiff_t>(imin),
                        items.begin() + static_cast<std::ptrdiff_t>(imax),
                        LessBoundsItem<1> {}
                    );
                }

                const auto isplit = imin + inum / 2;

                // Left
                subdivide(items, imin, isplit, trisPerChunk, inIndices, inAreaTypes, curNode, nodes, curTri, outIndices, outAreaTypes);
                // Right
                subdivide(items, isplit, imax, trisPerChunk, inIndices, inAreaTypes, curNode, nodes, curTri, outIndices, outAreaTypes);

                const auto iescape = static_cast<std::ptrdiff_t>(curNode) - static_cast<std::ptrdiff_t>(icur);
                // Negative index means escape.
                node.mOffset = -iescape;
            }
        }
    }

    ChunkyTriMesh::ChunkyTriMesh(const std::vector<float>& verts, const std::vector<int>& indices,
                                 const std::vector<AreaType>& flags, const std::size_t trisPerChunk)
        : mMaxTrisPerChunk(0)
    {
        const auto trianglesCount = indices.size() / 3;

        if (trianglesCount == 0)
            return;

        const auto nchunks = (trianglesCount + trisPerChunk - 1) / trisPerChunk;

        mNodes.resize(nchunks * 4);
        mIndices.resize(trianglesCount * 3);
        mAreaTypes.resize(trianglesCount);

        // Build tree
        std::vector<BoundsItem> items(trianglesCount);

        for (std::size_t i = 0; i < trianglesCount; i++)
        {
            auto& item = items[i];

            item.mOffset = static_cast<std::ptrdiff_t>(i);
            item.mAreaTypes = flags[i];

            // Calc triangle XZ bounds.
            const auto baseIndex = static_cast<std::size_t>(indices[i * 3]) * 3;

            item.mBounds.mMinBound.x() = item.mBounds.mMaxBound.x() = verts[baseIndex + 0];
            item.mBounds.mMinBound.y() = item.mBounds.mMaxBound.y() = verts[baseIndex + 2];

            for (std::size_t j = 1; j < 3; ++j)
            {
                const auto index = static_cast<std::size_t>(indices[i * 3 + j]) * 3;

                item.mBounds.mMinBound.x() = std::min(item.mBounds.mMinBound.x(), verts[index + 0]);
                item.mBounds.mMinBound.y() = std::min(item.mBounds.mMinBound.y(), verts[index + 2]);

                item.mBounds.mMaxBound.x() = std::max(item.mBounds.mMaxBound.x(), verts[index + 0]);
                item.mBounds.mMaxBound.y() = std::max(item.mBounds.mMaxBound.y(), verts[index + 2]);
            }
        }

        std::size_t curTri = 0;
        std::size_t curNode = 0;
        subdivide(items, 0, trianglesCount, trisPerChunk, indices, flags, curNode, mNodes, curTri, mIndices, mAreaTypes);

        items.clear();

        mNodes.resize(curNode);

        // Calc max tris per node.
        for (auto& node : mNodes)
        {
            const bool isLeaf = node.mOffset >= 0;
            if (!isLeaf)
                continue;
            if (node.mSize > mMaxTrisPerChunk)
                mMaxTrisPerChunk = node.mSize;
        }
    }
}
