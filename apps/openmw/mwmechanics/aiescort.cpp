#include "aiescort.hpp"

#include <components/esm/util.hpp>
#include <components/esm3/aisequence.hpp>
#include <components/esm3/loadcell.hpp>
#include <components/misc/algorithm.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/datetimemanager.hpp"

#include "character.hpp"
#include "creaturestats.hpp"
#include "movement.hpp"

/*
    TODO: Different behavior for AIEscort a d x y z and AIEscortCell a c d x y z.
    TODO: Take account for actors being in different cells.
*/

namespace MWMechanics
{
    AiEscort::AiEscort(const ESM::RefId& actorId, int duration, float x, float y, float z, bool repeat)
        : TypedAiPackage<AiEscort>(repeat)
        , mX(x)
        , mY(y)
        , mZ(z)
        , mDuration(duration)
        , mRemainingDuration(static_cast<float>(duration))
    {
        mTargetActorRefId = actorId;
    }

    AiEscort::AiEscort(
        const ESM::RefId& actorId, std::string_view cellId, int duration, float x, float y, float z, bool repeat)
        : TypedAiPackage<AiEscort>(repeat)
        , mCellId(cellId)
        , mX(x)
        , mY(y)
        , mZ(z)
        , mDuration(duration)
        , mRemainingDuration(static_cast<float>(duration))
    {
        mTargetActorRefId = actorId;
    }

    AiEscort::AiEscort(const ESM::AiSequence::AiEscort* escort)
        : TypedAiPackage<AiEscort>(escort->mRepeat)
        , mCellId(escort->mCellId)
        , mX(escort->mData.mX)
        , mY(escort->mData.mY)
        , mZ(escort->mData.mZ)
        , mDuration(escort->mData.mDuration)
        , mRemainingDuration(escort->mRemainingDuration)
    {
        mTargetActorRefId = escort->mTargetId;
        mTargetActorId = escort->mTargetActorId;
    }

    bool AiEscort::execute(
        const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
    {
        // If AiEscort has ran for as long or longer then the duration specified
        // and the duration is not infinite, the package is complete.
        if (mDuration > 0)
        {
            mRemainingDuration
                -= ((duration * MWBase::Environment::get().getWorld()->getTimeManager()->getGameTimeScale()) / 3600);
            if (mRemainingDuration <= 0)
            {
                mRemainingDuration = mDuration;
                return true;
            }
        }

        if (!mCellId.empty() && !Misc::StringUtils::ciEqual(mCellId, actor.getCell()->getCell()->getNameId()))
            return false; // Not in the correct cell, pause and rely on the player to go back through a teleport door

        actor.getClass().getCreatureStats(actor).setDrawState(DrawState::Nothing);
        actor.getClass().getCreatureStats(actor).setMovementFlag(CreatureStats::Flag_Run, false);

        const MWWorld::Ptr follower = MWBase::Environment::get().getWorld()->getPtr(mTargetActorRefId, false);
        const osg::Vec3f leaderPos = actor.getRefData().getPosition().asVec3();
        const osg::Vec3f followerPos = follower.getRefData().getPosition().asVec3();
        const osg::Vec3f halfExtents = MWBase::Environment::get().getWorld()->getHalfExtents(actor);
        const float maxHalfExtent = std::max(halfExtents.x(), std::max(halfExtents.y(), halfExtents.z()));

        if ((leaderPos - followerPos).length2() <= mMaxDist * mMaxDist)
        {
            // TESCS allows the creation of Escort packages without a specific destination
            constexpr float nowhere = std::numeric_limits<float>::max();
            if (mX == nowhere || mY == nowhere)
                return true;
            if (mZ == nowhere)
            {
                if (mCellId.empty()
                    && ESM::positionToExteriorCellLocation(mX, mY)
                        == actor.getCell()->getCell()->getExteriorCellLocation())
                    return false;
                return true;
            }

            const osg::Vec3f dest(mX, mY, mZ);
            if (pathTo(actor, dest, duration, characterController.getSupportedMovementDirections(), maxHalfExtent))
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

    void AiEscort::writeState(ESM::AiSequence::AiSequence& sequence) const
    {
        auto escort = std::make_unique<ESM::AiSequence::AiEscort>();
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

    void AiEscort::fastForward(const MWWorld::Ptr& actor, AiState& state)
    {
        // Update duration counter if this package has a duration
        if (mDuration > 0)
            mRemainingDuration--;
    }
}
