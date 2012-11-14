#ifndef AIWANDER_H
#define AIWANDER_H

#include "aipackage.hpp"
#include <iostream>
#include <vector>

namespace MWWorld
{
class Ptr;
}

namespace MWMechanics
{

class AiWander : public AiPackage
{
public:

    AiWander(int distance, int duration, int timeOfDay,std::vector<int> Idle,bool reset=false);
    virtual AiPackage *clone() const;
    virtual bool execute (const MWWorld::Ptr& actor);
    ///< \return Package completed?
    virtual int getTypeId() const;
    ///< 0: Wander

    int getDistance() const;
    int getDuration()const;
    int getTimeOfDay()const;
    bool getReset()const;
    int getIdle(int index) const;

private:
    int mDistance;
    int mDuration;
    int	mTimeOfDay;
    std::vector<int> mIdle;
    bool mReset;
};
}

#endif // AIWANDER_H
