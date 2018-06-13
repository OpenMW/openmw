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
    struct AiWanderStorage;

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

            virtual void writeState(ESM::AiSequence::AiSequence &sequence) const;

            virtual void fastForward(const MWWorld::Ptr& actor, AiState& state);

            bool getRepeat() const;

            osg::Vec3f getDestination(const MWWorld::Ptr& actor) const;

            enum GreetingState {
                Greet_None,
                Greet_InProgress,
                Greet_Done
            };

            enum WanderState {
                Wander_ChooseAction,
                Wander_IdleNow,
                Wander_MoveNow,
                Wander_Walking
            };

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
            void playGreetingIfPlayerGetsTooClose(const MWWorld::Ptr& actor, AiWanderStorage& storage);
            void evadeObstacles(const MWWorld::Ptr& actor, AiWanderStorage& storage, float duration, ESM::Position& pos);
            void playIdleDialogueRandomly(const MWWorld::Ptr& actor);
            void turnActorToFacePlayer(const osg::Vec3f& actorPosition, const osg::Vec3f& playerPosition, AiWanderStorage& storage);
            void doPerFrameActionsForState(const MWWorld::Ptr& actor, float duration, AiWanderStorage& storage, ESM::Position& pos);
            void onIdleStatePerFrameActions(const MWWorld::Ptr& actor, float duration, AiWanderStorage& storage);
            void onWalkingStatePerFrameActions(const MWWorld::Ptr& actor, float duration, AiWanderStorage& storage, ESM::Position& pos);
            void onChooseActionStatePerFrameActions(const MWWorld::Ptr& actor, AiWanderStorage& storage);
            bool reactionTimeActions(const MWWorld::Ptr& actor, AiWanderStorage& storage,
            const MWWorld::CellStore*& currentCell, bool cellChange, ESM::Position& pos, float duration);
            bool isPackageCompleted(const MWWorld::Ptr& actor, AiWanderStorage& storage);
            void wanderNearStart(const MWWorld::Ptr &actor, AiWanderStorage &storage, int wanderDistance);
            bool destinationIsAtWater(const MWWorld::Ptr &actor, const osg::Vec3f& destination);
            bool destinationThroughGround(const osg::Vec3f& startPoint, const osg::Vec3f& destination);
            void completeManualWalking(const MWWorld::Ptr &actor, AiWanderStorage &storage);

            int mDistance; // how far the actor can wander from the spawn point
            int mDuration;
            float mRemainingDuration;
            int mTimeOfDay;
            std::vector<unsigned char> mIdle;
            bool mRepeat;

            bool mStoredInitialActorPosition;
            osg::Vec3f mInitialActorPosition;

            bool mHasDestination;
            osg::Vec3f mDestination;

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
