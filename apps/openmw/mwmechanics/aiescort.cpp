#include "aiescort.hpp"


MWMechanics::AiEscort::AiEscort(std::string ActorID,int Duration, float X, float Y, float Z, bool Reset)
{
    mActorID = ActorID;
    mDuration = Duration;
    mX = X;
    mY = Y;
    mZ = Z;
    mReset = Reset;

}
MWMechanics::AiEscort *MWMechanics::AiEscort::clone() const
{
    AiEscort * temp = new AiEscort(*this);
    return temp;
}

bool MWMechanics::AiEscort::execute (const MWWorld::Ptr& actor)
{
    std::cout << "AiEscort complted. \n";
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
bool MWMechanics::AiEscort::getReset()
{
    return mReset;
}

std::string MWMechanics::AiEscort::getActorID()
{
    return mActorID;
}

int MWMechanics::AiEscort::getDuration()
{
    return mDuration;
}
