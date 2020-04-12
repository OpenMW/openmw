#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_H

#include "tilebounds.hpp"
#include "status.hpp"

#include <osg/io_utils>

#include <components/bullethelpers/operators.hpp>
#include <components/misc/guarded.hpp>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

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

    class RecastMesh;

    void writeToFile(const RecastMesh& recastMesh, const std::string& pathPrefix, const std::string& revision);
    void writeToFile(const dtNavMesh& navMesh, const std::string& pathPrefix, const std::string& revision);
}

#endif
