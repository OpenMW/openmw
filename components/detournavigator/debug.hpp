#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_H

#include "tilebounds.hpp"
#include "status.hpp"
#include "recastmesh.hpp"
#include "agentbounds.hpp"

#include <osg/io_utils>

#include <components/bullethelpers/operators.hpp>

#include <string>

class dtNavMesh;

namespace DetourNavigator
{
    inline std::ostream& operator <<(std::ostream& stream, const TileBounds& value)
    {
        return stream << "TileBounds {" << value.mMin << ", " << value.mMax << "}";
    }

    inline std::ostream& operator <<(std::ostream& stream, Status value)
    {
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(name) \
    case Status::name: return stream << "DetourNavigator::Status::"#name;
        switch (value)
        {
            OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(Success)
            OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(PartialPath)
            OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(NavMeshNotFound)
            OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(StartPolygonNotFound)
            OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(EndPolygonNotFound)
            OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(MoveAlongSurfaceFailed)
            OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(FindPathOverPolygonsFailed)
            OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(GetPolyHeightFailed)
            OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE(InitNavMeshQueryFailed)
        }
#undef OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_STATUS_MESSAGE
        return stream << "DetourNavigator::Error::" << static_cast<int>(value);
    }

    inline std::ostream& operator<<(std::ostream& s, const Water& v)
    {
        return s << "Water {" << v.mCellSize << ", " << v.mLevel << "}";
    }

    inline std::ostream& operator<<(std::ostream& s, const CellWater& v)
    {
        return s << "CellWater {" << v.mCellPosition << ", " << v.mWater << "}";
    }

    inline std::ostream& operator<<(std::ostream& s, const FlatHeightfield& v)
    {
        return s << "FlatHeightfield {" << v.mCellPosition << ", " << v.mCellSize << ", " << v.mHeight << "}";
    }

    inline std::ostream& operator<<(std::ostream& s, const Heightfield& v)
    {
        s << "Heightfield {.mCellPosition=" << v.mCellPosition
          << ", .mCellSize=" << v.mCellSize
          << ", .mLength=" << static_cast<int>(v.mLength)
          << ", .mMinHeight=" << v.mMinHeight
          << ", .mMaxHeight=" << v.mMaxHeight
          << ", .mHeights={";
        for (float h : v.mHeights)
            s << h << ", ";
        s << "}";
        return s << ", .mOriginalSize=" << v.mOriginalSize << "}";
    }

    inline std::ostream& operator<<(std::ostream& s, CollisionShapeType v)
    {
        switch (v)
        {
            case CollisionShapeType::Aabb: return s << "AgentShapeType::Aabb";
            case CollisionShapeType::RotatingBox: return s << "AgentShapeType::RotatingBox";
        }
        return s << "AgentShapeType::" << static_cast<std::underlying_type_t<CollisionShapeType>>(v);
    }

    inline std::ostream& operator<<(std::ostream& s, const AgentBounds& v)
    {
        return s << "AgentBounds {" << v.mShapeType << ", " << v.mHalfExtents << "}";
    }

    class RecastMesh;
    struct RecastSettings;

    void writeToFile(const RecastMesh& recastMesh, const std::string& pathPrefix,
        const std::string& revision, const RecastSettings& settings);
    void writeToFile(const dtNavMesh& navMesh, const std::string& pathPrefix, const std::string& revision);
}

#endif
