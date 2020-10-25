#ifndef GAME_MWMECHANICS_AIESCORT_H
#define GAME_MWMECHANICS_AIESCORT_H

#include "typedaipackage.hpp"

#include <string>

namespace ESM
{
namespace AiSequence
{
    struct AiEscort;
}
}

namespace MWMechanics
{
    /// \brief AI Package to have an NPC lead the player to a specific point
    class AiEscort final : public TypedAiPackage<AiEscort>
    {
        public:
            /// Implementation of AiEscort
            /** The Actor will escort the specified actor to the world position x, y, z until they reach their position, or they run out of time
                \implement AiEscort **/
            AiEscort(const std::string &actorId, int duration, float x, float y, float z);
            /// Implementation of AiEscortCell
            /** The Actor will escort the specified actor to the cell position x, y, z until they reach their position, or they run out of time
                \implement AiEscortCell **/
            AiEscort(const std::string &actorId, const std::string &cellId, int duration, float x, float y, float z);

            AiEscort(const ESM::AiSequence::AiEscort* escort);

            bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration) override;

            static constexpr AiPackageTypeId getTypeId() { return AiPackageTypeId::Escort; }

            static constexpr Options makeDefaultOptions()
            {
                AiPackage::Options options;
                options.mUseVariableSpeed = true;
                options.mSideWithTarget = true;
                return options;
            }

            void writeState(ESM::AiSequence::AiSequence &sequence) const override;

            void fastForward(const MWWorld::Ptr& actor, AiState& state) override;

            osg::Vec3f getDestination() const override { return osg::Vec3f(mX, mY, mZ); }

        private:
            const std::string mCellId;
            const float mX;
            const float mY;
            const float mZ;
            float mMaxDist = 450;
            const float mDuration; // In hours
            float mRemainingDuration; // In hours

            const int mCellX;
            const int mCellY;
    };
}
#endif
