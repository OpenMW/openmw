#include "aifollow.hpp"

#include <iostream>

#include <components/esm/aisequence.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"
#include "creaturestats.hpp"
#include "movement.hpp"

#include <OgreMath.h>

#include "steering.hpp"

MWMechanics::AiFollow::AiFollow(const std::string &actorId,float duration, float x, float y, float z)
: mAlwaysFollow(false), mCommanded(false), mRemainingDuration(duration), mX(x), mY(y), mZ(z)
, mActorRefId(actorId), mCellId(""), mActorId(-1)
{
}
MWMechanics::AiFollow::AiFollow(const std::string &actorId,const std::string &cellId,float duration, float x, float y, float z)
: mAlwaysFollow(false), mCommanded(false), mRemainingDuration(duration), mX(x), mY(y), mZ(z)
, mActorRefId(actorId), mCellId(cellId), mActorId(-1)
{
}

MWMechanics::AiFollow::AiFollow(const std::string &actorId, bool commanded)
: mAlwaysFollow(true), mCommanded(commanded), mRemainingDuration(0), mX(0), mY(0), mZ(0)
, mActorRefId(actorId), mCellId(""), mActorId(-1)
{
}

MWMechanics::AiFollow::AiFollow(const ESM::AiSequence::AiFollow *follow)
    : mAlwaysFollow(follow->mAlwaysFollow), mRemainingDuration(follow->mRemainingDuration)
    , mX(follow->mData.mX), mY(follow->mData.mY), mZ(follow->mData.mZ)
    , mActorRefId(follow->mTargetId), mActorId(-1), mCellId(follow->mCellId)
    , mCommanded(follow->mCommanded)
{

}

bool MWMechanics::AiFollow::execute (const MWWorld::Ptr& actor, AiState& state, float duration)
{
    MWWorld::Ptr target = getTarget();

    if (target.isEmpty() || !target.getRefData().getCount() || !target.getRefData().isEnabled()  // Really we should be checking whether the target is currently registered
                                                                                                 // with the MechanicsManager
            )
        return true; //Target doesn't exist

    actor.getClass().getCreatureStats(actor).setDrawState(DrawState_Nothing);

    ESM::Position pos = actor.getRefData().getPosition(); //position of the actor

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
            (pos.pos[2]-mZ)*(pos.pos[2]-mZ) < 100*100) //Close-ish to final position
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

    if(distance(dest, pos.pos[0], pos.pos[1], pos.pos[2]) < 400) //Stop when you get close
        actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
    else {
        pathTo(actor, dest, duration); //Go to the destination
    }

    //Check if you're far away
    if(distance(dest, pos.pos[0], pos.pos[1], pos.pos[2]) > 750)
        actor.getClass().getCreatureStats(actor).setMovementFlag(MWMechanics::CreatureStats::Flag_Run, true); //Make NPC run
    else if(distance(dest, pos.pos[0], pos.pos[1], pos.pos[2])  < 625) //Have a bit of a dead zone, otherwise npc will constantly flip between running and not when right on the edge of the running threshhold
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
