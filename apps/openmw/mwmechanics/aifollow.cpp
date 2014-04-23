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
: mAlwaysFollow(false), mDuration(duration), mX(x), mY(y), mZ(z), mActorId(actorId), mCellId(""), mTimer(0), mStuckTimer(0)
{
}
MWMechanics::AiFollow::AiFollow(const std::string &actorId,const std::string &cellId,float duration, float x, float y, float z)
: mAlwaysFollow(false), mDuration(duration), mX(x), mY(y), mZ(z), mActorId(actorId), mCellId(cellId), mTimer(0), mStuckTimer(0)
{
}

MWMechanics::AiFollow::AiFollow(const std::string &actorId)
: mAlwaysFollow(true), mDuration(0), mX(0), mY(0), mZ(0), mActorId(actorId), mCellId(""), mTimer(0), mStuckTimer(0)
{
}

bool MWMechanics::AiFollow::execute (const MWWorld::Ptr& actor,float duration)
{
    const MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtr(mActorId, false); //The target to follow

    if(target == MWWorld::Ptr()) return true;   //Target doesn't exist

    mTimer = mTimer + duration; //Update timer
    mStuckTimer = mStuckTimer + duration;   //Update stuck timer
    mTotalTime = mTotalTime + duration; //Update total time following

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
    ESM::Pathgrid::Point dest;
    dest.mX = target.getRefData().getPosition().pos[0];
    dest.mY = target.getRefData().getPosition().pos[1];
    dest.mZ = target.getRefData().getPosition().pos[2];

    //Current position, for pathfilding stuff
    ESM::Pathgrid::Point start;
    start.mX = pos.pos[0];
    start.mY = pos.pos[1];
    start.mZ = pos.pos[2];

    //Build the path to get to the destination
    if(mPathFinder.getPath().empty())
        mPathFinder.buildPath(start, dest, actor.getCell(), true);

    //***********************
    // Checks if you can't get to the end position at all
    //***********************
    if(mTimer > 0.25)
    {
        if(!mPathFinder.getPath().empty()) //Path has points in it
        {
            ESM::Pathgrid::Point lastPos = mPathFinder.getPath().back(); //Get the end of the proposed path

            if((dest.mX - lastPos.mX)*(dest.mX - lastPos.mX)
                +(dest.mY - lastPos.mY)*(dest.mY - lastPos.mY)
                +(dest.mZ - lastPos.mZ)*(dest.mZ - lastPos.mZ)
            > 100*100) //End of the path is far from the destination
                mPathFinder.addPointToPath(dest); //Adds the final destination to the path, to try to get to where you want to go
        }

        mTimer = 0;
    }

    //************************
    // Checks if you aren't moving; you're stuck
    //************************
    if(mStuckTimer>0.5) //Checks every half of a second
    {
        if((mStuckPos.pos[0] - pos.pos[0])*(mStuckPos.pos[0] - pos.pos[0])
            +(mStuckPos.pos[1] - pos.pos[1])*(mStuckPos.pos[1] - pos.pos[1])
            +(mStuckPos.pos[2] - pos.pos[2])*(mStuckPos.pos[2] - pos.pos[2]) < 100) //NPC is stuck
            mPathFinder.buildPath(start, dest, actor.getCell(), true);

        mStuckTimer = 0;
        mStuckPos = pos;
    }

    //Checks if the path isn't over, turn tomards the direction that you're going
    if(!mPathFinder.checkPathCompleted(pos.pos[0],pos.pos[1],pos.pos[2]))
    {
        zTurn(actor, Ogre::Degree(mPathFinder.getZAngleToNext(pos.pos[0], pos.pos[1])));
    }

    if((dest.mX - pos.pos[0])*(dest.mX - pos.pos[0])+(dest.mY - pos.pos[1])*(dest.mY - pos.pos[1])+(dest.mZ - pos.pos[2])*(dest.mZ - pos.pos[2])
        < 100*100)
        actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
    else
        actor.getClass().getMovementSettings(actor).mPosition[1] = 1;

    //Check if you're far away
    if((dest.mX - start.mX)*(dest.mX - start.mX)
                +(dest.mY - start.mY)*(dest.mY - start.mY)
                +(dest.mZ - start.mZ)*(dest.mZ - start.mZ) > 1000*1000)
        actor.getClass().getCreatureStats(actor).setMovementFlag(MWMechanics::CreatureStats::Flag_Run, true); //Make NPC run
    else if((dest.mX - start.mX)*(dest.mX - start.mX) //Have a bit of a dead zone, otherwise npc will constantly flip between running and not when right on the edge of the running threshhold
                +(dest.mY - start.mY)*(dest.mY - start.mY)
                +(dest.mZ - start.mZ)*(dest.mZ - start.mZ) < 800*800)
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
