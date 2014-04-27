#ifndef GAME_MWMECHANICS_AICOMBAT_H
#define GAME_MWMECHANICS_AICOMBAT_H

#include "aipackage.hpp"

#include "pathfinding.hpp"

#include "movement.hpp"
#include "obstacle.hpp"

#include "../mwworld/cellstore.hpp" // for Doors

#include "../mwbase/world.hpp"

namespace MWMechanics
{
    class AiCombat : public AiPackage
    {
        public:
            AiCombat(const MWWorld::Ptr& actor);

            virtual AiCombat *clone() const;

            virtual bool execute (const MWWorld::Ptr& actor,float duration);
            ///< \return Package completed?

            virtual int getTypeId() const;

            virtual unsigned int getPriority() const;

            const std::string &getTargetId() const;

        private:
            PathFinder mPathFinder;
            // controls duration of the actual strike
            float mTimerAttack;
            float mTimerReact;
            // controls duration of the sideway & forward moves
            // when mCombatMove is true
            float mTimerCombatMove;

            // AiCombat states
            bool mReadyToAttack, mStrike;
            bool mFollowTarget;
            bool mCombatMove;
            bool mBackOffDoor;

            bool mForceNoShortcut;
            ESM::Position mShortcutFailPos;

            ESM::Position mLastPos;
            MWMechanics::Movement mMovement;
            MWWorld::Ptr mTarget;

            const MWWorld::CellStore* mCell;
            ObstacleCheck mObstacleCheck;
            float mDoorCheckDuration;
            // TODO: for some reason mDoors.searchViaHandle() returns
            // null pointers, workaround by keeping an iterator
            MWWorld::CellRefList<ESM::Door>::List::iterator mDoorIter;
            MWWorld::CellRefList<ESM::Door>& mDoors;

            void buildNewPath(const MWWorld::Ptr& actor);
    };
}

#endif
