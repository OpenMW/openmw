#ifndef GAME_MWMECHANICS_AITRAVEL_H
#define GAME_MWMECHANICS_AITRAVEL_H

#include "typedaipackage.hpp"

namespace ESM
{
namespace AiSequence
{
    struct AiTravel;
}
}

namespace MWMechanics
{
    struct AiInternalTravel;

    /// \brief Causes the AI to travel to the specified point
    class AiTravel : public TypedAiPackage<AiTravel>
    {
        public:
            AiTravel(float x, float y, float z, AiTravel* derived);

            AiTravel(float x, float y, float z, AiInternalTravel* derived);

            AiTravel(float x, float y, float z);

            AiTravel(const ESM::AiSequence::AiTravel* travel);

            /// Simulates the passing of time
            void fastForward(const MWWorld::Ptr& actor, AiState& state) final;

            void writeState(ESM::AiSequence::AiSequence &sequence) const final;

            bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration) final;

            static constexpr AiPackageTypeId getTypeId() { return AiPackageTypeId::Travel; }

            static constexpr Options makeDefaultOptions()
            {
                AiPackage::Options options;
                options.mUseVariableSpeed = true;
                options.mAlwaysActive = true;
                return options;
            }

            osg::Vec3f getDestination() const final { return osg::Vec3f(mX, mY, mZ); }

        private:
            const float mX;
            const float mY;
            const float mZ;

            const bool mHidden;
    };

    struct AiInternalTravel final : public AiTravel
    {
        AiInternalTravel(float x, float y, float z);

        explicit AiInternalTravel(const ESM::AiSequence::AiTravel* travel);

        static constexpr AiPackageTypeId getTypeId() { return AiPackageTypeId::InternalTravel; }

        std::unique_ptr<AiPackage> clone() const final;
    };
}

#endif
