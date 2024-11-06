#ifndef OPENMW_MECHANICS_ACTOR_H
#define OPENMW_MECHANICS_ACTOR_H

#include <memory>

#include "character.hpp"
#include "creaturestats.hpp"
#include "greetingstate.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwworld/class.hpp"

#include <components/misc/timer.hpp>

namespace MWRender
{
    class Animation;
}
namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    /// @brief Holds temporary state for an actor that will be discarded when the actor leaves the scene.
    class Actor
    {
    public:
        Actor(const MWWorld::Ptr& ptr, MWRender::Animation* animation)
            : mCharacterController(ptr, animation)
            , mPositionAdjusted(ptr.getClass().getCreatureStats(ptr).getFallHeight() > 0)
        {
        }

        const MWWorld::Ptr& getPtr() const { return mCharacterController.getPtr(); }

        /// Notify this actor of its new base object Ptr, use when the object changed cells
        void updatePtr(const MWWorld::Ptr& newPtr) { mCharacterController.updatePtr(newPtr); }

        CharacterController& getCharacterController() { return mCharacterController; }
        const CharacterController& getCharacterController() const { return mCharacterController; }

        int getGreetingTimer() const { return mGreetingTimer; }
        void setGreetingTimer(int timer) { mGreetingTimer = timer; }

        float getAngleToPlayer() const { return mTargetAngleRadians; }
        void setAngleToPlayer(float angle) { mTargetAngleRadians = angle; }

        GreetingState getGreetingState() const { return mGreetingState; }
        void setGreetingState(GreetingState state) { mGreetingState = state; }

        bool isTurningToPlayer() const { return mIsTurningToPlayer; }
        void setTurningToPlayer(bool turning) { mIsTurningToPlayer = turning; }

        Misc::TimerStatus updateEngageCombatTimer(float duration)
        {
            return mEngageCombat.update(duration, MWBase::Environment::get().getWorld()->getPrng());
        }

        void setPositionAdjusted(bool adjusted) { mPositionAdjusted = adjusted; }
        bool getPositionAdjusted() const { return mPositionAdjusted; }

        void invalidate() { mInvalid = true; }
        bool isInvalid() const { return mInvalid; }

    private:
        CharacterController mCharacterController;
        int mGreetingTimer{ 0 };
        float mTargetAngleRadians{ 0.f };
        GreetingState mGreetingState{ Greet_None };
        Misc::DeviatingPeriodicTimer mEngageCombat{ 1.0f, 0.25f,
            Misc::Rng::deviate(0, 0.25f, MWBase::Environment::get().getWorld()->getPrng()) };
        bool mIsTurningToPlayer{ false };
        bool mInvalid{ false };
        bool mPositionAdjusted;
    };

}

#endif
