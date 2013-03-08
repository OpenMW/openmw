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

    typedef std::deque<std::string> AnimationQueue;
    AnimationQueue mAnimQueue;

    std::string mCurrentGroup;
    CharacterState mState;
    bool mSkipAnim;

protected:
    /* Called by the animation whenever a new text key is reached. */
    void markerEvent(float time, const std::string &evt);

    friend class MWRender::Animation;

public:
    CharacterController(const MWWorld::Ptr &ptr, MWRender::Animation *anim, CharacterState state, bool loop);
    CharacterController(const CharacterController &rhs);
    virtual ~CharacterController();

    void updatePtr(const MWWorld::Ptr &ptr);

    Ogre::Vector3 update(float duration);

    void playGroup(const std::string &groupname, int mode, int count);
    void skipAnim();

    void setState(CharacterState state, bool loop);
    CharacterState getState() const
    { return mState; }
};

}

#endif /* GAME_MWMECHANICS_CHARACTER_HPP */
