#ifndef GAME_MWMECHANICS_CHARACTER_HPP
#define GAME_MWMECHANICS_CHARACTER_HPP

#include <OgreVector3.h>

#include "../mwworld/ptr.hpp"

namespace MWRender
{
    class Animation;
}

namespace MWMechanics
{

class Movement;

enum CharacterState {
    CharState_SpecialIdle,
    CharState_Idle,
    CharState_Idle2,
    CharState_Idle3,
    CharState_Idle4,
    CharState_Idle5,
    CharState_Idle6,
    CharState_Idle7,
    CharState_Idle8,
    CharState_Idle9,
    CharState_IdleSwim,
    CharState_IdleSneak,

    CharState_WalkForward,
    CharState_WalkBack,
    CharState_WalkLeft,
    CharState_WalkRight,

    CharState_SwimWalkForward,
    CharState_SwimWalkBack,
    CharState_SwimWalkLeft,
    CharState_SwimWalkRight,

    CharState_RunForward,
    CharState_RunBack,
    CharState_RunLeft,
    CharState_RunRight,

    CharState_SwimRunForward,
    CharState_SwimRunBack,
    CharState_SwimRunLeft,
    CharState_SwimRunRight,

    CharState_SneakForward,
    CharState_SneakBack,
    CharState_SneakLeft,
    CharState_SneakRight,

    CharState_TurnLeft,
    CharState_TurnRight,

    CharState_Jump,

    /* Death states must be last! */
    CharState_Death1,
    CharState_Death2,
    CharState_Death3,
    CharState_Death4,
    CharState_Death5
};

class CharacterController
{
    MWWorld::Ptr mPtr;
    MWRender::Animation *mAnimation;

    typedef std::deque<std::pair<std::string,size_t> > AnimationQueue;
    AnimationQueue mAnimQueue;

    CharacterState mCharState;
    bool mLooping;
    bool mSkipAnim;

    // counted for skill increase
    float mSecondsOfSwimming;
    float mSecondsOfRunning;

    float mFallDistance;
    float mLastHeight;

    bool mMovingAnim;

public:
    CharacterController(const MWWorld::Ptr &ptr, MWRender::Animation *anim, CharacterState state, bool loop);
    virtual ~CharacterController();

    void updatePtr(const MWWorld::Ptr &ptr);

    void update(float duration, Movement &movement);

    void playGroup(const std::string &groupname, int mode, int count);
    void skipAnim();

    void setState(CharacterState state, bool loop);
    CharacterState getState() const
    { return mCharState; }

    void forceStateUpdate();
};

}

#endif /* GAME_MWMECHANICS_CHARACTER_HPP */
