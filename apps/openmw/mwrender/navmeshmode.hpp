#ifndef OPENMW_MWRENDER_NAVMESHMODE_H
#define OPENMW_MWRENDER_NAVMESHMODE_H

#include <string_view>

namespace MWRender
{
    enum class NavMeshMode
    {
        AreaType,
        UpdateFrequency,
    };

    NavMeshMode parseNavMeshMode(std::string_view value);
}

#endif
