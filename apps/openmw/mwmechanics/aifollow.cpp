#include "aifollow.hpp"

#include <iostream>

#include <components/esm/aisequence.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"
#include "creaturestats.hpp"
#include "movement.hpp"

#include <OgreMath.h>

#include "steering.hpp"

int MWMechanics::AiFollow::mFollowIndexCounter = 0;

MWMechanics::AiFollow::AiFollow(const std::string &actorId,float duration, float x, float y, float z)
: mAlwaysFollow(false), mCommanded(false), mRemainingDuration(duration), mX(x), mY(y), mZ(z)
, mActorRefId(actorId), mCellId(""), mActorId(-1), mFollowIndex(mFollowIndexCounter++)
{
}
MWMechanics::AiFollow::AiFollow(const std::string &actorId,const std::string &cellId,float duration, float x, float y, float z)
: mAlwaysFollow(false), mCommanded(false), mRemainingDuration(duration), mX(x), mY(y), mZ(z)
, mActorRefId(actorId), mCellId(cellId), mActorId(-1), mFollowIndex(mFollowIndexCounter++)
{
}

MWMechanics::AiFollow::AiFollow(const std::string &actorId, bool commanded)
: mAlwaysFollow(true), mCommanded(commanded), mRemainingDuration(0), mX(0), mY(0), mZ(0)
, mActorRefId(actorId), mCellId(""), mActorId(-1), mFollowIndex(mFollowIndexCounter++)
{

}

MWMechanics::AiFollow::AiFollow(const ESM::AiSequence::AiFollow *follow)
    : mAlwaysFollow(follow->mAlwaysFollow), mRemainingDuration(follow->mRemainingDuration)
    , mX(follow->mData.mX), mY(follow->mData.mY), mZ(follow->mData.mZ)
    , mActorRefId(follow->mTargetId), mActorId(-1), mCellId(follow->mCellId)
    , mCommanded(follow->mCommanded), mFollowIndex(mFollowIndexCounter++)
{

}

bool MWMechanics::AiFollow::execute (const MWWorld::Ptr& actor, AiState& state, float duration)
{
    MWWorld::Ptr target = getTarget();

    if (target.isEmpty() || !target.getRefData().getCount() || !target.getRefData().isEnabled()  // Really we should be checking whether the target is currently registered
                                                                                                 // with the MechanicsManager
            )
        return false; // Target is not here right now, wait for it to return

    actor.getClass().getCreatureStats(actor).setDrawState(DrawState_Nothing);

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

    if(distance(dest, pos.pos[0], pos.pos[1], pos.pos[2]) < followDistance) //Stop when you get close
        actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
    else {
        pathTo(actor, dest, duration); //Go to the destination
    }

    //Check if you're far away
    if(distance(dest, pos.pos[0], pos.pos[1], pos.pos[2]) > 450)
        actor.getClass().getCreatureStats(actor).setMovementFlag(MWMechanics::CreatureStats::Flag_Run, true); //Make NPC run
    else if(distance(dest, pos.pos[0], pos.pos[1], pos.pos[2])  < 325) //Have a bit of a dead zone, otherwise npc will constantly flip between running and not when right on the edge of the running threshhold
        actor.getClass().getCreatureStats(actor).setMovementFlag(MWMechanics::CreatureStats::Flag_Run, false); //make NPC walk

    return false;
}

std::string MWMechanics::AiFollow::getFollowedActor()
{
    return mActorRefId;
}

MWMechanics::AiFollow *MWMechanics::AiFollow::clone() const
{
    return new AiFollow(*this);
}

int MWMechanics::AiFollow::getTypeId() const
{
    return TypeIdFollow;
}

bool MWMechanics::AiFollow::isCommanded() const
{
    return mCommanded;
}

void MWMechanics::AiFollow::writeState(ESM::AiSequence::AiSequence &sequence) const
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

    ESM::AiSequence::AiPackageContainer package;
    package.mType = ESM::AiSequence::Ai_Follow;
    package.mPackage = follow.release();
    sequence.mPackages.push_back(package);
}

MWWorld::Ptr MWMechanics::AiFollow::getTarget()
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

int MWMechanics::AiFollow::getFollowIndex() const
{
    return mFollowIndex;
}
