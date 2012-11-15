#include "aifollow.hpp"
#include <iostream>

MWMechanics::AiFollow::AiFollow(const std::string &ActorID,float duration, float X, float Y, float Z):
mActorID(ActorID), mDuration(duration), mX(X), mY(Y), mZ(Z)
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
