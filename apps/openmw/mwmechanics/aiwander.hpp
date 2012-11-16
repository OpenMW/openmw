#ifndef GAME_MWMECHANICS_AIWANDER_H
#define GAME_MWMECHANICS_AIWANDER_H

#include "aipackage.hpp"
#include <vector>

namespace MWMechanics
{

    class AiWander : public AiPackage
    {
    public:

        AiWander(int distance, int duration, int timeOfDay, const std::vector<int>& idle);
        virtual AiPackage *clone() const;
        virtual bool execute (const MWWorld::Ptr& actor);
        ///< \return Package completed?
        virtual int getTypeId() const;
        ///< 0: Wander

    private:
        int mDistance;
        int mDuration;
        int mTimeOfDay;
        std::vector<int> mIdle;
    };
    }

#endif
