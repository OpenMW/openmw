#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_DEBUG_H

#include "tilebounds.hpp"
#include "status.hpp"
#include "recastmesh.hpp"
#include "agentbounds.hpp"
#include "flags.hpp"
#include "areatype.hpp"
#include "changetype.hpp"

#include <DetourStatus.h>

#include <string>
#include <iosfwd>

class dtNavMesh;

namespace DetourNavigator
{
    std::ostream& operator<<(std::ostream& stream, const TileBounds& value);

    std::ostream& operator<<(std::ostream& stream, Status value);

    std::ostream& operator<<(std::ostream& s, const Water& v);

    std::ostream& operator<<(std::ostream& s, const CellWater& v);

    std::ostream& operator<<(std::ostream& s, const FlatHeightfield& v);

    std::ostream& operator<<(std::ostream& s, const Heightfield& v);

    std::ostream& operator<<(std::ostream& s, CollisionShapeType v);

    std::ostream& operator<<(std::ostream& s, const AgentBounds& v);

    struct WriteDtStatus
    {
        dtStatus mStatus;
    };

    std::ostream& operator<<(std::ostream& stream, const WriteDtStatus& value);

    std::ostream& operator<<(std::ostream& stream, const Flag value);

    struct WriteFlags
    {
        Flags mValue;
    };

    std::ostream& operator<<(std::ostream& stream, const WriteFlags& value);

    std::ostream& operator<<(std::ostream& stream, AreaType value);

    std::ostream& operator<<(std::ostream& stream, ChangeType value);

    class RecastMesh;
    struct RecastSettings;

    void writeToFile(const RecastMesh& recastMesh, const std::string& pathPrefix,
        const std::string& revision, const RecastSettings& settings);
    void writeToFile(const dtNavMesh& navMesh, const std::string& pathPrefix, const std::string& revision);
}

#endif
