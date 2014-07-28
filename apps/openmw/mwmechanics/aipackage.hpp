#ifndef GAME_MWMECHANICS_AIPACKAGE_H
#define GAME_MWMECHANICS_AIPACKAGE_H

#include "pathfinding.hpp"
#include <components/esm/defs.hpp>
#include "../mwbase/world.hpp"

#include "obstacle.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace ESM
{
    namespace AiSequence
    {
        class AiSequence;
    }
}

namespace MWMechanics
{
    /// \brief Base class for AI packages
    class AiPackage
    {
        public:
            ///Enumerates the various AITypes availible.
            enum TypeId {
                TypeIdNone = -1,
                TypeIdWander = 0,
                TypeIdTravel = 1,
                TypeIdEscort = 2,
                TypeIdFollow = 3,
                TypeIdActivate = 4,
                TypeIdCombat = 5,
                TypeIdPursue = 6,
                TypeIdAvoidDoor = 7
            };

            ///Default constructor
            AiPackage();

            ///Default Deconstructor
            virtual ~AiPackage();

            ///Clones the package
            virtual AiPackage *clone() const = 0;

            /// Updates and runs the package (Should run every frame)
            /// \return Package completed?
            virtual bool execute (const MWWorld::Ptr& actor,float duration) = 0;

            /// Returns the TypeID of the AiPackage
            /// \see enum TypeId
            virtual int getTypeId() const = 0;

            /// Higher number is higher priority (0 being the lowest)
            virtual unsigned int getPriority() const {return 0;}

            virtual void writeState (ESM::AiSequence::AiSequence& sequence) const {}

        protected:
            /// Causes the actor to attempt to walk to the specified location
            /** \return If the actor has arrived at his destination **/
            bool pathTo(const MWWorld::Ptr& actor, ESM::Pathgrid::Point dest, float duration);

            PathFinder mPathFinder;
            ObstacleCheck mObstacleCheck;

            float mDoorCheckDuration;
            float mTimer;
            float mStuckTimer;

            ESM::Position mStuckPos;
            ESM::Pathgrid::Point mPrevDest;
    };
}

#endif

