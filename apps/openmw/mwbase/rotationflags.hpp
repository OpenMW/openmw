#ifndef GAME_MWBASE_ROTATIONFLAGS_H
#define GAME_MWBASE_ROTATIONFLAGS_H

namespace MWBase
{
    using RotationFlags = unsigned short;

    enum RotationFlag : RotationFlags
    {
        RotationFlag_none = 0,
        RotationFlag_adjust = 1,
        RotationFlag_inverseOrder = 1 << 1,
    };
}

#endif
