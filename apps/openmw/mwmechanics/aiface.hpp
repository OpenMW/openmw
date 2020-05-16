#ifndef GAME_MWMECHANICS_AIFACE_H
#define GAME_MWMECHANICS_AIFACE_H

#include "typedaipackage.hpp"

namespace MWMechanics
{
    /// AiPackage which makes an actor face a certain direction.
    class AiFace final : public TypedAiPackage<AiFace> {
        public:
            AiFace(float targetX, float targetY);

            bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration) final;

            static constexpr TypeId getTypeId() { return TypeIdFace; }

            static constexpr unsigned int defaultPriority() { return 2; }

            static constexpr bool defaultCanCancel() { return false; }

            static constexpr bool defaultShouldCancelPreviousAi() { return false; }

        private:
            float mTargetX, mTargetY;
    };
}

#endif
