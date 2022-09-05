#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_CHANGETYPE_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_CHANGETYPE_H

namespace DetourNavigator
{
    enum class ChangeType
    {
        remove = 0,
        mixed = 1,
        add = 2,
        update = 3,
    };

    inline ChangeType addChangeType(const ChangeType current, const ChangeType add)
    {
        return current == add ? current : ChangeType::mixed;
    }
}

#endif
