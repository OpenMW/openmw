#ifndef GAME_MWMECHANICS_AIBREATHE_H
#define GAME_MWMECHANICS_AIBREATHE_H

#include "aipackage.hpp"

namespace MWMechanics
{
    /// \brief AiPackage to have an actor resurface to breathe
    // The AI will go up if lesser than half breath left
    class AiBreathe : public AiPackage
    {
        public:
            AiBreathe();

            virtual AiBreathe *clone() const;

            virtual bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration);

            virtual int getTypeId() const;

            virtual unsigned int getPriority() const;

            virtual bool canCancel() const { return false; }
            virtual bool shouldCancelPreviousAi() const { return false; }
    };
}
#endif

