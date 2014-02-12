#ifndef GAME_MWMECHANICS_AICOMBAT_H
#define GAME_MWMECHANICS_AICOMBAT_H

#include "aipackage.hpp"

#include "pathfinding.hpp"

#include "movement.hpp"

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

            // the z rotation angle (degrees) we want to reach
            // used every frame when mRotate is true
            float mTargetAngle;

            bool mReadyToAttack, mStrike;
            bool mFollowTarget;
            bool mCombatMove;
            bool mRotate;

            MWMechanics::Movement mMovement;
            MWWorld::Ptr mTarget;

            void buildNewPath(const MWWorld::Ptr& actor);
    };
}

#endif
