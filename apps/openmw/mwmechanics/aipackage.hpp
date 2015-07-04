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
    const float AI_REACTION_TIME = 0.25f;

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
            virtual bool execute (const MWWorld::Ptr& actor, AiState& state, float duration) = 0;

            /// Returns the TypeID of the AiPackage
            /// \see enum TypeId
            virtual int getTypeId() const = 0;

            /// Higher number is higher priority (0 being the lowest)
            virtual unsigned int getPriority() const {return 0;}

            virtual void writeState (ESM::AiSequence::AiSequence& sequence) const {}

            /// Simulates the passing of time
            virtual void fastForward(const MWWorld::Ptr& actor, AiState& state) {}

        protected:
            /// Causes the actor to attempt to walk to the specified location
            /** \return If the actor has arrived at his destination **/
            bool pathTo(const MWWorld::Ptr& actor, const ESM::Pathgrid::Point& dest, float duration, float destTolerance = 0.0f);

            /// Check if there aren't any obstacles along the path to make shortcut possible
            /// If a shortcut is possible then path will be cleared and filled with the destination point.
            /// \param destInLOS If not NULL function will return ray cast check result
            /// \return If can shortcut the path
            bool shortcutPath(const ESM::Pathgrid::Point& startPoint, const ESM::Pathgrid::Point& endPoint, const MWWorld::Ptr& actor, bool *destInLOS);

            /// Check if the way to the destination is clear, taking into account actor speed
            bool checkWayIsClearForActor(const ESM::Pathgrid::Point& startPoint, const ESM::Pathgrid::Point& endPoint, const MWWorld::Ptr& actor);

            virtual bool doesPathNeedRecalc(ESM::Pathgrid::Point dest, const ESM::Cell *cell);

            // TODO: all this does not belong here, move into temporary storage
            PathFinder mPathFinder;
            ObstacleCheck mObstacleCheck;

            float mTimer;
            float mStuckTimer;

            ESM::Position mStuckPos;
            ESM::Pathgrid::Point mPrevDest;

            bool mShortcutProhibited; // shortcutting may be prohibited after unsuccessful attempt
            ESM::Pathgrid::Point mShortcutFailPos; // position of last shortcut fail
    };
}

#endif

