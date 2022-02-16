#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_CHANGETYPE_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_CHANGETYPE_H

#include <ostream>

namespace DetourNavigator
{
    enum class ChangeType
    {
        remove = 0,
        mixed = 1,
        add = 2,
        update = 3,
    };

    inline std::ostream& operator <<(std::ostream& stream, ChangeType value)
    {
        switch (value)
        {
            case ChangeType::remove:
                return stream << "ChangeType::remove";
            case ChangeType::mixed:
                return stream << "ChangeType::mixed";
            case ChangeType::add:
                return stream << "ChangeType::add";
            case ChangeType::update:
                return stream << "ChangeType::update";
        }
        return stream << "ChangeType::" << static_cast<int>(value);
    }
}

#endif
