#ifndef GAME_MWMECHANICS_AITRAVEL_H
#define GAME_MWMECHANICS_AITRAVEL_H

#include "aipackage.hpp"

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
    class AiTravel final : public AiPackage
    {
        public:
            /// Default constructor
            AiTravel(float x, float y, float z, bool hidden = false);
            AiTravel(const ESM::AiSequence::AiTravel* travel);

            /// Simulates the passing of time
            void fastForward(const MWWorld::Ptr& actor, AiState& state) final;

            void writeState(ESM::AiSequence::AiSequence &sequence) const final;

            AiTravel *clone() const final;

            bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration) final;

            int getTypeId() const final;

            bool useVariableSpeed() const final { return true; }

            bool alwaysActive() const final { return true; }

            osg::Vec3f getDestination() const final { return osg::Vec3f(mX, mY, mZ); }

        private:
            float mX;
            float mY;
            float mZ;

            bool mHidden;
    };
}

#endif
