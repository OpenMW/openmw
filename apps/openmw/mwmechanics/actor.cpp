#include "actor.hpp"

#include "character.hpp"

namespace MWMechanics
{
    Actor::Actor(const MWWorld::Ptr &ptr, MWRender::Animation *animation)
    {
        mCharacterController.reset(new CharacterController(ptr, animation));
    }

    void Actor::updatePtr(const MWWorld::Ptr &newPtr)
    {
        mCharacterController->updatePtr(newPtr);
    }

    CharacterController* Actor::getCharacterController()
    {
        return mCharacterController.get();
    }

    int Actor::getGreetingTimer() const
    {
        return mGreetingTimer;
    }

    void Actor::setGreetingTimer(int timer)
    {
        mGreetingTimer = timer;
    }

    float Actor::getAngleToPlayer() const
    {
        return mTargetAngleRadians;
    }

    void Actor::setAngleToPlayer(float angle)
    {
        mTargetAngleRadians = angle;
    }

    GreetingState Actor::getGreetingState() const
    {
        return mGreetingState;
    }

    void Actor::setGreetingState(GreetingState state)
    {
        mGreetingState = state;
    }

    bool Actor::isTurningToPlayer() const
    {
        return mIsTurningToPlayer;
    }

    void Actor::setTurningToPlayer(bool turning)
    {
        mIsTurningToPlayer = turning;
    }
}
