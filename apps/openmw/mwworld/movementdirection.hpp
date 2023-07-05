#ifndef OPENMW_APPS_OPENMW_MWWORLD_MOVEMENTDIRECTION_H
#define OPENMW_APPS_OPENMW_MWWORLD_MOVEMENTDIRECTION_H

namespace MWWorld
{
    using MovementDirectionFlags = unsigned char;

    enum MovementDirectionFlag : MovementDirectionFlags
    {
        MovementDirectionFlag_Forward = 1 << 0,
        MovementDirectionFlag_Back = 1 << 1,
        MovementDirectionFlag_Left = 1 << 2,
        MovementDirectionFlag_Right = 1 << 3,
    };
}

#endif
