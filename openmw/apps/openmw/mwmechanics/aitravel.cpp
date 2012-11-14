#include "aitravel.hpp"

MWMechanics::AiTravel::AiTravel(float x, float y, float z, bool reset)
{
    mX = x;
    mY = y;
    mZ = z;
    mReset = reset;
}
MWMechanics::AiTravel * MWMechanics::AiTravel::clone() const
{
    AiTravel * temp = new AiTravel(*this);
    return temp;
}
bool MWMechanics::AiTravel::execute (const MWWorld::Ptr& actor)
{
    std::cout << "AiTravel complited. \n";
    return true;
}
int MWMechanics::AiTravel::getTypeId() const
{
    return 1;
}

float MWMechanics::AiTravel::getX()
{
    return mX;
}


float MWMechanics::AiTravel::getY()
{
    return mY;
}

float MWMechanics::AiTravel::getZ()
{
    return mZ;
}

bool MWMechanics::AiTravel::getReset()
{
    return mReset;
}
