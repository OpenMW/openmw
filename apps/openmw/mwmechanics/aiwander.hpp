#ifndef GAME_MWMECHANICS_AIWANDER_H
#define GAME_MWMECHANICS_AIWANDER_H

#include "typedaipackage.hpp"

#include <vector>

#include "pathfinding.hpp"
#include "obstacle.hpp"
#include "aistate.hpp"
#include "aitimer.hpp"

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
        AiReactionTimer mReaction;

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

        float mCheckIdlePositionTimer;
        int mStuckCount;

        AiWanderStorage():
            mState(Wander_ChooseAction),
            mIsWanderingManually(false),
            mCanWanderAlongPathGrid(true),
            mIdleAnimation(0),
            mBadIdles(),
            mPopulateAvailableNodes(true),
            mAllowedNodes(),
            mTrimCurrentNode(false),
            mCheckIdlePositionTimer(0),
            mStuckCount(0)
            {};

        void setState(const WanderState wanderState, const bool isManualWander = false)
        {
            mState = wanderState;
            mIsWanderingManually = isManualWander;
        }
    };

    /// \brief Causes the Actor to wander within a specified range
    class AiWander final : public TypedAiPackage<AiWander>
    {
        public:
            /// Constructor
            /** \param distance Max distance the ACtor will wander
                \param duration Time, in hours, that this package will be preformed
                \param timeOfDay Currently unimplemented. Not functional in the original engine.
                \param idle Chances of each idle to play (9 in total)
                \param repeat Repeat wander or not **/
            AiWander(int distance, int duration, int timeOfDay, const std::vector<unsigned char>& idle, bool repeat);

            explicit AiWander (const ESM::AiSequence::AiWander* wander);

            bool execute(const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration) override;

            static constexpr AiPackageTypeId getTypeId() { return AiPackageTypeId::Wander; }

            static constexpr Options makeDefaultOptions()
            {
                AiPackage::Options options;
                options.mUseVariableSpeed = true;
                return options;
            }

            void writeState(ESM::AiSequence::AiSequence &sequence) const override;

            void fastForward(const MWWorld::Ptr& actor, AiState& state) override;

            osg::Vec3f getDestination(const MWWorld::Ptr& actor) const override;

            osg::Vec3f getDestination() const override
            {
                if (!mHasDestination)
                    return osg::Vec3f(0, 0, 0);

                return mDestination;
            }

            bool isStationary() const { return mDistance == 0; }

        private:
            void stopWalking(const MWWorld::Ptr& actor);

            /// Have the given actor play an idle animation
            /// @return Success or error
            bool playIdle(const MWWorld::Ptr& actor, unsigned short idleSelect);
            bool checkIdle(const MWWorld::Ptr& actor, unsigned short idleSelect);
            short unsigned getRandomIdle();
            void setPathToAnAllowedNode(const MWWorld::Ptr& actor, AiWanderStorage& storage, const ESM::Position& actorPos);
            void evadeObstacles(const MWWorld::Ptr& actor, AiWanderStorage& storage);
            void doPerFrameActionsForState(const MWWorld::Ptr& actor, float duration, AiWanderStorage& storage);
            void onIdleStatePerFrameActions(const MWWorld::Ptr& actor, float duration, AiWanderStorage& storage);
            void onWalkingStatePerFrameActions(const MWWorld::Ptr& actor, float duration, AiWanderStorage& storage);
            void onChooseActionStatePerFrameActions(const MWWorld::Ptr& actor, AiWanderStorage& storage);
            bool reactionTimeActions(const MWWorld::Ptr& actor, AiWanderStorage& storage, ESM::Position& pos);
            inline bool isPackageCompleted() const;
            void wanderNearStart(const MWWorld::Ptr &actor, AiWanderStorage &storage, int wanderDistance);
            bool destinationIsAtWater(const MWWorld::Ptr &actor, const osg::Vec3f& destination);
            void completeManualWalking(const MWWorld::Ptr &actor, AiWanderStorage &storage);
            bool isNearAllowedNode(const MWWorld::Ptr &actor, const AiWanderStorage& storage, float distance) const;

            const int mDistance; // how far the actor can wander from the spawn point
            const int mDuration;
            float mRemainingDuration;
            const int mTimeOfDay;
            const std::vector<unsigned char> mIdle;

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
