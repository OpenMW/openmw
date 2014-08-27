#ifndef GAME_MWMECHANICS_AICOMBAT_H
#define GAME_MWMECHANICS_AICOMBAT_H

#include "aipackage.hpp"

#include "pathfinding.hpp"

#include "movement.hpp"
#include "obstacle.hpp"

#include <OgreVector3.h>

#include "../mwworld/cellstore.hpp" // for Doors

#include "../mwbase/world.hpp"

#include <boost/shared_ptr.hpp>

namespace ESM
{
    namespace AiSequence
    {
        struct AiCombat;
    }
}

namespace MWMechanics
{
    class Action;

    /// \brief Causes the actor to fight another actor
    class AiCombat : public AiPackage
    {
        public:
            ///Constructor
            /** \param actor Actor to fight **/
            AiCombat(const MWWorld::Ptr& actor);

            AiCombat (const ESM::AiSequence::AiCombat* combat);

            void init();

            virtual AiCombat *clone() const;

            virtual bool execute (const MWWorld::Ptr& actor,float duration);

            virtual int getTypeId() const;

            virtual unsigned int getPriority() const;

            ///Returns target ID
            MWWorld::Ptr getTarget() const;

            virtual void writeState(ESM::AiSequence::AiSequence &sequence) const;

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

            float mStrength; // this is actually make sense only in ranged combat
            float mMinMaxAttackDuration[3][2]; // slash, thrust, chop has different durations
            bool mMinMaxAttackDurationInitialised;

            bool mForceNoShortcut;
            ESM::Position mShortcutFailPos;

            Ogre::Vector3 mLastActorPos;
            MWMechanics::Movement mMovement;

            int mTargetActorId;
            Ogre::Vector3 mLastTargetPos;

            const MWWorld::CellStore* mCell;
            ObstacleCheck mObstacleCheck;

            boost::shared_ptr<Action> mCurrentAction;
            float mActionCooldown;

            void buildNewPath(const MWWorld::Ptr& actor, const MWWorld::Ptr& target);
    };
}

#endif
