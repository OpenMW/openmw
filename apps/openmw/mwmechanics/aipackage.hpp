#ifndef GAME_MWMECHANICS_AIPACKAGE_H
#define GAME_MWMECHANICS_AIPACKAGE_H

#include <components/esm/defs.hpp>

#include "pathfinding.hpp"
#include "obstacle.hpp"
#include "aistate.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace ESM
{
    struct Cell;
    namespace AiSequence
    {
        struct AiSequence;
    }
}


namespace MWMechanics
{

    class CharacterController;

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

                // These 4 are not really handled as Ai Packages in the MW engine
                // For compatibility do *not* return these in the getCurrentAiPackage script function..
                TypeIdCombat = 5,
                TypeIdPursue = 6,
                TypeIdAvoidDoor = 7,
                TypeIdFace = 8
            };

            ///Default constructor
            AiPackage();

            ///Default Deconstructor
            virtual ~AiPackage();

            ///Clones the package
            virtual AiPackage *clone() const = 0;

            /// Updates and runs the package (Should run every frame)
            /// \return Package completed?
            virtual bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration) = 0;

            /// Returns the TypeID of the AiPackage
            /// \see enum TypeId
            virtual int getTypeId() const = 0;

            /// Higher number is higher priority (0 being the lowest)
            virtual unsigned int getPriority() const {return 0;}

            virtual void writeState (ESM::AiSequence::AiSequence& sequence) const {}

            /// Simulates the passing of time
            virtual void fastForward(const MWWorld::Ptr& actor, AiState& state) {}

            /// Get the target actor the AI is targeted at (not applicable to all AI packages, default return empty Ptr)
            virtual MWWorld::Ptr getTarget() const;

            /// Return true if having this AiPackage makes the actor side with the target in fights (default false)
            virtual bool sideWithTarget() const;

            /// Return true if the actor should follow the target through teleport doors (default false)
            virtual bool followTargetThroughDoors() const;

            /// Can this Ai package be canceled? (default true)
            virtual bool canCancel() const;

            /// Upon adding this Ai package, should the Ai Sequence attempt to cancel previous Ai packages (default true)?
            virtual bool shouldCancelPreviousAi() const;

            /// Return true if this package should repeat. Currently only used for Wander packages.
            virtual bool getRepeat() const;

            bool isTargetMagicallyHidden(const MWWorld::Ptr& target);

        protected:
            /// Causes the actor to attempt to walk to the specified location
            /** \return If the actor has arrived at his destination **/
            bool pathTo(const MWWorld::Ptr& actor, ESM::Pathgrid::Point dest, float duration);

            virtual bool doesPathNeedRecalc(ESM::Pathgrid::Point dest, const ESM::Cell *cell);

            void evadeObstacles(const MWWorld::Ptr& actor, float duration, const ESM::Position& pos);

            // TODO: all this does not belong here, move into temporary storage
            PathFinder mPathFinder;
            ObstacleCheck mObstacleCheck;

            float mTimer;

            ESM::Pathgrid::Point mPrevDest;

        private:
            bool isNearInactiveCell(const ESM::Position& actorPos);

    };
}

#endif

