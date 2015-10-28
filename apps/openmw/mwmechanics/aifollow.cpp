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


struct AiFollowStorage : AiTemporaryBase
{
    float mTimer;
    bool mMoving;

    AiFollowStorage() : mTimer(0.f), mMoving(false) {}
};

int AiFollow::mFollowIndexCounter = 0;

AiFollow::AiFollow(const std::string &actorId,float duration, float x, float y, float z)
: mAlwaysFollow(false), mCommanded(false), mRemainingDuration(duration), mX(x), mY(y), mZ(z)
, mActorRefId(actorId), mActorId(-1), mCellId(""), mActive(false), mFollowIndex(mFollowIndexCounter++)
{
}
AiFollow::AiFollow(const std::string &actorId,const std::string &cellId,float duration, float x, float y, float z)
: mAlwaysFollow(false), mCommanded(false), mRemainingDuration(duration), mX(x), mY(y), mZ(z)
, mActorRefId(actorId), mActorId(-1), mCellId(cellId), mActive(false), mFollowIndex(mFollowIndexCounter++)
{
}

AiFollow::AiFollow(const std::string &actorId, bool commanded)
: mAlwaysFollow(true), mCommanded(commanded), mRemainingDuration(0), mX(0), mY(0), mZ(0)
, mActorRefId(actorId), mActorId(-1), mCellId(""), mActive(false), mFollowIndex(mFollowIndexCounter++)
{

}

AiFollow::AiFollow(const ESM::AiSequence::AiFollow *follow)
    : mAlwaysFollow(follow->mAlwaysFollow), mCommanded(follow->mCommanded), mRemainingDuration(follow->mRemainingDuration)
    , mX(follow->mData.mX), mY(follow->mData.mY), mZ(follow->mData.mZ)
    , mActorRefId(follow->mTargetId), mActorId(-1)
    , mCellId(follow->mCellId), mActive(follow->mActive), mFollowIndex(mFollowIndexCounter++)
{

}

bool AiFollow::execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
{
    MWWorld::Ptr target = getTarget();

    if (target.isEmpty() || !target.getRefData().getCount() || !target.getRefData().isEnabled()  // Really we should be checking whether the target is currently registered
                                                                                                 // with the MechanicsManager
            )
        return false; // Target is not here right now, wait for it to return

    actor.getClass().getCreatureStats(actor).setDrawState(DrawState_Nothing);

    AiFollowStorage& storage = state.get<AiFollowStorage>();

    // AiFollow requires the target to be in range and within sight for the initial activation
    if (!mActive)
    {
        storage.mTimer -= duration;

        if (storage.mTimer < 0)
        {
            if ((actor.getRefData().getPosition().asVec3() - target.getRefData().getPosition().asVec3()).length2()
                    < 500*500
                    && MWBase::Environment::get().getWorld()->getLOS(actor, target))
                mActive = true;
            storage.mTimer = 0.5f;
        }
    }
    if (!mActive)
        return false;

    ESM::Position pos = actor.getRefData().getPosition(); //position of the actor

    float followDistance = 180;
    // When there are multiple actors following the same target, they form a group with each group member at 180*(i+1) distance to the target
    int i=0;
    std::list<int> followers = MWBase::Environment::get().getMechanicsManager()->getActorsFollowingIndices(target);
    followers.sort();
    for (std::list<int>::iterator it = followers.begin(); it != followers.end(); ++it)
    {
        if (*it == mFollowIndex)
            followDistance *= (i+1);
        ++i;
    }

    if(!mAlwaysFollow) //Update if you only follow for a bit
    {
         //Check if we've run out of time
        if (mRemainingDuration != 0)
        {
            mRemainingDuration -= duration;
            if (duration <= 0)
                return true;
        }

        if((pos.pos[0]-mX)*(pos.pos[0]-mX) +
            (pos.pos[1]-mY)*(pos.pos[1]-mY) +
            (pos.pos[2]-mZ)*(pos.pos[2]-mZ) < followDistance*followDistance) //Close-ish to final position
        {
            if(actor.getCell()->isExterior()) //Outside?
            {
                if(mCellId == "") //No cell to travel to
                    return true;
            }
            else
            {
                if(mCellId == actor.getCell()->getCell()->mName) //Cell to travel to
                    return true;
            }
        }
    }

    //Set the target destination from the actor
    ESM::Pathgrid::Point dest = target.getRefData().getPosition().pos;

    float dist = distance(dest, pos.pos[0], pos.pos[1], pos.pos[2]);
    const float threshold = 10;

    if (storage.mMoving)  //Stop when you get close
        storage.mMoving = (dist > followDistance);
    else
        storage.mMoving = (dist > followDistance + threshold);

    if(!storage.mMoving)
    {
        actor.getClass().getMovementSettings(actor).mPosition[1] = 0;

        // turn towards target anyway
        float directionX = target.getRefData().getPosition().pos[0] - actor.getRefData().getPosition().pos[0];
        float directionY = target.getRefData().getPosition().pos[1] - actor.getRefData().getPosition().pos[1];
        zTurn(actor, std::atan2(directionX,directionY), osg::DegreesToRadians(5.f));
    }
    else
    {
        pathTo(actor, dest, duration); //Go to the destination
    }

    //Check if you're far away
    if(dist > 450)
        actor.getClass().getCreatureStats(actor).setMovementFlag(MWMechanics::CreatureStats::Flag_Run, true); //Make NPC run
    else if(dist  < 325) //Have a bit of a dead zone, otherwise npc will constantly flip between running and not when right on the edge of the running threshhold
        actor.getClass().getCreatureStats(actor).setMovementFlag(MWMechanics::CreatureStats::Flag_Run, false); //make NPC walk

    return false;
}

std::string AiFollow::getFollowedActor()
{
    return mActorRefId;
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
    std::auto_ptr<ESM::AiSequence::AiFollow> follow(new ESM::AiSequence::AiFollow());
    follow->mData.mX = mX;
    follow->mData.mY = mY;
    follow->mData.mZ = mZ;
    follow->mTargetId = mActorRefId;
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

MWWorld::Ptr AiFollow::getTarget()
{
    if (mActorId == -2)
        return MWWorld::Ptr();

    if (mActorId == -1)
    {
        MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtr(mActorRefId, false);
        if (target.isEmpty())
        {
            mActorId = -2;
            return target;
        }
        else
            mActorId = target.getClass().getCreatureStats(target).getActorId();
    }

    if (mActorId != -1)
        return MWBase::Environment::get().getWorld()->searchPtrViaActorId(mActorId);
    else
        return MWWorld::Ptr();
}

int AiFollow::getFollowIndex() const
{
    return mFollowIndex;
}

}
