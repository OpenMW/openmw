#ifndef GAME_MWMECHANICS_AIWANDER_H
#define GAME_MWMECHANICS_AIWANDER_H

#include "aipackage.hpp"
#include <vector>

#include "pathfinding.hpp"
#include "obstacle.hpp"

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

            int mDistance; // how far the actor can wander from the spawn point
            int mDuration;
            int mTimeOfDay;
            std::vector<int> mIdle;
            bool mRepeat;

            bool mSaidGreeting;
            int mGreetDistanceMultiplier;
            float mGreetDistanceReset;
            float mChance;

            // Cached current cell location
            int mCellX;
            int mCellY;
            // Cell location multiplied by ESM::Land::REAL_SIZE
            float mXCell;
            float mYCell;

            const MWWorld::CellStore* mCell; // for detecting cell change

            // if false triggers calculating allowed nodes based on mDistance
            bool mStoredAvailableNodes;
            // AiWander states
            bool mChooseAction;
            bool mIdleNow;
            bool mMoveNow;
            bool mWalking;

            float mIdleChanceMultiplier;
            unsigned short mPlayedIdle;

            MWWorld::TimeStamp mStartTime;

            // allowed pathgrid nodes based on mDistance from the spawn point
            std::vector<ESM::Pathgrid::Point> mAllowedNodes;
            ESM::Pathgrid::Point mCurrentNode;
            bool mTrimCurrentNode;
            void trimAllowedNodes(std::vector<ESM::Pathgrid::Point>& nodes,
                                  const PathFinder& pathfinder);

            PathFinder mPathFinder;

            ObstacleCheck mObstacleCheck;
            float mDoorCheckDuration;
            int mStuckCount;

            // the z rotation angle (degrees) we want to reach
            // used every frame when mRotate is true
            float mTargetAngle;
            bool mRotate;
            float mReaction; // update some actions infrequently
    };
}

#endif
