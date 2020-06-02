#ifndef GAME_MWMECHANICS_AICAST_H
#define GAME_MWMECHANICS_AICAST_H

#include "typedaipackage.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    /// AiPackage which makes an actor to cast given spell.
    class AiCast final : public TypedAiPackage<AiCast> {
        public:
            AiCast(const std::string& targetId, const std::string& spellId, bool manualSpell=false);

            bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration) final;

            int getTypeId() const final;

            MWWorld::Ptr getTarget() const final;

            unsigned int getPriority() const final;

            bool canCancel() const final { return false; }
            bool shouldCancelPreviousAi() const final { return false; }

        private:
            const std::string mTargetId;
            const std::string mSpellId;
            bool mCasting;
            const bool mManual;
            const float mDistance;
    };
}

#endif
