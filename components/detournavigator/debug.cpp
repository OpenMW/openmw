#include "debug.hpp"
#include "recastmesh.hpp"
#include "settings.hpp"
#include "settingsutils.hpp"
#include "tilespositionsrange.hpp"
#include "version.hpp"

#include <components/debug/writeflags.hpp>

#include <DetourNavMesh.h>
#include <DetourStatus.h>

#include <osg/io_utils>

#include <array>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <system_error>

namespace DetourNavigator
{
    std::ostream& operator<<(std::ostream& stream, const TileBounds& value)
    {
        return stream << "TileBounds {" << value.mMin << ", " << value.mMax << "}";
    }

    std::ostream& operator<<(std::ostream& stream, Status value)
    {
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(name)                                                   \
    case Status::name:                                                                                                 \
        return stream << "DetourNavigator::Status::" #name;
        switch (value)
        {
            OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(Success)
            OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(PartialPath)
            OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(NavMeshNotFound)
            OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(StartPolygonNotFound)
            OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(EndPolygonNotFound)
            OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(TargetPolygonNotFound)
            OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(MoveAlongSurfaceFailed)
            OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(FindPathOverPolygonsFailed)
            OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(InitNavMeshQueryFailed)
            OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(FindStraightPathFailed)
        }
#undef OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE
        return stream << "DetourNavigator::Error::" << static_cast<int>(value);
    }

    std::ostream& operator<<(std::ostream& s, const Water& v)
    {
        return s << "Water {" << v.mCellSize << ", " << v.mLevel << "}";
    }

    std::ostream& operator<<(std::ostream& s, const CellWater& v)
    {
        return s << "CellWater {" << v.mCellPosition << ", " << v.mWater << "}";
    }

    std::ostream& operator<<(std::ostream& s, const FlatHeightfield& v)
    {
        return s << "FlatHeightfield {" << v.mCellPosition << ", " << v.mCellSize << ", " << v.mHeight << "}";
    }

    std::ostream& operator<<(std::ostream& s, const Heightfield& v)
    {
        s << "Heightfield {.mCellPosition=" << v.mCellPosition << ", .mCellSize=" << v.mCellSize
          << ", .mLength=" << static_cast<int>(v.mLength) << ", .mMinHeight=" << v.mMinHeight
          << ", .mMaxHeight=" << v.mMaxHeight << ", .mHeights={";
        for (float h : v.mHeights)
            s << h << ", ";
        s << "}";
        return s << ", .mOriginalSize=" << v.mOriginalSize << "}";
    }

    std::ostream& operator<<(std::ostream& s, CollisionShapeType v)
    {
        switch (v)
        {
            case CollisionShapeType::Aabb:
                return s << "CollisionShapeType::Aabb";
            case CollisionShapeType::RotatingBox:
                return s << "CollisionShapeType::RotatingBox";
            case CollisionShapeType::Cylinder:
                return s << "CollisionShapeType::Cylinder";
        }
        return s << "CollisionShapeType::" << static_cast<std::underlying_type_t<CollisionShapeType>>(v);
    }

    std::ostream& operator<<(std::ostream& s, const AgentBounds& v)
    {
        return s << "AgentBounds {" << v.mShapeType << ", {" << v.mHalfExtents.x() << ", " << v.mHalfExtents.y() << ", "
                 << v.mHalfExtents.z() << "}}";
    }

    namespace
    {
        using StatusString = Debug::FlagString<unsigned int>;

        constexpr std::array dtStatuses{
            StatusString{ DT_FAILURE, "DT_FAILURE" },
            StatusString{ DT_SUCCESS, "DT_SUCCESS" },
            StatusString{ DT_IN_PROGRESS, "DT_IN_PROGRESS" },
            StatusString{ DT_WRONG_MAGIC, "DT_WRONG_MAGIC" },
            StatusString{ DT_WRONG_VERSION, "DT_WRONG_VERSION" },
            StatusString{ DT_OUT_OF_MEMORY, "DT_OUT_OF_MEMORY" },
            StatusString{ DT_INVALID_PARAM, "DT_INVALID_PARAM" },
            StatusString{ DT_BUFFER_TOO_SMALL, "DT_BUFFER_TOO_SMALL" },
            StatusString{ DT_OUT_OF_NODES, "DT_OUT_OF_NODES" },
            StatusString{ DT_PARTIAL_RESULT, "DT_PARTIAL_RESULT" },
        };
    }

    std::ostream& operator<<(std::ostream& stream, const WriteDtStatus& value)
    {
        return Debug::writeFlags(stream, value.mStatus, dtStatuses);
    }

    std::ostream& operator<<(std::ostream& stream, const Flag value)
    {
        switch (value)
        {
            case Flag_none:
                return stream << "none";
            case Flag_walk:
                return stream << "walk";
            case Flag_swim:
                return stream << "swim";
            case Flag_openDoor:
                return stream << "openDoor";
            case Flag_usePathgrid:
                return stream << "usePathgrid";
        }

        return stream;
    }

    std::ostream& operator<<(std::ostream& stream, const WriteFlags& value)
    {
        if (value.mValue == Flag_none)
        {
            return stream << Flag_none;
        }
        else
        {
            bool first = true;
            for (const auto flag : { Flag_walk, Flag_swim, Flag_openDoor, Flag_usePathgrid })
            {
                if (value.mValue & flag)
                {
                    if (!first)
                        stream << " | ";
                    first = false;
                    stream << flag;
                }
            }

            return stream;
        }
    }

