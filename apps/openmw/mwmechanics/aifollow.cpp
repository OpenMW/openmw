#include "aifollow.hpp"
#include <iostream>
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"
#include "creaturestats.hpp"
#include "movement.hpp"

#include <OgreMath.h>

#include "steering.hpp"

MWMechanics::AiFollow::AiFollow(const std::string &actorId,float duration, float x, float y, float z)
: mAlwaysFollow(false), mDuration(duration), mX(x), mY(y), mZ(z), mActorId(actorId), mCellId("")
{
    mTimer = 0;
    mStuckTimer = 0;
}
MWMechanics::AiFollow::AiFollow(const std::string &actorId,const std::string &cellId,float duration, float x, float y, float z)
: mAlwaysFollow(false), mDuration(duration), mX(x), mY(y), mZ(z), mActorId(actorId), mCellId(cellId)
{
    mTimer = 0;
    mStuckTimer = 0;
}

MWMechanics::AiFollow::AiFollow(const std::string &actorId)
: mAlwaysFollow(true), mDuration(0), mX(0), mY(0), mZ(0), mActorId(actorId), mCellId("")
{
    mTimer = 0;
    mStuckTimer = 0;
}

bool MWMechanics::AiFollow::execute (const MWWorld::Ptr& actor,float duration)
{
    const MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtr(mActorId, false); //The target to follow

    if(target == MWWorld::Ptr()) return true;   //Target doesn't exist

    ESM::Position pos = actor.getRefData().getPosition(); //position of the actor

    if(!mAlwaysFollow) //Update if you only follow for a bit
    {
        if(mTotalTime > mDuration && mDuration != 0) //Check if we've run out of time
            return true;

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

    //Set the target desition from the actor
    ESM::Pathgrid::Point dest = target.getRefData().getPosition().pos;

    pathTo(actor, dest, duration); //Go to the destination

    if(distance(dest, pos.pos[0], pos.pos[1], pos.pos[2]) < 100) //Stop when you get close
        actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
    else
        actor.getClass().getMovementSettings(actor).mPosition[1] = 1;

    //Check if you're far away
    if(distance(dest, pos.pos[0], pos.pos[1], pos.pos[2]) > 1000)
        actor.getClass().getCreatureStats(actor).setMovementFlag(MWMechanics::CreatureStats::Flag_Run, true); //Make NPC run
    else if(distance(dest, pos.pos[0], pos.pos[1], pos.pos[2])  < 800) //Have a bit of a dead zone, otherwise npc will constantly flip between running and not when right on the edge of the running threshhold
        actor.getClass().getCreatureStats(actor).setMovementFlag(MWMechanics::CreatureStats::Flag_Run, false); //make NPC walk

    return false;
}

std::string MWMechanics::AiFollow::getFollowedActor()
{
    return mActorId;
}

MWMechanics::AiFollow *MWMechanics::AiFollow::clone() const
{
    return new AiFollow(*this);
}

 int MWMechanics::AiFollow::getTypeId() const
{
    return TypeIdFollow;
}
