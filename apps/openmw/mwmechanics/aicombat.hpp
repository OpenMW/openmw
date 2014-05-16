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
    /// \brief Causes the actor to fight another actor
    class AiCombat : public AiPackage
    {
        public:
            ///Constructor
            /** \param actor Actor to fight **/
            AiCombat(const MWWorld::Ptr& actor);

            virtual AiCombat *clone() const;

            virtual bool execute (const MWWorld::Ptr& actor,float duration);

            virtual int getTypeId() const;

            virtual unsigned int getPriority() const;

            ///Returns target ID
            std::string getTargetId() const;

        private:
            PathFinder mPathFinder;
            // controls duration of the actual strike
            float mTimerAttack;
            float mTimerReact;
            // controls duration of the sideway & forward moves
            // when mCombatMove is true
            float mTimerCombatMove;

            // AiCombat states
            bool mReadyToAttack, mAttack;
            bool mFollowTarget;
            bool mCombatMove;
            bool mBackOffDoor;

            bool mForceNoShortcut;
            ESM::Position mShortcutFailPos;

            ESM::Position mLastPos;
            MWMechanics::Movement mMovement;
            int mTargetActorId;

            const MWWorld::CellStore* mCell;
            ObstacleCheck mObstacleCheck;
            float mDoorCheckDuration;
            // TODO: for some reason mDoors.searchViaHandle() returns
            // null pointers, workaround by keeping an iterator
            MWWorld::CellRefList<ESM::Door>::List::iterator mDoorIter;
            MWWorld::CellRefList<ESM::Door>& mDoors;

            void buildNewPath(const MWWorld::Ptr& actor, const MWWorld::Ptr& target);
    };
}

#endif
