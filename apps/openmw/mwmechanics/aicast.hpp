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
    class AiCast : public AiPackage {
        public:
            AiCast(const std::string& targetId, const std::string& spellId, bool manualSpell=false);

            virtual AiPackage *clone() const;

            virtual bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration);

            virtual int getTypeId() const;

            virtual MWWorld::Ptr getTarget() const;

            virtual unsigned int getPriority() const;

            virtual bool canCancel() const { return false; }
            virtual bool shouldCancelPreviousAi() const { return false; }

        private:
            std::string mTargetId;
            std::string mSpellId;
            bool mCasting;
            bool mManual;
            float mDistance;
    };
}

#endif
