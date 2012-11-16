#include "aifollow.hpp"
#include <iostream>

MWMechanics::AiFollow::AiFollow(const std::string &actorID,float duration, float x, float y, float z):
mActorID(actorID), mDuration(duration), mX(x), mY(y), mZ(z)
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
