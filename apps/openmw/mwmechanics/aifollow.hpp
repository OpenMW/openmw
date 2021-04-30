#ifndef GAME_MWMECHANICS_AIFOLLOW_H
#define GAME_MWMECHANICS_AIFOLLOW_H

#include "typedaipackage.hpp"

#include <string>

#include <components/esm/defs.hpp>

#include "../mwworld/ptr.hpp"

namespace ESM
{
namespace AiSequence
{
    struct AiFollow;
}
}

namespace MWMechanics
{
    struct AiFollowStorage : AiTemporaryBase
    {
        float mTimer;
        bool mMoving;
        float mTargetAngleRadians;
        bool mTurnActorToTarget;

        AiFollowStorage() :
            mTimer(0.f),
            mMoving(false),
            mTargetAngleRadians(0.f),
            mTurnActorToTarget(false)
        {}
    };

    /// \brief AiPackage for an actor to follow another actor/the PC
    /** The AI will follow the target until a condition (time, or position) are set. Both can be disabled to cause the actor to follow the other indefinitely
    **/
    class AiFollow final : public TypedAiPackage<AiFollow>
    {
        public:
            AiFollow(const std::string &actorId, float duration, float x, float y, float z);
            AiFollow(const std::string &actorId, const std::string &CellId, float duration, float x, float y, float z);
            /// Follow Actor for duration or until you arrive at a world position
            AiFollow(const MWWorld::Ptr& actor, float duration, float X, float Y, float Z);
            /// Follow Actor for duration or until you arrive at a position in a cell
            AiFollow(const MWWorld::Ptr& actor, const std::string &CellId, float duration, float X, float Y, float Z);
            /// Follow Actor indefinitively
            AiFollow(const MWWorld::Ptr& actor, bool commanded=false);

            AiFollow(const ESM::AiSequence::AiFollow* follow);

            bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration) override;

            static constexpr AiPackageTypeId getTypeId() { return AiPackageTypeId::Follow; }

            static constexpr Options makeDefaultOptions()
            {
                AiPackage::Options options;
                options.mUseVariableSpeed = true;
                options.mSideWithTarget = true;
                options.mFollowTargetThroughDoors = true;
                return options;
            }

            /// Returns the actor being followed
            std::string getFollowedActor();

            void writeState (ESM::AiSequence::AiSequence& sequence) const override;

            bool isCommanded() const;

            int getFollowIndex() const;

            void fastForward(const MWWorld::Ptr& actor, AiState& state) override;

            osg::Vec3f getDestination() const override
            {
                MWWorld::Ptr target = getTarget();
                if (target.isEmpty())
                    return osg::Vec3f(0, 0, 0);

                return target.getRefData().getPosition().asVec3();
            }

        private:
            /// This will make the actor always follow.
            /** Thus ignoring mDuration and mX,mY,mZ (used for summoned creatures). **/
            const bool mAlwaysFollow;
            const float mDuration; // Hours
            float mRemainingDuration; // Hours
            const float mX;
            const float mY;
            const float mZ;
            const std::string mCellId;
            bool mActive; // have we spotted the target?
            const int mFollowIndex;

            static int mFollowIndexCounter;
    };
}
#endif
