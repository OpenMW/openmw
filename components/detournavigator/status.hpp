#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_STATUS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_STATUS_H

namespace DetourNavigator
{
    enum class Status
    {
        Success,
        PartialPath,
        NavMeshNotFound,
        StartPolygonNotFound,
        EndPolygonNotFound,
        TargetPolygonNotFound,
        MoveAlongSurfaceFailed,
        FindPathOverPolygonsFailed,
        InitNavMeshQueryFailed,
        FindStraightPathFailed,
    };

    constexpr const char* getMessage(Status value)
    {
        switch (value)
        {
            case Status::Success:
                return "success";
            case Status::PartialPath:
                return "partial path is found";
            case Status::NavMeshNotFound:
                return "navmesh is not found";
            case Status::StartPolygonNotFound:
                return "polygon for start position is not found on navmesh";
            case Status::EndPolygonNotFound:
                return "polygon for end position is not found on navmesh";
            case Status::TargetPolygonNotFound:
                return "polygon for target position is not found on navmesh";
            case Status::MoveAlongSurfaceFailed:
                return "move along surface on navmesh is failed";
            case Status::FindPathOverPolygonsFailed:
                return "path over navmesh polygons is not found";
            case Status::InitNavMeshQueryFailed:
                return "failed to init navmesh query";
            case Status::FindStraightPathFailed:
                return "failed to straight path using polygon path";
        }
        return "unknown error";
    }
}

#endif
