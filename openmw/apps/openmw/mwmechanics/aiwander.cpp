#include "aiwander.hpp"

MWMechanics::AiWander::AiWander(int distance, int duration, int timeOfDay,std::vector<int> Idle,bool reset)
{
    mDistance = distance;
    mDuration = duration;
    mTimeOfDay = timeOfDay;
    mIdle = Idle;
    mReset = reset;
}

int MWMechanics::AiWander::getDistance() const
{
    return mDistance;
}

int MWMechanics::AiWander::getDuration() const
{
    return mDuration;
}

int MWMechanics::AiWander::getTimeOfDay() const
{
    return mTimeOfDay;
}

bool MWMechanics::AiWander::getReset() const
{
    return mReset;
}

MWMechanics::AiPackage * MWMechanics::AiWander::clone() const
{
    return new AiWander(*this);
}

bool MWMechanics::AiWander::execute (const MWWorld::Ptr& actor)
{
    std::cout << "AiWadner complited. \n";
    return true;
}

int MWMechanics::AiWander::getTypeId() const
{
    return 0;
}

int MWMechanics::AiWander::getIdle(int index) const
{
    if(index < 0 || (uint)index > mIdle.size())
        return -1;
    int temp;
    temp = mIdle.at(index);
    return temp;
}
