#include "aiescort.hpp"

#include <components/esm3/aisequence.hpp>
#include <components/esm3/loadcell.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"

#include "creaturestats.hpp"
#include "movement.hpp"

/*
    TODO: Different behavior for AIEscort a d x y z and AIEscortCell a c d x y z.
    TODO: Take account for actors being in different cells.
*/

namespace MWMechanics
{
    AiEscort::AiEscort(std::string_view actorId, int duration, float x, float y, float z, bool repeat)
    : TypedAiPackage<AiEscort>(repeat), mX(x), mY(y), mZ(z), mDuration(duration), mRemainingDuration(static_cast<float>(duration))
    , mCellX(std::numeric_limits<int>::max())
    , mCellY(std::numeric_limits<int>::max())
    {
        mTargetActorRefId = std::string(actorId);
    }

    AiEscort::AiEscort(std::string_view actorId, std::string_view cellId, int duration, float x, float y, float z, bool repeat)
    : TypedAiPackage<AiEscort>(repeat), mCellId(cellId), mX(x), mY(y), mZ(z), mDuration(duration), mRemainingDuration(static_cast<float>(duration))
    , mCellX(std::numeric_limits<int>::max())
    , mCellY(std::numeric_limits<int>::max())
    {
        mTargetActorRefId = std::string(actorId);
    }

    AiEscort::AiEscort(const ESM::AiSequence::AiEscort *escort)
        : TypedAiPackage<AiEscort>(escort->mRepeat), mCellId(escort->mCellId), mX(escort->mData.mX), mY(escort->mData.mY), mZ(escort->mData.mZ)
        , mDuration(escort->mData.mDuration)
        , mRemainingDuration(escort->mRemainingDuration)
        , mCellX(std::numeric_limits<int>::max())
        , mCellY(std::numeric_limits<int>::max())
    {
        mTargetActorRefId = escort->mTargetId;
        mTargetActorId = escort->mTargetActorId;
    }

    bool AiEscort::execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
    {
        // If AiEscort has ran for as long or longer then the duration specified
        // and the duration is not infinite, the package is complete.
        if (mDuration > 0)
        {
            mRemainingDuration -= ((duration*MWBase::Environment::get().getWorld()->getTimeScaleFactor()) / 3600);
            if (mRemainingDuration <= 0)
            {
                mRemainingDuration = mDuration;
                return true;
            }
        }

        if (!mCellId.empty() && mCellId != actor.getCell()->getCell()->getCellId().mWorldspace)
            return false; // Not in the correct cell, pause and rely on the player to go back through a teleport door

        actor.getClass().getCreatureStats(actor).setDrawState(DrawState_Nothing);
        actor.getClass().getCreatureStats(actor).setMovementFlag(CreatureStats::Flag_Run, false);

        const MWWorld::Ptr follower = MWBase::Environment::get().getWorld()->getPtr(mTargetActorRefId, false);
        const osg::Vec3f leaderPos = actor.getRefData().getPosition().asVec3();
        const osg::Vec3f followerPos = follower.getRefData().getPosition().asVec3();
        const osg::Vec3f halfExtents = MWBase::Environment::get().getWorld()->getHalfExtents(actor);
        const float maxHalfExtent = std::max(halfExtents.x(), std::max(halfExtents.y(), halfExtents.z()));

        if ((leaderPos - followerPos).length2() <= mMaxDist * mMaxDist)
        {
            const osg::Vec3f dest(mX, mY, mZ);
            if (pathTo(actor, dest, duration, maxHalfExtent)) //Returns true on path complete
            {
                mRemainingDuration = mDuration;
                return true;
            }
            mMaxDist = maxHalfExtent + 450.0f;
        }
        else
        {
            // Stop moving if the player is too far away
            MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(actor, "idle3", 0, 1);
            actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
            mMaxDist = maxHalfExtent + 250.0f;
        }

        return false;
    }

    void AiEscort::writeState(ESM::AiSequence::AiSequence &sequence) const
    {
        std::unique_ptr<ESM::AiSequence::AiEscort> escort(new ESM::AiSequence::AiEscort());
        escort->mData.mX = mX;
        escort->mData.mY = mY;
        escort->mData.mZ = mZ;
        escort->mData.mDuration = mDuration;
        escort->mTargetId = mTargetActorRefId;
        escort->mTargetActorId = mTargetActorId;
        escort->mRemainingDuration = mRemainingDuration;
        escort->mCellId = mCellId;
        escort->mRepeat = getRepeat();

        ESM::AiSequence::AiPackageContainer package;
        package.mType = ESM::AiSequence::Ai_Escort;
        package.mPackage = std::move(escort);
        sequence.mPackages.push_back(std::move(package));
    }

    void AiEscort::fastForward(const MWWorld::Ptr& actor, AiState &state)
    {
        // Update duration counter if this package has a duration
        if (mDuration > 0)
            mRemainingDuration--;
    }
}

