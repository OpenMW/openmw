#ifndef GAME_MWMECHANICS_AIBREATHE_H
#define GAME_MWMECHANICS_AIBREATHE_H

#include "aipackage.hpp"

namespace MWMechanics
{
    /// \brief AiPackage to have an actor resurface to breathe
    // The AI will go up if lesser than half breath left
    class AiBreathe final : public AiPackage
    {
        public:
            AiBreathe();

            AiBreathe *clone() const final;

            bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration) final;

            int getTypeId() const final;

            unsigned int getPriority() const final;

            bool canCancel() const final { return false; }
            bool shouldCancelPreviousAi() const final { return false; }
    };
}
#endif

