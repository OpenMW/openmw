#include "aifallow.hpp"

MWMechanics::AiFallow::AiFallow(std::string ActorID,float duration, float X, float Y, float Z, bool Reset)
{
    mActorID = ActorID;
    mDuration = duration;
    mX = X;
    mY = Y;
    mZ = Z;
    mReset = Reset;
}
MWMechanics::AiFallow *MWMechanics::AiFallow::clone() const
{
    AiFallow * temp = new AiFallow(*this);
    return temp;
}

 bool MWMechanics::AiFallow::execute (const MWWorld::Ptr& actor)
{
    std::cout << "AiFallow complited. \n";
    return true;
}

 int MWMechanics::AiFallow::getTypeId() const
{
    return 3;
}
