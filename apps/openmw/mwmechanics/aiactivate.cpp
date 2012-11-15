#include "aiactivate.hpp"
#include <iostream>

MWMechanics::AiActivate::AiActivate(const MWWorld::Ptr& object)
{
    mObject = &object;
}
MWMechanics::AiActivate *MWMechanics::AiActivate::clone() const
{
    return new AiActivate(*this);
}
bool MWMechanics::AiActivate::execute (const MWWorld::Ptr& actor)
{
    std::cout << "AiActivate completed.\n";
    return true;
}

int MWMechanics::AiActivate::getTypeId() const
{
    return 4;
}
