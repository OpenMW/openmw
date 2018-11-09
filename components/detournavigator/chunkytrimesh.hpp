#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_CHUNKYTRIMESH_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_CHUNKYTRIMESH_H

#include "areatype.hpp"

#include <osg/Vec2f>

#include <array>
#include <vector>

namespace DetourNavigator
{
    struct Rect
    {
        osg::Vec2f mMinBound;
        osg::Vec2f mMaxBound;
    };

    struct ChunkyTriMeshNode
    {
        Rect mBounds;
        std::ptrdiff_t mOffset;
        std::size_t mSize;
    };

    struct Chunk
    {
        const int* const mIndices;
        const AreaType* const mAreaTypes;
        const std::size_t mSize;
    };

    inline bool checkOverlapRect(const Rect& lhs, const Rect& rhs)
    {
        bool overlap = true;
        overlap = (lhs.mMinBound.x() > rhs.mMaxBound.x() || lhs.mMaxBound.x() < rhs.mMinBound.x()) ? false : overlap;
        overlap = (lhs.mMinBound.y() > rhs.mMaxBound.y() || lhs.mMaxBound.y() < rhs.mMinBound.y()) ? false : overlap;
        return overlap;
    }

    class ChunkyTriMesh
    {
    public:
        /// Creates partitioned triangle mesh (AABB tree),
        /// where each node contains at max trisPerChunk triangles.
        ChunkyTriMesh(const std::vector<float>& verts, const std::vector<int>& tris,
                      const std::vector<AreaType>& flags, const std::size_t trisPerChunk);

        ChunkyTriMesh(const ChunkyTriMesh&) = delete;
        ChunkyTriMesh& operator=(const ChunkyTriMesh&) = delete;

        /// Returns the chunk indices which overlap the input rectable.
        template <class Function>
        void forEachChunksOverlappingRect(const Rect& rect, Function&& function) const
        {
            // Traverse tree
            for (std::size_t i = 0; i < mNodes.size(); )
            {
                const ChunkyTriMeshNode* node = &mNodes[i];
                const bool overlap = checkOverlapRect(rect, node->mBounds);
                const bool isLeafNode = node->mOffset >= 0;

                if (isLeafNode && overlap)
                    function(i);

                if (overlap || isLeafNode)
                    i++;
                else
                {
                    const auto escapeIndex = -node->mOffset;
                    i += static_cast<std::size_t>(escapeIndex);
                }
            }
        }

        Chunk getChunk(const std::size_t chunkId) const
        {
            const auto& node = mNodes[chunkId];
            return Chunk {
                mIndices.data() + node.mOffset * 3,
                mAreaTypes.data() + node.mOffset,
                node.mSize
            };
        }

        std::size_t getMaxTrisPerChunk() const
        {
            return mMaxTrisPerChunk;
        }

    private:
        std::vector<ChunkyTriMeshNode> mNodes;
        std::vector<int> mIndices;
        std::vector<AreaType> mAreaTypes;
        std::size_t mMaxTrisPerChunk;
    };
}

#endif
