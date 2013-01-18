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
    CharState_Alive,
    CharState_Dead
};

class CharacterController
{
    MWWorld::Ptr mPtr;
    MWRender::Animation *mAnimation;

    std::vector<std::string> mAnimNames;

    typedef std::deque<std::string> AnimationQueue;
    AnimationQueue mAnimQueue;

    std::string mCurrentGroup;
    CharacterState mState;
    bool mSkipAnim;

protected:
    /* Called by the animation whenever a new text key is reached. */
    void markerEvent(const std::string &evt);

    friend class MWRender::Animation;

public:
    CharacterController(const MWWorld::Ptr &ptr, MWRender::Animation *anim, CharacterState state);
    CharacterController(const CharacterController &rhs);

    Ogre::Vector3 update(float duration);

    void playGroup(const std::string &groupname, int mode, int count);
    void skipAnim();

    void setState(CharacterState state);
    CharacterState getState() const
    { return mState; }
};

}

#endif /* GAME_MWMECHANICS_CHARACTER_HPP */
