#ifndef GAME_MWMECHANICS_AIWANDER_H
#define GAME_MWMECHANICS_AIWANDER_H

#include "aipackage.hpp"
#include <vector>

#include "pathfinding.hpp"

#include "../mwworld/timestamp.hpp"

namespace MWMechanics
{
    class AiWander : public AiPackage
    {
        public:

            AiWander(int distance, int duration, int timeOfDay, const std::vector<int>& idle, bool repeat);
            virtual AiPackage *clone() const;
            virtual bool execute (const MWWorld::Ptr& actor,float duration);
            ///< \return Package completed?
            virtual int getTypeId() const;
            ///< 0: Wander

        private:
            void stopWalking(const MWWorld::Ptr& actor);
            void playIdle(const MWWorld::Ptr& actor, unsigned short idleSelect);
            bool checkIdle(const MWWorld::Ptr& actor, unsigned short idleSelect);

            int mDistance;
            int mDuration;
            int mTimeOfDay;
            std::vector<int> mIdle;
            bool mRepeat;

            bool mSaidGreeting;

            float mX;
            float mY;
            float mZ;

            int mCellX;
            int mCellY;
            float mXCell;
            float mYCell;

            // for checking if we're stuck (but don't check Z axis)
            float mPrevX;
            float mPrevY;

            enum WalkState
            {
                State_Norm,
                State_CheckStuck,
                State_Evade
            };
            WalkState mWalkState;

            int mStuckCount;
            int mEvadeCount;

            bool mStoredAvailableNodes;
            bool mChooseAction;
            bool mIdleNow;
            bool mMoveNow;
            bool mWalking;

            float mIdleChanceMultiplier;
            unsigned short mPlayedIdle;

            MWWorld::TimeStamp mStartTime;

            std::vector<ESM::Pathgrid::Point> mAllowedNodes;
            ESM::Pathgrid::Point mCurrentNode;

            PathFinder mPathFinder;
            const ESM::Pathgrid *mPathgrid;

    };
}

#endif
