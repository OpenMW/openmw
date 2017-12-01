#ifndef GAME_MWMECHANICS_AITRAVEL_H
#define GAME_MWMECHANICS_AITRAVEL_H

#include "aipackage.hpp"

#include "pathfinding.hpp"

namespace ESM
{
namespace AiSequence
{
    struct AiTravel;
}
}

namespace MWMechanics
{
    /// \brief Causes the AI to travel to the specified point
    class AiTravel : public AiPackage
    {
        public:
            /// Default constructor
            AiTravel(float x, float y, float z, bool hidden = false);
            AiTravel(const ESM::AiSequence::AiTravel* travel);

            /// Simulates the passing of time
            virtual void fastForward(const MWWorld::Ptr& actor, AiState& state);

            void writeState(ESM::AiSequence::AiSequence &sequence) const;

            virtual AiTravel *clone() const;

            virtual bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration);

            virtual int getTypeId() const;

        private:
            float mX;
            float mY;
            float mZ;

            bool mHidden;
    };
}

#endif
