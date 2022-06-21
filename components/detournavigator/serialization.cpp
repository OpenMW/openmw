#include "serialization.hpp"

#include "dbrefgeometryobject.hpp"
#include "preparednavmeshdata.hpp"
#include "recast.hpp"
#include "recastmesh.hpp"
#include "settings.hpp"
#include "agentbounds.hpp"

#include <components/serialization/binaryreader.hpp>
#include <components/serialization/binarywriter.hpp>
#include <components/serialization/format.hpp>
#include <components/serialization/sizeaccumulator.hpp>

#include <cstddef>
#include <cstring>
#include <type_traits>
#include <vector>

namespace DetourNavigator
{
namespace
{
    template <Serialization::Mode mode>
    struct Format : Serialization::Format<mode, Format<mode>>
    {
        using Serialization::Format<mode, Format<mode>>::operator();

        template <class Visitor>
        void operator()(Visitor&& visitor, const osg::Vec2i& value) const
        {
            visitor(*this, value.ptr(), 2);
        }

        template <class Visitor>
        void operator()(Visitor&& visitor, const osg::Vec2f& value) const
        {
            visitor(*this, value.ptr(), 2);
        }

        template <class Visitor>
        void operator()(Visitor&& visitor, const osg::Vec3f& value) const
        {
            visitor(*this, value.ptr(), 3);
        }

        template <class Visitor>
        void operator()(Visitor&& visitor, const Water& value) const
        {
            visitor(*this, value.mCellSize);
            visitor(*this, value.mLevel);
        }

        template <class Visitor>
        void operator()(Visitor&& visitor, const CellWater& value) const
        {
            visitor(*this, value.mCellPosition);
            visitor(*this, value.mWater);
        }

        template <class Visitor>
        void operator()(Visitor&& visitor, const RecastSettings& value) const
        {
            visitor(*this, value.mCellHeight);
            visitor(*this, value.mCellSize);
            visitor(*this, value.mDetailSampleDist);
            visitor(*this, value.mDetailSampleMaxError);
            visitor(*this, value.mMaxClimb);
            visitor(*this, value.mMaxSimplificationError);
            visitor(*this, value.mMaxSlope);
            visitor(*this, value.mRecastScaleFactor);
            visitor(*this, value.mSwimHeightScale);
            visitor(*this, value.mBorderSize);
            visitor(*this, value.mMaxEdgeLen);
            visitor(*this, value.mMaxVertsPerPoly);
            visitor(*this, value.mRegionMergeArea);
            visitor(*this, value.mRegionMinArea);
            visitor(*this, value.mTileSize);
        }

        template <class Visitor>
        void operator()(Visitor&& visitor, const TileBounds& value) const
        {
            visitor(*this, value.mMin);
            visitor(*this, value.mMax);
        }

        template <class Visitor>
        void operator()(Visitor&& visitor, const Heightfield& value) const
        {
            visitor(*this, value.mCellPosition);
            visitor(*this, value.mCellSize);
            visitor(*this, value.mLength);
            visitor(*this, value.mMinHeight);
            visitor(*this, value.mMaxHeight);
            visitor(*this, value.mHeights);
            visitor(*this, value.mOriginalSize);
            visitor(*this, value.mMinX);
            visitor(*this, value.mMinY);
        }

        template <class Visitor>
        void operator()(Visitor&& visitor, const FlatHeightfield& value) const
        {
            visitor(*this, value.mCellPosition);
            visitor(*this, value.mCellSize);
            visitor(*this, value.mHeight);
        }

        template <class Visitor>
        void operator()(Visitor&& visitor, const RecastMesh& value) const
        {
            visitor(*this, value.getWater());
            visitor(*this, value.getHeightfields());
            visitor(*this, value.getFlatHeightfields());
        }

        template <class Visitor>
        void operator()(Visitor&& visitor, const ESM::Position& value) const
        {
            visitor(*this, value.pos);
            visitor(*this, value.rot);
        }

        template <class Visitor>
        void operator()(Visitor&& visitor, const ObjectTransform& value) const
        {
            visitor(*this, value.mPosition);
            visitor(*this, value.mScale);
        }

        template <class Visitor>
        void operator()(Visitor&& visitor, const DbRefGeometryObject& value) const
        {
            visitor(*this, value.mShapeId);
            visitor(*this, value.mObjectTransform);
        }

        template <class Visitor>
        void operator()(Visitor&& visitor, const RecastSettings& settings, const AgentBounds& agentBounds,
            const RecastMesh& recastMesh, const std::vector<DbRefGeometryObject>& dbRefGeometryObjects) const
        {
            visitor(*this, DetourNavigator::recastMeshMagic);
            visitor(*this, DetourNavigator::recastMeshVersion);
            visitor(*this, settings);
            visitor(*this, agentBounds);
            visitor(*this, recastMesh);
            visitor(*this, dbRefGeometryObjects);
        }

