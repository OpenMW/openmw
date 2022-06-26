#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_FLAGS_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_FLAGS_H

namespace DetourNavigator
{
    using Flags = unsigned short;

    enum Flag : Flags
    {
        Flag_none = 0,
        Flag_walk = 1 << 0,
        Flag_swim = 1 << 1,
        Flag_openDoor = 1 << 2,
        Flag_usePathgrid = 1 << 3,
    };
}

#endif
