#ifndef GAME_MWMECHANICS_AIWANDER_H
#define GAME_MWMECHANICS_AIWANDER_H

#include "aipackage.hpp"

#include <vector>

#include "../mwworld/timestamp.hpp"

#include "pathfinding.hpp"
#include "obstacle.hpp"
#include "aistate.hpp"

namespace ESM
{
    struct Cell;
    namespace AiSequence
    {
        struct AiWander;
    }
}

namespace MWMechanics
{
    /// \brief This class holds the variables AiWander needs which are deleted if the package becomes inactive.
    struct AiWanderStorage : AiTemporaryBase
    {
        float mReaction; // update some actions infrequently

        // AiWander states
        enum WanderState
        {
            Wander_ChooseAction,
            Wander_IdleNow,
            Wander_MoveNow,
            Wander_Walking
        };
        WanderState mState;

        bool mIsWanderingManually;
        bool mCanWanderAlongPathGrid;

        unsigned short mIdleAnimation;
        std::vector<unsigned short> mBadIdles; // Idle animations that when called cause errors

        // do we need to calculate allowed nodes based on mDistance
        bool mPopulateAvailableNodes;

        // allowed pathgrid nodes based on mDistance from the spawn point
        // in local coordinates of mCell
        std::vector<ESM::Pathgrid::Point> mAllowedNodes;

        ESM::Pathgrid::Point mCurrentNode;
        bool mTrimCurrentNode;

        float mDoorCheckDuration;
        int mStuckCount;

        AiWanderStorage():
            mReaction(0),
            mState(Wander_ChooseAction),
            mIsWanderingManually(false),
            mCanWanderAlongPathGrid(true),
            mIdleAnimation(0),
            mBadIdles(),
            mPopulateAvailableNodes(true),
            mAllowedNodes(),
            mTrimCurrentNode(false),
            mDoorCheckDuration(0), // TODO: maybe no longer needed
            mStuckCount(0)
            {};

        void setState(const WanderState wanderState, const bool isManualWander = false)
        {
            mState = wanderState;
            mIsWanderingManually = isManualWander;
        }
    };

    /// \brief Causes the Actor to wander within a specified range
    class AiWander : public AiPackage
    {
        public:
            /// Constructor
            /** \param distance Max distance the ACtor will wander
                \param duration Time, in hours, that this package will be preformed
                \param timeOfDay Currently unimplemented. Not functional in the original engine.
                \param idle Chances of each idle to play (9 in total)
                \param repeat Repeat wander or not **/
            AiWander(int distance, int duration, int timeOfDay, const std::vector<unsigned char>& idle, bool repeat);

            AiWander (const ESM::AiSequence::AiWander* wander);

            virtual AiPackage *clone() const;

            virtual bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration);

            virtual int getTypeId() const;

            virtual bool useVariableSpeed() const { return true;}

            virtual void writeState(ESM::AiSequence::AiSequence &sequence) const;

            virtual void fastForward(const MWWorld::Ptr& actor, AiState& state);

            bool getRepeat() const;

            osg::Vec3f getDestination(const MWWorld::Ptr& actor) const;

            virtual osg::Vec3f getDestination() const
            {
                if (!mHasDestination)
                    return osg::Vec3f(0, 0, 0);

                return mDestination;
            }

        private:
            // NOTE: mDistance and mDuration must be set already
            void init();
            void stopWalking(const MWWorld::Ptr& actor, AiWanderStorage& storage, bool clearPath = true);

            /// Have the given actor play an idle animation
            /// @return Success or error
            bool playIdle(const MWWorld::Ptr& actor, unsigned short idleSelect);
            bool checkIdle(const MWWorld::Ptr& actor, unsigned short idleSelect);
            short unsigned getRandomIdle();
            void setPathToAnAllowedNode(const MWWorld::Ptr& actor, AiWanderStorage& storage, const ESM::Position& actorPos);
            void evadeObstacles(const MWWorld::Ptr& actor, float duration, AiWanderStorage& storage);
            void turnActorToFacePlayer(const osg::Vec3f& actorPosition, const osg::Vec3f& playerPosition, AiWanderStorage& storage);
            void doPerFrameActionsForState(const MWWorld::Ptr& actor, float duration, AiWanderStorage& storage);
            void onIdleStatePerFrameActions(const MWWorld::Ptr& actor, float duration, AiWanderStorage& storage);
            void onWalkingStatePerFrameActions(const MWWorld::Ptr& actor, float duration, AiWanderStorage& storage);
            void onChooseActionStatePerFrameActions(const MWWorld::Ptr& actor, AiWanderStorage& storage);
            bool reactionTimeActions(const MWWorld::Ptr& actor, AiWanderStorage& storage, ESM::Position& pos);
            bool isPackageCompleted(const MWWorld::Ptr& actor, AiWanderStorage& storage);
            void wanderNearStart(const MWWorld::Ptr &actor, AiWanderStorage &storage, int wanderDistance);
            bool destinationIsAtWater(const MWWorld::Ptr &actor, const osg::Vec3f& destination);
            void completeManualWalking(const MWWorld::Ptr &actor, AiWanderStorage &storage);

            int mDistance; // how far the actor can wander from the spawn point
            int mDuration;
            float mRemainingDuration;
            int mTimeOfDay;
            std::vector<unsigned char> mIdle;
            bool mRepeat;

            bool mStoredInitialActorPosition;
            osg::Vec3f mInitialActorPosition; // Note: an original engine does not reset coordinates even when actor changes a cell

            bool mHasDestination;
            osg::Vec3f mDestination;
            bool mUsePathgrid;

            void getNeighbouringNodes(ESM::Pathgrid::Point dest, const MWWorld::CellStore* currentCell, ESM::Pathgrid::PointList& points);

            void getAllowedNodes(const MWWorld::Ptr& actor, const ESM::Cell* cell, AiWanderStorage& storage);

            void trimAllowedNodes(std::vector<ESM::Pathgrid::Point>& nodes, const PathFinder& pathfinder);

            // constants for converting idleSelect values into groupNames
            enum GroupIndex
            {
                GroupIndex_MinIdle = 2,
                GroupIndex_MaxIdle = 9
            };

            /// convert point from local (i.e. cell) to world coordinates
            void ToWorldCoordinates(ESM::Pathgrid::Point& point, const ESM::Cell * cell);

            void SetCurrentNodeToClosestAllowedNode(const osg::Vec3f& npcPos, AiWanderStorage& storage);

            void AddNonPathGridAllowedPoints(osg::Vec3f npcPos, const ESM::Pathgrid * pathGrid, int pointIndex, AiWanderStorage& storage);

            void AddPointBetweenPathGridPoints(const ESM::Pathgrid::Point& start, const ESM::Pathgrid::Point& end, AiWanderStorage& storage);

            /// lookup table for converting idleSelect value to groupName
            static const std::string sIdleSelectToGroupName[GroupIndex_MaxIdle - GroupIndex_MinIdle + 1];

            static int OffsetToPreventOvercrowding();
    }; 
}

#endif
