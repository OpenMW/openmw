#ifndef GAME_MWMECHANICS_AIPACKAGE_H
#define GAME_MWMECHANICS_AIPACKAGE_H

#include "pathfinding.hpp"
#include <components/esm/defs.hpp>

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
            virtual MWWorld::Ptr getTarget();

            /// Return true if having this AiPackage makes the actor side with the target in fights (default false)
            virtual bool sideWithTarget() const;

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

            bool isWithinMaxRange(const osg::Vec3f& pos1, const osg::Vec3f& pos2) const
            {
                // Maximum travel distance for vanilla compatibility.
                // Was likely meant to prevent NPCs walking into non-loaded exterior cells, but for some reason is used in interior cells as well.
                // We can make this configurable at some point, but the default *must* be the below value. Anything else will break shoddily-written content (*cough* MW *cough*) in bizarre ways.
                return (pos1 - pos2).length2() <= 7168*7168;
            }

        private:
            bool isNearInactiveCell(const ESM::Position& actorPos);

    };

}

#endif

