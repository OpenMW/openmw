#include "aitravel.hpp"
#include <iostream>

MWMechanics::AiTravel::AiTravel(float x, float y, float z)
: mX(x),mY(y),mZ(z)
{
}

MWMechanics::AiTravel * MWMechanics::AiTravel::clone() const
{
    return new AiTravel(*this);
}

bool MWMechanics::AiTravel::execute (const MWWorld::Ptr& actor)
{
    std::cout << "AiTravel completed.\n";
    return true;
}

int MWMechanics::AiTravel::getTypeId() const
{
    return 1;
}


