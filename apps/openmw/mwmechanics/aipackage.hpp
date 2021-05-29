#ifndef GAME_MWMECHANICS_AIPACKAGE_H
#define GAME_MWMECHANICS_AIPACKAGE_H

#include <memory>

#include <components/detournavigator/areatype.hpp>

#include "pathfinding.hpp"
#include "obstacle.hpp"
#include "aistate.hpp"
#include "aipackagetypeid.hpp"
#include "aitimer.hpp"

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
    class PathgridGraph;

    /// \brief Base class for AI packages
    class AiPackage
    {
        public:
            struct Options
            {
                unsigned int mPriority = 0;
                bool mUseVariableSpeed = false;
                bool mSideWithTarget = false;
                bool mFollowTargetThroughDoors = false;
                bool mCanCancel = true;
                bool mShouldCancelPreviousAi = true;
                bool mRepeat = false;
                bool mAlwaysActive = false;

                constexpr Options withRepeat(bool value)
                {
                    mRepeat = value;
                    return *this;
                }

                constexpr Options withShouldCancelPreviousAi(bool value)
                {
                    mShouldCancelPreviousAi = value;
                    return *this;
                }
            };

            AiPackage(AiPackageTypeId typeId, const Options& options);

            virtual ~AiPackage() = default;

            static constexpr Options makeDefaultOptions()
            {
                return Options{};
            }

            ///Clones the package
            virtual std::unique_ptr<AiPackage> clone() const = 0;

            /// Updates and runs the package (Should run every frame)
            /// \return Package completed?
            virtual bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration) = 0;

            /// Returns the TypeID of the AiPackage
            /// \see enum TypeId
            AiPackageTypeId getTypeId() const { return mTypeId; }

            /// Higher number is higher priority (0 being the lowest)
            unsigned int getPriority() const { return mOptions.mPriority; }

            /// Check if package use movement with variable speed
            bool useVariableSpeed() const { return mOptions.mUseVariableSpeed; }

            virtual void writeState (ESM::AiSequence::AiSequence& sequence) const {}

            /// Simulates the passing of time
            virtual void fastForward(const MWWorld::Ptr& actor, AiState& state) {}

            /// Get the target actor the AI is targeted at (not applicable to all AI packages, default return empty Ptr)
            virtual MWWorld::Ptr getTarget() const;

            /// Get the destination point of the AI package (not applicable to all AI packages, default return (0, 0, 0))
            virtual osg::Vec3f getDestination(const MWWorld::Ptr& actor) const { return osg::Vec3f(0, 0, 0); };

            /// Return true if having this AiPackage makes the actor side with the target in fights (default false)
            bool sideWithTarget() const { return mOptions.mSideWithTarget; }

            /// Return true if the actor should follow the target through teleport doors (default false)
            bool followTargetThroughDoors() const { return mOptions.mFollowTargetThroughDoors; }

            /// Can this Ai package be canceled? (default true)
            bool canCancel() const { return mOptions.mCanCancel; }

            /// Upon adding this Ai package, should the Ai Sequence attempt to cancel previous Ai packages (default true)?
            bool shouldCancelPreviousAi() const { return mOptions.mShouldCancelPreviousAi; }

            /// Return true if this package should repeat. Currently only used for Wander packages.
            bool getRepeat() const { return mOptions.mRepeat; }

            virtual osg::Vec3f getDestination() const { return osg::Vec3f(0, 0, 0); }

            /// Return true if any loaded actor with this AI package must be active.
            bool alwaysActive() const { return mOptions.mAlwaysActive; }

            /// Reset pathfinding state
            void reset();

            /// Return if actor's rotation speed is sufficient to rotate to the destination pathpoint on the run. Otherwise actor should rotate while standing.
            static bool isReachableRotatingOnTheRun(const MWWorld::Ptr& actor, const osg::Vec3f& dest);

            osg::Vec3f getNextPathPoint(const osg::Vec3f& destination) const;

            float getNextPathPointTolerance(float speed, float duration, const osg::Vec3f& halfExtents) const;

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

            DetourNavigator::AreaCosts getAreaCosts(const MWWorld::Ptr& actor) const;

            const AiPackageTypeId mTypeId;
            const Options mOptions;

            // TODO: all this does not belong here, move into temporary storage
            PathFinder mPathFinder;
            ObstacleCheck mObstacleCheck;

            AiReactionTimer mReaction;

            std::string mTargetActorRefId;
            mutable int mTargetActorId;

            short mRotateOnTheRunChecks; // attempts to check rotation to the pathpoint on the run possibility

            bool mIsShortcutting;   // if shortcutting at the moment
            bool mShortcutProhibited; // shortcutting may be prohibited after unsuccessful attempt
            osg::Vec3f mShortcutFailPos; // position of last shortcut fail
            float mLastDestinationTolerance = 0;

        private:
            bool isNearInactiveCell(osg::Vec3f position);
    };
}

#endif

