#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESH_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_RECASTMESH_H

#include "areatype.hpp"
#include "objecttransform.hpp"
#include "version.hpp"

#include <components/resource/bulletshape.hpp>

#include <osg/Vec2i>
#include <osg/Vec3f>

#include <cstdint>
#include <memory>
#include <numeric>
#include <string>
#include <tuple>
#include <vector>

namespace DetourNavigator
{
    class Mesh
    {
    public:
        Mesh(std::vector<int>&& indices, std::vector<float>&& vertices, std::vector<AreaType>&& areaTypes);

        const std::vector<int>& getIndices() const noexcept { return mIndices; }
        const std::vector<float>& getVertices() const noexcept { return mVertices; }
        const std::vector<AreaType>& getAreaTypes() const noexcept { return mAreaTypes; }
        std::size_t getVerticesCount() const noexcept { return mVertices.size() / 3; }
        std::size_t getTrianglesCount() const noexcept { return mAreaTypes.size(); }

    private:
        std::vector<int> mIndices;
        std::vector<float> mVertices;
        std::vector<AreaType> mAreaTypes;

        friend inline bool operator<(const Mesh& lhs, const Mesh& rhs) noexcept
        {
            return std::tie(lhs.mIndices, lhs.mVertices, lhs.mAreaTypes)
                < std::tie(rhs.mIndices, rhs.mVertices, rhs.mAreaTypes);
        }

        friend inline std::size_t getSize(const Mesh& value) noexcept
        {
            return value.mIndices.size() * sizeof(int) + value.mVertices.size() * sizeof(float)
                + value.mAreaTypes.size() * sizeof(AreaType);
        }
    };

    struct Water
    {
        int mCellSize;
        float mLevel;
    };

    inline bool operator<(const Water& lhs, const Water& rhs) noexcept
    {
        const auto tie = [](const Water& v) { return std::tie(v.mCellSize, v.mLevel); };
        return tie(lhs) < tie(rhs);
    }

    struct CellWater
    {
        osg::Vec2i mCellPosition;
        Water mWater;
    };

    inline bool operator<(const CellWater& lhs, const CellWater& rhs) noexcept
    {
        const auto tie = [](const CellWater& v) { return std::tie(v.mCellPosition, v.mWater); };
        return tie(lhs) < tie(rhs);
    }

    inline osg::Vec2f getWaterShift2d(const osg::Vec2i& cellPosition, int cellSize)
    {
        return osg::Vec2f((cellPosition.x() + 0.5f) * cellSize, (cellPosition.y() + 0.5f) * cellSize);
    }

    inline osg::Vec3f getWaterShift3d(const osg::Vec2i& cellPosition, int cellSize, float level)
    {
        return osg::Vec3f(getWaterShift2d(cellPosition, cellSize), level);
    }

    struct Heightfield
    {
        osg::Vec2i mCellPosition;
        int mCellSize;
        std::uint8_t mLength;
        float mMinHeight;
        float mMaxHeight;
        std::vector<float> mHeights;
        std::size_t mOriginalSize;
        std::uint8_t mMinX;
        std::uint8_t mMinY;
    };

    inline auto makeTuple(const Heightfield& v) noexcept
    {
        return std::tie(v.mCellPosition, v.mCellSize, v.mLength, v.mMinHeight, v.mMaxHeight, v.mHeights,
            v.mOriginalSize, v.mMinX, v.mMinY);
    }

    inline bool operator<(const Heightfield& lhs, const Heightfield& rhs) noexcept
    {
        return makeTuple(lhs) < makeTuple(rhs);
    }

    struct FlatHeightfield
    {
        osg::Vec2i mCellPosition;
        int mCellSize;
        float mHeight;
    };

    inline bool operator<(const FlatHeightfield& lhs, const FlatHeightfield& rhs) noexcept
    {
        const auto tie = [](const FlatHeightfield& v) { return std::tie(v.mCellPosition, v.mCellSize, v.mHeight); };
        return tie(lhs) < tie(rhs);
    }

    struct MeshSource
    {
        osg::ref_ptr<const Resource::BulletShape> mShape;
        ObjectTransform mObjectTransform;
        AreaType mAreaType;
    };

    class RecastMesh
    {
    public:
        explicit RecastMesh(const Version& version, Mesh mesh, std::vector<CellWater> water,
            std::vector<Heightfield> heightfields, std::vector<FlatHeightfield> flatHeightfields,
            std::vector<MeshSource> sources);

        const Version& getVersion() const noexcept { return mVersion; }

        const Mesh& getMesh() const noexcept { return mMesh; }

        const std::vector<CellWater>& getWater() const { return mWater; }

        const std::vector<Heightfield>& getHeightfields() const noexcept { return mHeightfields; }

        const std::vector<FlatHeightfield>& getFlatHeightfields() const noexcept { return mFlatHeightfields; }

        const std::vector<MeshSource>& getMeshSources() const noexcept { return mMeshSources; }

    private:
        Version mVersion;
        Mesh mMesh;
        std::vector<CellWater> mWater;
        std::vector<Heightfield> mHeightfields;
        std::vector<FlatHeightfield> mFlatHeightfields;
        std::vector<MeshSource> mMeshSources;

        friend inline std::size_t getSize(const RecastMesh& value) noexcept
        {
            return getSize(value.mMesh) + value.mWater.size() * sizeof(CellWater)
                + value.mHeightfields.size() * sizeof(Heightfield)
                + std::accumulate(value.mHeightfields.begin(), value.mHeightfields.end(), std::size_t{ 0 },
                    [](std::size_t r, const Heightfield& v) { return r + v.mHeights.size() * sizeof(float); })
                + value.mFlatHeightfields.size() * sizeof(FlatHeightfield);
        }
    };
}

#endif
