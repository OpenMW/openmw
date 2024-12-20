#include "aifollow.hpp"

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
#include "steering.hpp"

namespace
{
    osg::Vec3f::value_type getHalfExtents(const MWWorld::ConstPtr& actor)
    {
        if (actor.getClass().isNpc())
            return 64;
        return MWBase::Environment::get().getWorld()->getHalfExtents(actor).y();
    }
}

namespace MWMechanics
{
    int AiFollow::mFollowIndexCounter = 0;

    AiFollow::AiFollow(
        ESM::RefNum actor, std::string_view cellId, float duration, float x, float y, float z, bool repeat)
        : TypedAiPackage<AiFollow>(repeat)
        , mAlwaysFollow(false)
        , mDuration(duration)
        , mRemainingDuration(duration)
        , mX(x)
        , mY(y)
        , mZ(z)
        , mActive(false)
        , mFollowIndex(mFollowIndexCounter++)
    {
        mTargetActor = actor;
    }

    AiFollow::AiFollow(
        const ESM::RefId& actorId, std::string_view cellId, float duration, float x, float y, float z, bool repeat)
        : TypedAiPackage<AiFollow>(repeat)
        , mAlwaysFollow(false)
        , mDuration(duration)
        , mRemainingDuration(duration)
        , mX(x)
        , mY(y)
        , mZ(z)
        , mCellId(cellId)
        , mActive(false)
        , mFollowIndex(mFollowIndexCounter++)
    {
        mTargetActorRefId = actorId;
    }

    AiFollow::AiFollow(const MWWorld::Ptr& actor, bool commanded)
        : TypedAiPackage<AiFollow>(makeDefaultOptions().withShouldCancelPreviousAi(!commanded))
        , mAlwaysFollow(true)
        , mDuration(0)
        , mRemainingDuration(0)
        , mX(0)
        , mY(0)
        , mZ(0)
        , mActive(false)
        , mFollowIndex(mFollowIndexCounter++)
    {
        mTargetActorRefId = actor.getCellRef().getRefId();
        mTargetActor = actor.getCellRef().getRefNum();
    }

    AiFollow::AiFollow(const ESM::AiSequence::AiFollow* follow)
        : TypedAiPackage<AiFollow>(
              makeDefaultOptions().withShouldCancelPreviousAi(!follow->mCommanded).withRepeat(follow->mRepeat))
        , mAlwaysFollow(follow->mAlwaysFollow)
        , mDuration(follow->mData.mDuration)
        , mRemainingDuration(follow->mRemainingDuration)
        , mX(follow->mData.mX)
        , mY(follow->mData.mY)
        , mZ(follow->mData.mZ)
        , mCellId(follow->mCellId)
        , mActive(follow->mActive)
        , mFollowIndex(mFollowIndexCounter++)
    {
        mTargetActorRefId = follow->mTargetId;
        mTargetActor = follow->mTargetActor;
    }