        template <class Visitor, class T>
        auto operator()(Visitor&& visitor, T& value) const
            -> std::enable_if_t<std::is_same_v<std::decay_t<T>, rcPolyMesh>>
        {
            visitor(*this, value.nverts);
            visitor(*this, value.npolys);
            visitor(*this, value.maxpolys);
            visitor(*this, value.nvp);
            visitor(*this, value.bmin);
            visitor(*this, value.bmax);
            visitor(*this, value.cs);
            visitor(*this, value.ch);
            visitor(*this, value.borderSize);
            visitor(*this, value.maxEdgeError);
            if constexpr (mode == Serialization::Mode::Read)
            {
                if (value.verts == nullptr)
                    permRecastAlloc(value.verts, getVertsLength(value));
                if (value.polys == nullptr)
                    permRecastAlloc(value.polys, getPolysLength(value));
                if (value.regs == nullptr)
                    permRecastAlloc(value.regs, getRegsLength(value));
                if (value.flags == nullptr)
                    permRecastAlloc(value.flags, getFlagsLength(value));
                if (value.areas == nullptr)
                    permRecastAlloc(value.areas, getAreasLength(value));
            }
            visitor(*this, value.verts, getVertsLength(value));
            visitor(*this, value.polys, getPolysLength(value));
            visitor(*this, value.regs, getRegsLength(value));
            visitor(*this, value.flags, getFlagsLength(value));
            visitor(*this, value.areas, getAreasLength(value));
        }

        template <class Visitor, class T>
        auto operator()(Visitor&& visitor, T& value) const
            -> std::enable_if_t<std::is_same_v<std::decay_t<T>, rcPolyMeshDetail>>
        {
            visitor(*this, value.nmeshes);
            if constexpr (mode == Serialization::Mode::Read)
                if (value.meshes == nullptr)
                    permRecastAlloc(value.meshes, getMeshesLength(value));
            visitor(*this, value.meshes, getMeshesLength(value));
            visitor(*this, value.nverts);
            if constexpr (mode == Serialization::Mode::Read)
                if (value.verts == nullptr)
                    permRecastAlloc(value.verts, getVertsLength(value));
            visitor(*this, value.verts, getVertsLength(value));
            visitor(*this, value.ntris);
            if constexpr (mode == Serialization::Mode::Read)
                if (value.tris == nullptr)
                    permRecastAlloc(value.tris, getTrisLength(value));
            visitor(*this, value.tris, getTrisLength(value));
        }

        template <class Visitor, class T>
        auto operator()(Visitor&& visitor, T& value) const
            -> std::enable_if_t<std::is_same_v<std::decay_t<T>, PreparedNavMeshData>>
        {
            if constexpr (mode == Serialization::Mode::Write)
            {
                visitor(*this, DetourNavigator::preparedNavMeshDataMagic);
                visitor(*this, DetourNavigator::preparedNavMeshDataVersion);
            }
            else
            {
                static_assert(mode == Serialization::Mode::Read);
                char magic[std::size(DetourNavigator::preparedNavMeshDataMagic)];
                visitor(*this, magic);
                if (std::memcmp(magic, DetourNavigator::preparedNavMeshDataMagic, sizeof(magic)) != 0)
                    throw std::runtime_error("Bad PreparedNavMeshData magic");
                std::uint32_t version = 0;
                visitor(*this, version);
                if (version != DetourNavigator::preparedNavMeshDataVersion)
                    throw std::runtime_error("Bad PreparedNavMeshData version");
            }
            visitor(*this, value.mUserId);
            visitor(*this, value.mCellSize);
            visitor(*this, value.mCellHeight);
            visitor(*this, value.mPolyMesh);
            visitor(*this, value.mPolyMeshDetail);
        }

        template <class Visitor>
        void operator()(Visitor&& visitor, const AgentBounds& value) const
        {
            visitor(*this, value.mShapeType);
            visitor(*this, value.mHalfExtents);
        }
    };
}
} // namespace DetourNavigator

namespace DetourNavigator
{
    std::vector<std::byte> serialize(const RecastSettings& settings, const AgentBounds& agentBounds,
        const RecastMesh& recastMesh, const std::vector<DbRefGeometryObject>& dbRefGeometryObjects)
    {
        constexpr Format<Serialization::Mode::Write> format;
        Serialization::SizeAccumulator sizeAccumulator;
        format(sizeAccumulator, settings, agentBounds, recastMesh, dbRefGeometryObjects);
        std::vector<std::byte> result(sizeAccumulator.value());
        format(Serialization::BinaryWriter(result.data(), result.data() + result.size()),
               settings, agentBounds, recastMesh, dbRefGeometryObjects);
        return result;
    }

    std::vector<std::byte> serialize(const PreparedNavMeshData& value)
    {
        constexpr Format<Serialization::Mode::Write> format;
        Serialization::SizeAccumulator sizeAccumulator;
        format(sizeAccumulator, value);
        std::vector<std::byte> result(sizeAccumulator.value());
        format(Serialization::BinaryWriter(result.data(), result.data() + result.size()), value);
        return result;
    }

    bool deserialize(const std::vector<std::byte>& data, PreparedNavMeshData& value)
    {
        try
        {
            constexpr Format<Serialization::Mode::Read> format;
            format(Serialization::BinaryReader(data.data(), data.data() + data.size()), value);
            return true;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }
}
