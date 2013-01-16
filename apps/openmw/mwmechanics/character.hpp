#ifndef GAME_MWMECHANICS_CHARACTER_HPP
#define GAME_MWMECHANICS_CHARACTER_HPP

#include "../mwworld/ptr.hpp"

namespace MWRender
{
    class Animation;
}

namespace MWMechanics
{

enum CharacterState {
    CharState_Idle,
    CharState_Dead
};

class CharacterController
{
    MWWorld::Ptr mPtr;
    MWRender::Animation *mAnimation;

    CharacterState mState;

public:
    CharacterController(const MWWorld::Ptr &ptr, MWRender::Animation *anim, CharacterState state);

    void setState(CharacterState state);
    CharacterState getState() const
    { return mState; }
};

}

#endif /* GAME_MWMECHANICS_CHARACTER_HPP */
