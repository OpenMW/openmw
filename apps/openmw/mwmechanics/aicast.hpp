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

            static constexpr TypeId getTypeId() { return TypeIdCast; }

            MWWorld::Ptr getTarget() const final;

            static constexpr unsigned int defaultPriority() { return 3; }

            static constexpr bool defaultCanCancel() { return false; }

            static constexpr bool defaultShouldCancelPreviousAi() { return false; }

        private:
            std::string mTargetId;
            std::string mSpellId;
            bool mCasting;
            bool mManual;
            float mDistance;
    };
}

#endif
