#ifndef OPENMW_MECHANICS_ACTOR_H
#define OPENMW_MECHANICS_ACTOR_H

#include <memory>

#include "../mwmechanics/actorutil.hpp"

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
    class CharacterController;

    /// @brief Holds temporary state for an actor that will be discarded when the actor leaves the scene.
    class Actor
    {
    public:
        Actor(const MWWorld::Ptr& ptr, MWRender::Animation* animation);

        /// Notify this actor of its new base object Ptr, use when the object changed cells
        void updatePtr(const MWWorld::Ptr& newPtr);

        CharacterController* getCharacterController();

        int getGreetingTimer() const;
        void setGreetingTimer(int timer);

        float getAngleToPlayer() const;
        void setAngleToPlayer(float angle);

        GreetingState getGreetingState() const;
        void setGreetingState(GreetingState state);

        bool isTurningToPlayer() const;
        void setTurningToPlayer(bool turning);

        Misc::TimerStatus updateEngageCombatTimer(float duration)
        {
            return mEngageCombat.update(duration);
        }

    private:
        std::unique_ptr<CharacterController> mCharacterController;
        int mGreetingTimer{0};
        float mTargetAngleRadians{0.f};
        GreetingState mGreetingState{Greet_None};
        bool mIsTurningToPlayer{false};
        Misc::DeviatingPeriodicTimer mEngageCombat{1.0f, 0.25f, Misc::Rng::deviate(0, 0.25f)};
    };

}

#endif
