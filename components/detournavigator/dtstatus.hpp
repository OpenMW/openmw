#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_DTSTATUS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_DTSTATUS_H

#include <DetourStatus.h>

#include <sstream>
#include <vector>

namespace DetourNavigator
{
    struct WriteDtStatus
    {
        dtStatus status;
    };

    static const std::vector<std::pair<const dtStatus, const char* const>> dtStatuses {
        {DT_FAILURE, "DT_FAILURE"},
        {DT_SUCCESS, "DT_SUCCESS"},
        {DT_IN_PROGRESS, "DT_IN_PROGRESS"},
        {DT_WRONG_MAGIC, "DT_WRONG_MAGIC"},
        {DT_WRONG_VERSION, "DT_WRONG_VERSION"},
        {DT_OUT_OF_MEMORY, "DT_OUT_OF_MEMORY"},
        {DT_INVALID_PARAM, "DT_INVALID_PARAM"},
        {DT_BUFFER_TOO_SMALL, "DT_BUFFER_TOO_SMALL"},
        {DT_OUT_OF_NODES, "DT_OUT_OF_NODES"},
        {DT_PARTIAL_RESULT, "DT_PARTIAL_RESULT"},
    };

    inline std::ostream& operator <<(std::ostream& stream, const WriteDtStatus& value)
    {
        for (const auto& status : dtStatuses)
            if (value.status & status.first)
                stream << status.second << " ";
        return stream;
    }
}

#endif
