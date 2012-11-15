#ifndef GAME_MWMECHANICS_AIWANDER_H
#define GAME_MWMECHANICS_AIWANDER_H

#include "aipackage.hpp"
#include <vector>

namespace MWMechanics
{

class AiWander : public AiPackage
{
public:

    AiWander(int distance, int duration, int timeOfDay,std::vector<int> Idle);
    virtual AiPackage *clone() const;
    virtual bool execute (const MWWorld::Ptr& actor);
    ///< \return Package completed?
    virtual int getTypeId() const;
    ///< 0: Wander

    int getDistance() const;
    int getDuration()const;
    int getTimeOfDay()const;
    int getIdle(int index) const;

private:
    int mDistance;
    int mDuration;
    int	mTimeOfDay;
    std::vector<int> mIdle;
};
}

#endif // AIWANDER_H
