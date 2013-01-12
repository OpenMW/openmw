#ifndef GAME_MWMECHANICS_CHARACTER_HPP
#define GAME_MWMECHANICS_CHARACTER_HPP

#include "../mwworld/ptr.hpp"

namespace MWMechanics
{

enum CharacterState {
    CharState_Idle,
    CharState_Dead
};

class CharacterController
{
    MWWorld::Ptr mPtr;

    CharacterState mState;

public:
    CharacterController(const MWWorld::Ptr &ptr, CharacterState state)
      : mPtr(ptr), mState(state)
    { }

    CharacterState getState() const
    { return mState; }

    void setState(CharacterState state)
    { mState = state; }
};

}

#endif /* GAME_MWMECHANICS_CHARACTER_HPP */
