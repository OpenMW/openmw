#ifndef GAME_MWMECHANICS_DRAWSTATE_H
#define GAME_MWMECHANICS_DRAWSTATE_H

namespace MWMechanics
{
    /// \note The _ suffix is required to avoid a collision with a Windoze macro. Die, Microsoft! Die!
    enum DrawState_
    {
        DrawState_Nothing = 0,
        DrawState_Weapon = 1,
        DrawState_Spell = 2
    };
}

#endif
