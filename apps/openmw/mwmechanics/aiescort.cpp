#include "aiescort.hpp"
#include <iostream>

MWMechanics::AiEscort::AiEscort(const std::string &actorId,int duration, float x, float y, float z)
: mActorId(actorId), mX(x), mY(y), mZ(z), mDuration(duration)
{
}

MWMechanics::AiEscort::AiEscort(const std::string &actorId,const std::string &cellId,int duration, float x, float y, float z)
: mActorId(actorId), mCellId(cellId), mX(x), mY(y), mZ(z), mDuration(duration)
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

