#include "aiescort.hpp"
#include <iostream>

MWMechanics::AiEscort::AiEscort(const std::string &actorID,int duration, float x, float y, float z):
mActorID(actorID), mDuration(duration), mX(x), mY(y), mZ(z)
{
}
MWMechanics::AiEscort *MWMechanics::AiEscort::clone() const
{
    return new AiEscort(*this);
}

bool MWMechanics::AiEscort::execute (const MWWorld::Ptr& actor)
{
    std::cout << "AiEscort completed. \n";
    return true;
}

int MWMechanics::AiEscort::getTypeId() const
{
    return 2;
}

float MWMechanics::AiEscort::getX()
{
    return mX;
}
float MWMechanics::AiEscort::getY()
{
    return mY;
}
float MWMechanics::AiEscort::getZ()
{
    return mZ;
}

std::string MWMechanics::AiEscort::getActorID()
{
    return mActorID;
}

int MWMechanics::AiEscort::getDuration()
{
    return mDuration;
}
