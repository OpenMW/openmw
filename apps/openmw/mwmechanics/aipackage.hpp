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
    const float AI_REACTION_TIME = 0.25f;

    class CharacterController;
    class PathgridGraph;

    /// \brief Base class for AI packages
    class AiPackage
    {
        public:
            ///Enumerates the various AITypes available
            enum TypeId {
                TypeIdNone = -1,
                TypeIdWander = 0,
                TypeIdTravel = 1,
                TypeIdEscort = 2,
                TypeIdFollow = 3,
                TypeIdActivate = 4,

                // These 5 are not really handled as Ai Packages in the MW engine
                // For compatibility do *not* return these in the getCurrentAiPackage script function..
                TypeIdCombat = 5,
                TypeIdPursue = 6,
                TypeIdAvoidDoor = 7,
                TypeIdFace = 8,
                TypeIdBreathe = 9,
                TypeIdInternalTravel = 10,
                TypeIdCast = 11
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

            /// Check if package use movement with variable speed
            virtual bool useVariableSpeed() const { return false;}

            virtual void writeState (ESM::AiSequence::AiSequence& sequence) const {}

            /// Simulates the passing of time
            virtual void fastForward(const MWWorld::Ptr& actor, AiState& state) {}

            /// Get the target actor the AI is targeted at (not applicable to all AI packages, default return empty Ptr)
            virtual MWWorld::Ptr getTarget() const;

            /// Get the destination point of the AI package (not applicable to all AI packages, default return (0, 0, 0))
            virtual osg::Vec3f getDestination(const MWWorld::Ptr& actor) const { return osg::Vec3f(0, 0, 0); };

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

            virtual osg::Vec3f getDestination() const { return osg::Vec3f(0, 0, 0); }

            // Return true if any loaded actor with this AI package must be active.
            virtual bool alwaysActive() const { return false; }

            /// Reset pathfinding state
            void reset();

            /// Return if actor's rotation speed is sufficient to rotate to the destination pathpoint on the run. Otherwise actor should rotate while standing.
            static bool isReachableRotatingOnTheRun(const MWWorld::Ptr& actor, const osg::Vec3f& dest);

        protected:
            /// Handles path building and shortcutting with obstacles avoiding
            /** \return If the actor has arrived at his destination **/
            bool pathTo(const MWWorld::Ptr& actor, const osg::Vec3f& dest, float duration, float destTolerance = 0.0f);

            /// Check if there aren't any obstacles along the path to make shortcut possible
            /// If a shortcut is possible then path will be cleared and filled with the destination point.
            /// \param destInLOS If not nullptr function will return ray cast check result
            /// \return If can shortcut the path
            bool shortcutPath(const osg::Vec3f& startPoint, const osg::Vec3f& endPoint, const MWWorld::Ptr& actor,
                              bool *destInLOS, bool isPathClear);

            /// Check if the way to the destination is clear, taking into account actor speed
            bool checkWayIsClearForActor(const osg::Vec3f& startPoint, const osg::Vec3f& endPoint, const MWWorld::Ptr& actor);

            bool doesPathNeedRecalc(const osg::Vec3f& newDest, const MWWorld::Ptr& actor) const;

            void evadeObstacles(const MWWorld::Ptr& actor);

            void openDoors(const MWWorld::Ptr& actor);

            const PathgridGraph& getPathGridGraph(const MWWorld::CellStore* cell);

            DetourNavigator::Flags getNavigatorFlags(const MWWorld::Ptr& actor) const;

            // TODO: all this does not belong here, move into temporary storage
            PathFinder mPathFinder;
            ObstacleCheck mObstacleCheck;

            float mTimer;

            std::string mTargetActorRefId;
            mutable int mTargetActorId;

            short mRotateOnTheRunChecks; // attempts to check rotation to the pathpoint on the run possibility

            bool mIsShortcutting;   // if shortcutting at the moment
            bool mShortcutProhibited; // shortcutting may be prohibited after unsuccessful attempt
            osg::Vec3f mShortcutFailPos; // position of last shortcut fail

        private:
            bool isNearInactiveCell(osg::Vec3f position);
    };
}

#endif