    bool AiFollow::execute(
        const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
    {
        const MWWorld::Ptr target = getTarget();

        // Target is not here right now, wait for it to return
        // Really we should be checking whether the target is currently registered with the MechanicsManager
        if (target == MWWorld::Ptr() || !target.getCellRef().getCount() || !target.getRefData().isEnabled())
            return false;

        actor.getClass().getCreatureStats(actor).setDrawState(DrawState::Nothing);

        AiFollowStorage& storage = state.get<AiFollowStorage>();

        bool& rotate = storage.mTurnActorToTarget;
        if (rotate)
        {
            if (zTurn(actor, storage.mTargetAngleRadians))
                rotate = false;

            return false;
        }

        const osg::Vec3f actorPos(actor.getRefData().getPosition().asVec3());
        const osg::Vec3f targetPos(target.getRefData().getPosition().asVec3());
        const osg::Vec3f targetDir = targetPos - actorPos;

        // In the original engine the first follower stays closer to the player than any subsequent followers.
        // Followers beyond the first usually attempt to stand inside each other.
        osg::Vec3f::value_type floatingDistance = 0;
        auto followers = MWBase::Environment::get().getMechanicsManager()->getActorsFollowingByIndex(target);
        if (followers.size() >= 2 && followers.cbegin()->first != mFollowIndex)
        {
            for (auto& follower : followers)
            {
                auto halfExtent = getHalfExtents(follower.second);
                if (halfExtent > floatingDistance)
                    floatingDistance = halfExtent;
            }
            floatingDistance += 128;
        }
        floatingDistance += getHalfExtents(target) + 64;
        floatingDistance += getHalfExtents(actor) * 2;
        short followDistance = static_cast<short>(floatingDistance);

        // AiFollow requires the target to be in range and within sight for the initial activation
        if (!mActive)
        {
            storage.mTimer -= duration;

            if (storage.mTimer < 0)
            {
                float activeRange = followDistance + 384.f;
                if (targetDir.length2() < activeRange * activeRange
                    && MWBase::Environment::get().getWorld()->getLOS(actor, target))
                    mActive = true;
                storage.mTimer = 0.5f;
            }
        }
        if (!mActive)
            return false;

        if (!mAlwaysFollow) // Update if you only follow for a bit
        {
            // Check if we've run out of time
            if (mDuration > 0)
            {
                mRemainingDuration
                    -= ((duration * MWBase::Environment::get().getWorld()->getTimeManager()->getGameTimeScale())
                        / 3600);
                if (mRemainingDuration <= 0)
                {
                    mRemainingDuration = mDuration;
                    return true;
                }
            }

            osg::Vec3f finalPos(mX, mY, mZ);
            if ((actorPos - finalPos).length2() < followDistance * followDistance) // Close-ish to final position
            {
                if (actor.getCell()->isExterior()) // Outside?
                {
                    if (mCellId.empty()) // No cell to travel to
                    {
                        mRemainingDuration = mDuration;
                        return true;
                    }
                }
                else if (mCellId == actor.getCell()->getCell()->getWorldSpace()) // Cell to travel to
                {
                    mRemainingDuration = mDuration;
                    return true;
                }
            }
        }

        short baseFollowDistance = followDistance;
        short threshold = 30; // to avoid constant switching between moving/stopping
        if (storage.mMoving)
            followDistance -= threshold;
        else
            followDistance += threshold;

        if (targetDir.length2() <= followDistance * followDistance)
        {
            float faceAngleRadians = std::atan2(targetDir.x(), targetDir.y());

            if (!zTurn(actor, faceAngleRadians, osg::DegreesToRadians(45.f)))
            {
                storage.mTargetAngleRadians = faceAngleRadians;
                storage.mTurnActorToTarget = true;
            }

            return false;
        }

        // Go to the destination
        storage.mMoving = !pathTo(
            actor, targetPos, duration, characterController.getSupportedMovementDirections(), baseFollowDistance);

        if (storage.mMoving)
        {
            // Check if you're far away
            if (targetDir.length2() > 450 * 450)
                actor.getClass().getCreatureStats(actor).setMovementFlag(
                    MWMechanics::CreatureStats::Flag_Run, true); // Make NPC run
            else if (targetDir.length2()
                < 325 * 325) // Have a bit of a dead zone, otherwise npc will constantly flip between running and not
                             // when right on the edge of the running threshold
                actor.getClass().getCreatureStats(actor).setMovementFlag(
                    MWMechanics::CreatureStats::Flag_Run, false); // make NPC walk
        }

        return false;
    }

    ESM::RefId AiFollow::getFollowedActor()
    {
        return mTargetActorRefId;
    }

    bool AiFollow::isCommanded() const
    {
        return !mOptions.mShouldCancelPreviousAi;
    }

    void AiFollow::writeState(ESM::AiSequence::AiSequence& sequence) const
    {
        auto follow = std::make_unique<ESM::AiSequence::AiFollow>();
        follow->mData.mX = mX;
        follow->mData.mY = mY;
        follow->mData.mZ = mZ;
        follow->mData.mDuration = static_cast<int16_t>(mDuration);
        follow->mTargetId = mTargetActorRefId;
        follow->mTargetActor = mTargetActor;
        follow->mRemainingDuration = mRemainingDuration;
        follow->mCellId = mCellId;
        follow->mAlwaysFollow = mAlwaysFollow;
        follow->mCommanded = isCommanded();
        follow->mActive = mActive;
        follow->mRepeat = getRepeat();

        ESM::AiSequence::AiPackageContainer package;
        package.mType = ESM::AiSequence::Ai_Follow;
        package.mPackage = std::move(follow);
        sequence.mPackages.push_back(std::move(package));
    }

    int AiFollow::getFollowIndex() const
    {
        return mFollowIndex;
    }

    void AiFollow::fastForward(const MWWorld::Ptr& actor, AiState& state)
    {
        // Update duration counter if this package has a duration
        if (mDuration > 0)
            mRemainingDuration--;
    }

}
