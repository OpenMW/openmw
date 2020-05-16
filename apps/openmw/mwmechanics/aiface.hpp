#ifndef GAME_MWMECHANICS_AIFACE_H
#define GAME_MWMECHANICS_AIFACE_H

#include "aipackage.hpp"

namespace MWMechanics
{
    /// AiPackage which makes an actor face a certain direction.
    class AiFace final : public AiPackage {
        public:
            AiFace(float targetX, float targetY);

            AiPackage *clone() const final;

            bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration) final;

            int getTypeId() const final;

            unsigned int getPriority() const final;

            bool canCancel() const final { return false; }
            bool shouldCancelPreviousAi() const final { return false; }

        private:
            float mTargetX, mTargetY;
    };
}

#endif
