#include "navmeshmode.hpp"

#include <stdexcept>
#include <string>

namespace MWRender
{
    NavMeshMode parseNavMeshMode(std::string_view value)
    {
        if (value == "area type")
            return NavMeshMode::AreaType;
        if (value == "update frequency")
            return NavMeshMode::UpdateFrequency;
        throw std::logic_error("Unsupported navigation mesh rendering mode: " + std::string(value));
    }
}
