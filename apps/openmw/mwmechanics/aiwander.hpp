#ifndef GAME_MWMECHANICS_AIWANDER_H
#define GAME_MWMECHANICS_AIWANDER_H

#include "aipackage.hpp"

#include <vector>

#include "pathfinding.hpp"
#include "obstacle.hpp"

#include "../mwworld/timestamp.hpp"


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
                \param timeOfDay Start time of the package, if it has a duration. Currently unimplemented
                \param idle Chances of each idle to play (9 in total)
                \param repeat Repeat wander or not **/
            AiWander(int distance, int duration, int timeOfDay, const std::vector<unsigned char>& idle, bool repeat);

            AiWander (const ESM::AiSequence::AiWander* wander);

            

            virtual AiPackage *clone() const;

            virtual bool execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration);

            virtual int getTypeId() const;

            /// Set the position to return to for a stationary (non-wandering) actor
            /** In case another AI package moved the actor elsewhere **/
            void setReturnPosition (const osg::Vec3f& position);

            virtual void writeState(ESM::AiSequence::AiSequence &sequence) const;

            virtual void fastForward(const MWWorld::Ptr& actor, AiState& state);
            
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
            
            void stopWalking(const MWWorld::Ptr& actor, AiWanderStorage& storage);

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
                const MWWorld::CellStore*& currentCell, bool cellChange, ESM::Position& pos);
            bool isPackageCompleted(const MWWorld::Ptr& actor, AiWanderStorage& storage);
            void returnToStartLocation(const MWWorld::Ptr& actor, AiWanderStorage& storage, ESM::Position& pos);

            int mDistance; // how far the actor can wander from the spawn point
            int mDuration;
            int mTimeOfDay;
            std::vector<unsigned char> mIdle;
            bool mRepeat;
            

            bool mHasReturnPosition; // NOTE: Could be removed if mReturnPosition was initialized to actor position,
                                    // if we had the actor in the AiWander constructor...
            osg::Vec3f mReturnPosition;

            osg::Vec3f mInitialActorPosition;
            bool mStoredInitialActorPosition;

           

            // do we need to calculate allowed nodes based on mDistance
            bool mPopulateAvailableNodes;


            

            MWWorld::TimeStamp mStartTime;

            // allowed pathgrid nodes based on mDistance from the spawn point
            std::vector<ESM::Pathgrid::Point> mAllowedNodes;

            void getAllowedNodes(const MWWorld::Ptr& actor, const ESM::Cell* cell);

            ESM::Pathgrid::Point mCurrentNode;
            bool mTrimCurrentNode;
            void trimAllowedNodes(std::vector<ESM::Pathgrid::Point>& nodes,
                                  const PathFinder& pathfinder);


//             ObstacleCheck mObstacleCheck;
            float mDoorCheckDuration;
            int mStuckCount;

            // constants for converting idleSelect values into groupNames
            enum GroupIndex
            {
                GroupIndex_MinIdle = 2,
                GroupIndex_MaxIdle = 9
            };

            /// convert point from local (i.e. cell) to world co-ordinates
            void ToWorldCoordinates(ESM::Pathgrid::Point& point, const ESM::Cell * cell);

            void SetCurrentNodeToClosestAllowedNode(osg::Vec3f npcPos);

            void AddNonPathGridAllowedPoints(osg::Vec3f npcPos, const ESM::Pathgrid * pathGrid, int pointIndex);

            void AddPointBetweenPathGridPoints(const ESM::Pathgrid::Point& start, const ESM::Pathgrid::Point& end);

            /// lookup table for converting idleSelect value to groupName
            static const std::string sIdleSelectToGroupName[GroupIndex_MaxIdle - GroupIndex_MinIdle + 1];

            static int OffsetToPreventOvercrowding();
    };
    
    
}

#endif
