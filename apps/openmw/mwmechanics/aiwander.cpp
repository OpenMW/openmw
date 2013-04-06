#include "aiwander.hpp"
#include <iostream>

MWMechanics::AiWander::AiWander(int distance, int duration, int timeOfDay, const std::vector<int>& idle):
    mDistance(distance), mDuration(duration), mTimeOfDay(timeOfDay), mIdle(idle)
{
}

MWMechanics::AiPackage * MWMechanics::AiWander::clone() const
{
    return new AiWander(*this);
}

bool MWMechanics::AiWander::execute (const MWWorld::Ptr& actor)
{
    // Return completed
    return true;
}

int MWMechanics::AiWander::getTypeId() const
{
    return 0;
}
