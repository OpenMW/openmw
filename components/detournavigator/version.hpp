#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_VERSION_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_VERSION_H

#include <cstddef>
#include <tuple>

namespace DetourNavigator
{
    struct Version
    {
        std::size_t mGeneration;
        std::size_t mRevision;

        friend inline bool operator<(const Version& lhs, const Version& rhs)
        {
            return std::tie(lhs.mGeneration, lhs.mRevision) < std::tie(rhs.mGeneration, rhs.mRevision);
        }
    };
}

#endif
