#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_VERSION_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_VERSION_H

#include <cstddef>
#include <tuple>

namespace DetourNavigator
{
    struct Version
    {
        std::size_t mGeneration = 0;
        std::size_t mRevision = 0;

        friend inline auto tie(const Version& value)
        {
            return std::tie(value.mGeneration, value.mRevision);
        }

        friend inline bool operator<(const Version& lhs, const Version& rhs)
        {
            return tie(lhs) < tie(rhs);
        }

        friend inline bool operator<=(const Version& lhs, const Version& rhs)
        {
            return tie(lhs) <= tie(rhs);
        }

        friend inline bool operator==(const Version& lhs, const Version& rhs)
        {
            return tie(lhs) == tie(rhs);
        }

        friend inline bool operator!=(const Version& lhs, const Version& rhs)
        {
            return !(lhs == rhs);
        }
    };
}

#endif
