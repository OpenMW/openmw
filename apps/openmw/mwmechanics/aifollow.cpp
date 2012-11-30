#include "aifollow.hpp"
#include <iostream>

MWMechanics::AiFollow::AiFollow(const std::string &actorId,float duration, float x, float y, float z)
: mDuration(duration), mX(x), mY(y), mZ(z), mActorId(actorId)
{
}
MWMechanics::AiFollow::AiFollow(const std::string &actorId,const std::string &cellId,float duration, float x, float y, float z)
: mDuration(duration), mX(x), mY(y), mZ(z), mActorId(actorId), mCellId(cellId)
{
}

MWMechanics::AiFollow *MWMechanics::AiFollow::clone() const
{
    return new AiFollow(*this);
}

 bool MWMechanics::AiFollow::execute (const MWWorld::Ptr& actor)
{
    std::cout << "AiFollow completed.\n";
    return true;
}

 int MWMechanics::AiFollow::getTypeId() const
{
    return 3;
}
