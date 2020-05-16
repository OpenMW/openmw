#ifndef GAME_MWMECHANICS_AICAST_H
#define GAME_MWMECHANICS_AICAST_H

#include "aipackage.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    /// AiPackage which makes an actor to cast given spell.
    class AiCast final : public AiPackage {
        public:
            AiCast(const std::string& targetId, const std::string& spellId, bool manualSpell=false);

            AiPackage *clone() const final;

            bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration) final;

            int getTypeId() const final;

            MWWorld::Ptr getTarget() const final;

            unsigned int getPriority() const final;

            bool canCancel() const final { return false; }
            bool shouldCancelPreviousAi() const final { return false; }

        private:
            std::string mTargetId;
            std::string mSpellId;
            bool mCasting;
            bool mManual;
            float mDistance;
    };
}

#endif
