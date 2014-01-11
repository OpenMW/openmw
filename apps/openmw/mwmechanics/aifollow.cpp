#include "aifollow.hpp"
#include <iostream>
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwworld/class.hpp"
#include "movement.hpp"

MWMechanics::AiFollow::AiFollow(const std::string &actorId,float duration, float x, float y, float z)
: mDuration(duration), mX(x), mY(y), mZ(z), mActorId(actorId)
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

    if(mTotalTime > mDuration) return true;

    ESM::Pathgrid::Point dest;
    dest.mX = target.getRefData().getPosition().pos[0];
    dest.mY = target.getRefData().getPosition().pos[1];
    dest.mZ = target.getRefData().getPosition().pos[2];

    if(mTimer > 0.25)
    {
        ESM::Pathgrid::Point lastPos = mPathFinder.getPath().back();

        if((dest.mX - lastPos.mX)*(dest.mX - lastPos.mX) 
            +(dest.mY - lastPos.mY)*(dest.mY - lastPos.mY)
            +(dest.mZ - lastPos.mZ)*(dest.mZ - lastPos.mZ)
            > 100*100)
            mPathFinder.getPath().push_back(dest);

        mTimer = 0;
    }

    ESM::Position pos = actor.getRefData().getPosition();
    float zAngle = mPathFinder.getZAngleToNext(pos.pos[0], pos.pos[1]);
    MWBase::Environment::get().getWorld()->rotateObject(actor, 0, 0, zAngle, false);

    if((dest.mX - pos.pos[0])*(dest.mX - pos.pos[0])+(dest.mY - pos.pos[1])*(dest.mY - pos.pos[1])+(dest.mZ - pos.pos[2])*(dest.mZ - pos.pos[2])
        < 100*100)
        MWWorld::Class::get(actor).getMovementSettings(actor).mPosition[1] = 0;
    else
        MWWorld::Class::get(actor).getMovementSettings(actor).mPosition[1] = 1;

    std::cout << "AiFollow completed.\n";
    return false;
}

 int MWMechanics::AiFollow::getTypeId() const
{
    return 3;
}
