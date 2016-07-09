#ifndef GAME_MWMECHANICS_AIFACE_H
#define GAME_MWMECHANICS_AIFACE_H

#include "aipackage.hpp"

namespace MWMechanics
{
    /// AiPackage which makes an actor face a certain direction.
    class AiFace : public AiPackage {
        public:
            AiFace(float targetX, float targetY);

            virtual AiPackage *clone() const;

            virtual bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration);

            virtual int getTypeId() const;

            virtual unsigned int getPriority() const;

            virtual bool canCancel() const { return false; }
            virtual bool shouldCancelPreviousAi() const { return false; }

        private:
            float mTargetX, mTargetY;
    };
}

#endif
