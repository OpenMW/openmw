#ifndef GAME_MWMECHANICS_AIFACE_H
#define GAME_MWMECHANICS_AIFACE_H

#include "typedaipackage.hpp"

namespace MWMechanics
{
    /// AiPackage which makes an actor face a certain direction.
    class AiFace final : public TypedAiPackage<AiFace> {
        public:
            AiFace(float targetX, float targetY);

            bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration) override;

            static constexpr AiPackageTypeId getTypeId() { return AiPackageTypeId::Face; }

            static constexpr Options makeDefaultOptions()
            {
                AiPackage::Options options;
                options.mPriority = 2;
                options.mCanCancel = false;
                options.mShouldCancelPreviousAi = false;
                return options;
            }

        private:
            const float mTargetX;
            const float mTargetY;
    };
}

#endif
