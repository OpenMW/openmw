#include "aifollow.hpp"

#include <components/esm/aisequence.hpp>
#include <components/esm/loadcell.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"

#include "creaturestats.hpp"
#include "movement.hpp"
#include "steering.hpp"

namespace MWMechanics
{

int AiFollow::mFollowIndexCounter = 0;

AiFollow::AiFollow(const std::string &actorId, float duration, float x, float y, float z)
: mAlwaysFollow(false), mCommanded(false), mDuration(duration), mRemainingDuration(duration), mX(x), mY(y), mZ(z)
, mCellId(""), mActive(false), mFollowIndex(mFollowIndexCounter++)
{
    mTargetActorRefId = actorId;
}

AiFollow::AiFollow(const std::string &actorId, const std::string &cellId, float duration, float x, float y, float z)
: mAlwaysFollow(false), mCommanded(false), mDuration(duration), mRemainingDuration(duration), mX(x), mY(y), mZ(z)
, mCellId(cellId), mActive(false), mFollowIndex(mFollowIndexCounter++)
{
    mTargetActorRefId = actorId;
}

AiFollow::AiFollow(const MWWorld::Ptr& actor, float duration, float x, float y, float z)
: mAlwaysFollow(false), mCommanded(false), mDuration(duration), mRemainingDuration(duration), mX(x), mY(y), mZ(z)
, mCellId(""), mActive(false), mFollowIndex(mFollowIndexCounter++)
{
    mTargetActorRefId = actor.getCellRef().getRefId();
    mTargetActorId = actor.getClass().getCreatureStats(actor).getActorId();
}

AiFollow::AiFollow(const MWWorld::Ptr& actor, const std::string &cellId, float duration, float x, float y, float z)
: mAlwaysFollow(false), mCommanded(false), mDuration(duration), mRemainingDuration(duration), mX(x), mY(y), mZ(z)
, mCellId(cellId), mActive(false), mFollowIndex(mFollowIndexCounter++)
{
    mTargetActorRefId = actor.getCellRef().getRefId();
    mTargetActorId = actor.getClass().getCreatureStats(actor).getActorId();
}

AiFollow::AiFollow(const MWWorld::Ptr& actor, bool commanded)
: mAlwaysFollow(true), mCommanded(commanded), mDuration(0), mRemainingDuration(0), mX(0), mY(0), mZ(0)
, mCellId(""), mActive(false), mFollowIndex(mFollowIndexCounter++)
{
    mTargetActorRefId = actor.getCellRef().getRefId();
    mTargetActorId = actor.getClass().getCreatureStats(actor).getActorId();
}

AiFollow::AiFollow(const ESM::AiSequence::AiFollow *follow)
    : mAlwaysFollow(follow->mAlwaysFollow), mCommanded(follow->mCommanded), mRemainingDuration(follow->mRemainingDuration)
    , mX(follow->mData.mX), mY(follow->mData.mY), mZ(follow->mData.mZ)
    , mCellId(follow->mCellId), mActive(follow->mActive), mFollowIndex(mFollowIndexCounter++)
{
    mTargetActorRefId = follow->mTargetId;
    mTargetActorId = follow->mTargetActorId;
    // mDuration isn't saved in the save file, so just giving it "1" for now if the package had a duration.
    // The exact value of mDuration only matters for repeating packages.
    if (mRemainingDuration > 0) // Previously mRemainingDuration could be negative even when mDuration was 0. Checking for > 0 should fix old saves.
       mDuration = 1;
    else
       mDuration = 0;
}

bool AiFollow::execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
{
    const MWWorld::Ptr target = getTarget();

    // Target is not here right now, wait for it to return
    // Really we should be checking whether the target is currently registered with the MechanicsManager
    if (target == MWWorld::Ptr() || !target.getRefData().getCount() || !target.getRefData().isEnabled())
        return false;

    actor.getClass().getCreatureStats(actor).setDrawState(DrawState_Nothing);

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

    // AiFollow requires the target to be in range and within sight for the initial activation
    if (!mActive)
    {
        storage.mTimer -= duration;

        if (storage.mTimer < 0)
        {
            if (targetDir.length2() < 500*500 && MWBase::Environment::get().getWorld()->getLOS(actor, target))
                mActive = true;
            storage.mTimer = 0.5f;
        }
    }
    if (!mActive)
        return false;

    // The distances below are approximations based on observations of the original engine.
    // If only one actor is following the target, it uses 186.
    // If there are multiple actors following the same target, they form a group with each group member at 313 + (130 * i) distance to the target.

    short followDistance = 186;
    std::list<int> followers = MWBase::Environment::get().getMechanicsManager()->getActorsFollowingIndices(target);
    if (followers.size() >= 2)
    {
        followDistance = 313;
        short i = 0;
        followers.sort();
        for (std::list<int>::iterator it = followers.begin(); it != followers.end(); ++it)
        {
            if (*it == mFollowIndex)
                followDistance += 130 * i;
            ++i;
        }
    }

    if (!mAlwaysFollow) //Update if you only follow for a bit
    {
         //Check if we've run out of time
        if (mDuration > 0)
        {
            mRemainingDuration -= ((duration*MWBase::Environment::get().getWorld()->getTimeScaleFactor()) / 3600);
            if (mRemainingDuration <= 0)
            {
                mRemainingDuration = mDuration;
                return true;
            }
        }

        osg::Vec3f finalPos(mX, mY, mZ);
        if ((actorPos-finalPos).length2() < followDistance*followDistance) //Close-ish to final position
        {
            if (actor.getCell()->isExterior()) //Outside?
            {
                if (mCellId == "") //No cell to travel to
                    return true;
            }
            else
            {
                if (mCellId == actor.getCell()->getCell()->mName) //Cell to travel to
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

    storage.mMoving = !pathTo(actor, targetPos, duration, baseFollowDistance); // Go to the destination

    if (storage.mMoving)
    {
        //Check if you're far away
        if (targetDir.length2() > 450 * 450)
            actor.getClass().getCreatureStats(actor).setMovementFlag(MWMechanics::CreatureStats::Flag_Run, true); //Make NPC run
        else if (targetDir.length2() < 325 * 325) //Have a bit of a dead zone, otherwise npc will constantly flip between running and not when right on the edge of the running threshold
            actor.getClass().getCreatureStats(actor).setMovementFlag(MWMechanics::CreatureStats::Flag_Run, false); //make NPC walk
    }

    return false;
}

std::string AiFollow::getFollowedActor()
{
    return mTargetActorRefId;
}

AiFollow *MWMechanics::AiFollow::clone() const
{
    return new AiFollow(*this);
}

int AiFollow::getTypeId() const
{
    return TypeIdFollow;
}

bool AiFollow::isCommanded() const
{
    return mCommanded;
}

void AiFollow::writeState(ESM::AiSequence::AiSequence &sequence) const
{
    std::unique_ptr<ESM::AiSequence::AiFollow> follow(new ESM::AiSequence::AiFollow());
    follow->mData.mX = mX;
    follow->mData.mY = mY;
    follow->mData.mZ = mZ;
    follow->mTargetId = mTargetActorRefId;
    follow->mTargetActorId = mTargetActorId;
    follow->mRemainingDuration = mRemainingDuration;
    follow->mCellId = mCellId;
    follow->mAlwaysFollow = mAlwaysFollow;
    follow->mCommanded = mCommanded;
    follow->mActive = mActive;

    ESM::AiSequence::AiPackageContainer package;
    package.mType = ESM::AiSequence::Ai_Follow;
    package.mPackage = follow.release();
    sequence.mPackages.push_back(package);
}

int AiFollow::getFollowIndex() const
{
    return mFollowIndex;
}

void AiFollow::fastForward(const MWWorld::Ptr& actor, AiState &state)
{
    // Update duration counter if this package has a duration
    if (mDuration > 0)
        mRemainingDuration--;
}

}