    std::ostream& operator<<(std::ostream& stream, AreaType value)
    {
        switch (value)
        {
            case AreaType_null:
                return stream << "null";
            case AreaType_water:
                return stream << "water";
            case AreaType_door:
                return stream << "door";
            case AreaType_pathgrid:
                return stream << "pathgrid";
            case AreaType_ground:
                return stream << "ground";
        }
        return stream << "unknown area type (" << static_cast<std::underlying_type_t<AreaType>>(value) << ")";
    }

    std::ostream& operator<<(std::ostream& stream, ChangeType value)
    {
        switch (value)
        {
            case ChangeType::remove:
                return stream << "ChangeType::remove";
            case ChangeType::add:
                return stream << "ChangeType::add";
            case ChangeType::update:
                return stream << "ChangeType::update";
        }
        return stream << "ChangeType::" << static_cast<int>(value);
    }

    std::ostream& operator<<(std::ostream& stream, const Version& value)
    {
        return stream << "Version {" << value.mGeneration << ", " << value.mRevision << "}";
    }

    std::ostream& operator<<(std::ostream& stream, const TilesPositionsRange& value)
    {
        return stream << "TilesPositionsRange {" << value.mBegin << ", " << value.mEnd << "}";
    }

    std::ostream& operator<<(std::ostream& stream, const AreaCosts& value)
    {
        return stream << "AreaCosts {"
                      << ".mWater = " << value.mWater << ", .mDoor = " << value.mDoor
                      << ", .mPathgrid = " << value.mPathgrid << ", .mGround = " << value.mGround << "}";
    }

    std::ostream& operator<<(std::ostream& stream, const DetourSettings& value)
    {
        return stream << "DetourSettings {"
                      << ".mMaxPolys = " << value.mMaxPolys
                      << ", .mMaxNavMeshQueryNodes = " << value.mMaxNavMeshQueryNodes
                      << ", .mMaxPolygonPathSize = " << value.mMaxPolygonPathSize
                      << ", .mMaxSmoothPathSize = " << value.mMaxSmoothPathSize << "}";
    }

    void writeToFile(const RecastMesh& recastMesh, const std::string& pathPrefix, const std::string& revision,
        const RecastSettings& settings)
    {
        const auto path = pathPrefix + "recastmesh" + revision + ".obj";
        std::ofstream file(std::filesystem::path(path), std::ios::out);
        if (!file.is_open())
            throw std::system_error(
                errno, std::generic_category(), "Failed to open file to write recast mesh: " + path);
        file.exceptions(std::ios::failbit | std::ios::badbit);
        file.precision(std::numeric_limits<float>::max_exponent10);
        std::vector<float> vertices = recastMesh.getMesh().getVertices();
        for (std::size_t i = 0; i < vertices.size(); i += 3)
        {
            file << "v " << toNavMeshCoordinates(settings, vertices[i]) << ' '
                 << toNavMeshCoordinates(settings, vertices[i + 2]) << ' '
                 << toNavMeshCoordinates(settings, vertices[i + 1]) << '\n';
        }
        std::size_t count = 0;
        for (int v : recastMesh.getMesh().getIndices())
        {
            if (count % 3 == 0)
            {
                if (count != 0)
                    file << '\n';
                file << 'f';
            }
            file << ' ' << (v + 1);
            ++count;
        }
        file << '\n';
    }

    void writeToFile(const dtNavMesh& navMesh, const std::string& pathPrefix, const std::string& revision)
    {
        const int navMeshSetMagic = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T'; //'MSET';
        const int navMeshSetVersion = 1;

        struct NavMeshSetHeader
        {
            int magic;
            int version;
            int numTiles;
            dtNavMeshParams params;
        };

        struct NavMeshTileHeader
        {
            dtTileRef tileRef;
            int dataSize;
        };

        const auto path = pathPrefix + "all_tiles_navmesh" + revision + ".bin";
        std::ofstream file(std::filesystem::path(path), std::ios::out | std::ios::binary);
        if (!file.is_open())
            throw std::system_error(errno, std::generic_category(), "Failed to open file to write navmesh: " + path);
        file.exceptions(std::ios::failbit | std::ios::badbit);

        NavMeshSetHeader header;
        header.magic = navMeshSetMagic;
        header.version = navMeshSetVersion;
        header.numTiles = 0;
        for (int i = 0; i < navMesh.getMaxTiles(); ++i)
        {
            const auto tile = navMesh.getTile(i);
            if (!tile || !tile->header || !tile->dataSize)
                continue;
            header.numTiles++;
        }
        header.params = *navMesh.getParams();

        using const_char_ptr = const char*;
        file.write(const_char_ptr(&header), sizeof(header));

        for (int i = 0; i < navMesh.getMaxTiles(); ++i)
        {
            const auto tile = navMesh.getTile(i);
            if (!tile || !tile->header || !tile->dataSize)
                continue;

            NavMeshTileHeader tileHeader;
            tileHeader.tileRef = navMesh.getTileRef(tile);
            tileHeader.dataSize = tile->dataSize;

            file.write(const_char_ptr(&tileHeader), sizeof(tileHeader));
            file.write(const_char_ptr(tile->data), tile->dataSize);
        }
    }
}
