#include "aifollow.hpp"
#include <iostream>
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwworld/class.hpp"
#include "movement.hpp"

MWMechanics::AiFollow::AiFollow(const std::string &actorId,float duration, float x, float y, float z)
: mDuration(duration), mX(x), mY(y), mZ(z), mActorId(actorId), mCellId(""), mTimer(0), mStuckTimer(0)
{
}
MWMechanics::AiFollow::AiFollow(const std::string &actorId,const std::string &cellId,float duration, float x, float y, float z)
: mDuration(duration), mX(x), mY(y), mZ(z), mActorId(actorId), mCellId(cellId), mTimer(0), mStuckTimer(0)
{
}

MWMechanics::AiFollow *MWMechanics::AiFollow::clone() const
{
    return new AiFollow(*this);
}

bool MWMechanics::AiFollow::execute (const MWWorld::Ptr& actor,float duration)
{
    const MWWorld::Ptr target = MWBase::Environment::get().getWorld()->getPtr(mActorId, false);

    mTimer = mTimer + duration;
    mStuckTimer = mStuckTimer + duration;
    mTotalTime = mTotalTime + duration;

    ESM::Position pos = actor.getRefData().getPosition();

    if(mTotalTime > mDuration && mDuration != 0)
        return true;

    if((pos.pos[0]-mX)*(pos.pos[0]-mX) +
        (pos.pos[1]-mY)*(pos.pos[1]-mY) +
        (pos.pos[2]-mZ)*(pos.pos[2]-mZ) < 100*100) 
    {
        if(actor.getCell()->isExterior())
        {
            if(mCellId == "") 
                return true;
        }
        else
        {
            if(mCellId == actor.getCell()->mCell->mName)
                return true;
        }
    }

    ESM::Pathgrid::Point dest;
    dest.mX = target.getRefData().getPosition().pos[0];
    dest.mY = target.getRefData().getPosition().pos[1];
    dest.mZ = target.getRefData().getPosition().pos[2];

    if(mTimer > 0.25)
    {
        if(!mPathFinder.getPath().empty())
        {
            ESM::Pathgrid::Point lastPos = mPathFinder.getPath().back();

            if((dest.mX - lastPos.mX)*(dest.mX - lastPos.mX) 
                +(dest.mY - lastPos.mY)*(dest.mY - lastPos.mY)
                +(dest.mZ - lastPos.mZ)*(dest.mZ - lastPos.mZ)
            > 100*100)
            mPathFinder.addPointToPath(dest);
        }
        else
            mPathFinder.addPointToPath(dest);

        mTimer = 0;
    }

    if(mStuckTimer>0.5)
    {
        if((mStuckPos.pos[0] - pos.pos[0])*(mStuckPos.pos[0] - pos.pos[0])
            +(mStuckPos.pos[1] - pos.pos[1])*(mStuckPos.pos[1] - pos.pos[1])
            +(mStuckPos.pos[2] - pos.pos[2])*(mStuckPos.pos[2] - pos.pos[2])
            < 100) //NPC is stuck
        {
            ESM::Pathgrid::Point start;
            start.mX = pos.pos[0];
            start.mY = pos.pos[1];
            start.mZ = pos.pos[2];

            mPathFinder.buildPath(start, dest, actor.getCell(), true);
        }
        mStuckTimer = 0;
        mStuckPos = pos;
    }

    if(mPathFinder.getPath().empty())
        mPathFinder.addPointToPath(dest);

    if(!mPathFinder.checkPathCompleted(pos.pos[0],pos.pos[1],pos.pos[2]))
    {
        float zAngle = mPathFinder.getZAngleToNext(pos.pos[0], pos.pos[1]);
        MWBase::Environment::get().getWorld()->rotateObject(actor, 0, 0, zAngle, false);
    }

    if((dest.mX - pos.pos[0])*(dest.mX - pos.pos[0])+(dest.mY - pos.pos[1])*(dest.mY - pos.pos[1])+(dest.mZ - pos.pos[2])*(dest.mZ - pos.pos[2])
        < 100*100)
        MWWorld::Class::get(actor).getMovementSettings(actor).mPosition[1] = 0;
    else
        MWWorld::Class::get(actor).getMovementSettings(actor).mPosition[1] = 1;

    return false;
}

 int MWMechanics::AiFollow::getTypeId() const
{
    return 3;
}

std::string MWMechanics::AiFollow::getFollowedActor()
{
    return mActorId;
}
