#ifndef GAME_MWMECHANICS_AIWANDER_H
#define GAME_MWMECHANICS_AIWANDER_H

#include "aitemporarybase.hpp"
#include "aitimer.hpp"
#include "pathfinding.hpp"
#include "typedaipackage.hpp"

#include <components/esm3/loadpgrd.hpp>

#include <string_view>
#include <vector>

namespace ESM
{
    struct Cell;
    namespace AiSequence
    {
        struct AiWander;
    }
}

namespace Misc
{
    class CoordinateConverter;
}

namespace MWWorld
{
    class Cell;
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

        bool mPopulateAvailablePositions;

        // allowed destination positions based on mDistance from the spawn point
        std::vector<osg::Vec3f> mAllowedPositions;

        osg::Vec3f mCurrentPosition;
        bool mTrimCurrentPosition;

        float mCheckIdlePositionTimer;
        int mStuckCount;

        AiWanderStorage();

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

        explicit AiWander(const ESM::AiSequence::AiWander* wander);

        bool execute(const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state,
            float duration) override;

        static constexpr AiPackageTypeId getTypeId() { return AiPackageTypeId::Wander; }

        static constexpr Options makeDefaultOptions()
        {
            AiPackage::Options options;
            options.mUseVariableSpeed = true;
            return options;
        }

        void writeState(ESM::AiSequence::AiSequence& sequence) const override;

        void fastForward(const MWWorld::Ptr& actor, AiState& state) override;

        osg::Vec3f getDestination(const MWWorld::Ptr& actor) const override;

        osg::Vec3f getDestination() const override
        {
            if (!mHasDestination)
                return osg::Vec3f(0, 0, 0);

            return mDestination;
        }

        bool isStationary() const { return mDistance == 0; }

        std::optional<int> getDistance() const override { return mDistance; }

        std::optional<float> getDuration() const override { return static_cast<float>(mDuration); }

        const std::vector<unsigned char>& getIdle() const { return mIdle; }

        static std::string_view getIdleGroupName(size_t index) { return sIdleSelectToGroupName[index]; }

        void resetInitialPosition() override;

    private:
        void stopWalking(const MWWorld::Ptr& actor);

        /// Have the given actor play an idle animation
        /// @return Success or error
        bool playIdle(const MWWorld::Ptr& actor, unsigned short idleSelect);
        bool checkIdle(const MWWorld::Ptr& actor, unsigned short idleSelect);
        unsigned short getRandomIdle() const;
        void setPathToAnAllowedPosition(
            const MWWorld::Ptr& actor, AiWanderStorage& storage, const ESM::Position& actorPos);
        void evadeObstacles(const MWWorld::Ptr& actor, AiWanderStorage& storage);
        void doPerFrameActionsForState(const MWWorld::Ptr& actor, float duration,
            MWWorld::MovementDirectionFlags supportedMovementDirections, AiWanderStorage& storage);
        void onIdleStatePerFrameActions(const MWWorld::Ptr& actor, float duration, AiWanderStorage& storage);
        void onWalkingStatePerFrameActions(const MWWorld::Ptr& actor, float duration,
            MWWorld::MovementDirectionFlags supportedMovementDirections, AiWanderStorage& storage);
        void onChooseActionStatePerFrameActions(const MWWorld::Ptr& actor, AiWanderStorage& storage);
        bool reactionTimeActions(const MWWorld::Ptr& actor, AiWanderStorage& storage, ESM::Position& pos);
        inline bool isPackageCompleted() const;
        void wanderNearStart(const MWWorld::Ptr& actor, AiWanderStorage& storage, int wanderDistance);
        bool destinationIsAtWater(const MWWorld::Ptr& actor, const osg::Vec3f& destination);
        void completeManualWalking(const MWWorld::Ptr& actor, AiWanderStorage& storage);
        bool isNearAllowedPosition(const MWWorld::Ptr& actor, const AiWanderStorage& storage, float distance) const;

        // how far the actor can wander from the spawn point
        const unsigned mDistance;
        const unsigned mDuration;
        float mRemainingDuration;
        const int mTimeOfDay;
        const std::vector<unsigned char> mIdle;

        bool mStoredInitialActorPosition;
        // Note: an original engine does not reset coordinates even when actor changes a cell
        osg::Vec3f mInitialActorPosition;

        bool mHasDestination;
        osg::Vec3f mDestination;
        bool mUsePathgrid;

        void getNeighbouringNodes(
            const osg::Vec3f& dest, const MWWorld::CellStore* currentCell, ESM::Pathgrid::PointList& points);

        void fillAllowedPositions(const MWWorld::Ptr& actor, AiWanderStorage& storage);

        // constants for converting idleSelect values into groupNames
        enum GroupIndex
        {
            GroupIndex_MinIdle = 2,
            GroupIndex_MaxIdle = 9
        };

        void setCurrentPositionToClosestAllowedPosition(AiWanderStorage& storage);

        void addNonPathGridAllowedPoints(const ESM::Pathgrid* pathGrid, size_t pointIndex, AiWanderStorage& storage,
            const Misc::CoordinateConverter& converter);

        void addPositionBetweenPathgridPoints(
            const ESM::Pathgrid::Point& start, const ESM::Pathgrid::Point& end, AiWanderStorage& storage);

        /// lookup table for converting idleSelect value to groupName
        static const std::string_view sIdleSelectToGroupName[GroupIndex_MaxIdle - GroupIndex_MinIdle + 1];
    };
}

#endif
