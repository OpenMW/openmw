#include "aiwander.hpp"

#include <cfloat>
#include <iostream>

#include <components/misc/rng.hpp>

#include <components/esm/aisequence.hpp>

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"

#include "creaturestats.hpp"
#include "steering.hpp"
#include "movement.hpp"
#include "coordinateconverter.hpp"
#include "actorutil.hpp"



namespace MWMechanics
{
    static const int COUNT_BEFORE_RESET = 10;
    static const float DOOR_CHECK_INTERVAL = 1.5f;
    static const float REACTION_INTERVAL = 0.25f;
    static const int GREETING_SHOULD_START = 4; //how many reaction intervals should pass before NPC can greet player
    static const int GREETING_SHOULD_END = 10;

    // to prevent overcrowding
    static const int DESTINATION_TOLERANCE = 64;

    // distance must be long enough that NPC will need to move to get there.
    static const int MINIMUM_WANDER_DISTANCE = DESTINATION_TOLERANCE * 2;

    const std::string AiWander::sIdleSelectToGroupName[GroupIndex_MaxIdle - GroupIndex_MinIdle + 1] =
    {
        std::string("idle2"),
        std::string("idle3"),
        std::string("idle4"),
        std::string("idle5"),
        std::string("idle6"),
        std::string("idle7"),
        std::string("idle8"),
        std::string("idle9"),
    };

    /// \brief This class holds the variables AiWander needs which are deleted if the package becomes inactive.
    struct AiWanderStorage : AiTemporaryBase
    {
        // the z rotation angle to reach
        // when mTurnActorGivingGreetingToFacePlayer is true
        float mTargetAngleRadians;
        bool mTurnActorGivingGreetingToFacePlayer;
        float mReaction; // update some actions infrequently
        
        AiWander::GreetingState mSaidGreeting;
        int mGreetingTimer;

        const MWWorld::CellStore* mCell; // for detecting cell change

        // AiWander states
        AiWander::WanderState mState;
        
        unsigned short mIdleAnimation;
        std::vector<unsigned short> mBadIdles; // Idle animations that when called cause errors

        PathFinder mPathFinder;
        
        AiWanderStorage():
            mTargetAngleRadians(0),
            mTurnActorGivingGreetingToFacePlayer(false),
            mReaction(0),
            mSaidGreeting(AiWander::Greet_None),
            mGreetingTimer(0),
            mCell(NULL),
            mState(AiWander::Wander_ChooseAction),
            mIdleAnimation(0),
            mBadIdles()
            {};
    };
    
    AiWander::AiWander(int distance, int duration, int timeOfDay, const std::vector<unsigned char>& idle, bool repeat):
        mDistance(distance), mDuration(duration), mTimeOfDay(timeOfDay), mIdle(idle), mRepeat(repeat)
      , mStoredInitialActorPosition(false)
    {
        mIdle.resize(8, 0);
        init();
    }

    void AiWander::init()
    {
        // NOTE: mDistance and mDuration must be set already


        mStuckCount = 0;// TODO: maybe no longer needed
        mDoorCheckDuration = 0;
        mTrimCurrentNode = false;

        mHasReturnPosition = false;
        mReturnPosition = osg::Vec3f(0,0,0);

        if(mDistance < 0)
            mDistance = 0;
        if(mDuration < 0)
            mDuration = 0;
        if(mDuration == 0)
            mTimeOfDay = 0;

        mStartTime = MWBase::Environment::get().getWorld()->getTimeStamp();

        mPopulateAvailableNodes = true;

    }

    AiPackage * MWMechanics::AiWander::clone() const
    {
        return new AiWander(*this);
    }

    /*
     * AiWander high level states (0.29.0). Not entirely accurate in some cases
     * e.g. non-NPC actors do not greet and some creatures may be moving even in
     * the IdleNow state.
     *
     *                          [select node,
     *                           build path]
     *                 +---------->MoveNow----------->Walking
     *                 |                                 |
     * [allowed        |                                 |
     *  nodes]         |        [hello if near]          |
     *  start--->ChooseAction----->IdleNow               |
     *                ^ ^           |                    |
     *                | |           |                    |
     *                | +-----------+                    |
     *                |                                  |
     *                +----------------------------------+
     *
     *
     * New high level states.  Not exactly as per vanilla (e.g. door stuff)
     * but the differences are required because our physics does not work like
     * vanilla and therefore have to compensate/work around.
     *
     *                         [select node,     [if stuck evade
     *                          build path]       or remove nodes if near door]
     *                 +---------->MoveNow<---------->Walking
     *                 |              ^                | |
     *                 |              |(near door)     | |
     * [allowed        |              |                | |
     *  nodes]         |        [hello if near]        | |
     *  start--->ChooseAction----->IdleNow             | |
     *                ^ ^           |  ^               | |
     *                | |           |  | (stuck near   | |
     *                | +-----------+  +---------------+ |
     *                |                    player)       |
     *                +----------------------------------+
     *
     * NOTE: non-time critical operations are run once every 250ms or so.
     *
     * TODO: It would be great if door opening/closing can be detected and pathgrid
     * links dynamically updated.  Currently (0.29.0) AiWander allows choosing a
     * destination beyond closed doors which sometimes makes the actors stuck at the
     * door and impossible for the player to open the door.
     *
     * For now detect being stuck at the door and simply delete the nodes from the
     * allowed set.  The issue is when the door opens the allowed set is not
     * re-calculated.  However this would not be an issue in most cases since hostile
     * actors will enter combat (i.e. no longer wandering) and different pathfinding
     * will kick in.
     */
    bool AiWander::execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
    {
        // get or create temporary storage
        AiWanderStorage& storage = state.get<AiWanderStorage>();
        

        const MWWorld::CellStore*& currentCell = storage.mCell;
        MWMechanics::CreatureStats& cStats = actor.getClass().getCreatureStats(actor);
        if(cStats.isDead() || cStats.getHealth().getCurrent() <= 0)
            return true; // Don't bother with dead actors

        bool cellChange = currentCell && (actor.getCell() != currentCell);
        if(!currentCell || cellChange)
        {
            currentCell = actor.getCell();
            mPopulateAvailableNodes = true;
        }

        cStats.setDrawState(DrawState_Nothing);
        cStats.setMovementFlag(CreatureStats::Flag_Run, false);

        ESM::Position pos = actor.getRefData().getPosition();
        
        doPerFrameActionsForState(actor, duration, storage, pos);

        playIdleDialogueRandomly(actor);

        float& lastReaction = storage.mReaction;
        lastReaction += duration;
        if (REACTION_INTERVAL <= lastReaction)
        {
            lastReaction = 0;
            return reactionTimeActions(actor, storage, currentCell, cellChange, pos);
        }
        else
            return false;
    }

    bool AiWander::reactionTimeActions(const MWWorld::Ptr& actor, AiWanderStorage& storage,
        const MWWorld::CellStore*& currentCell, bool cellChange, ESM::Position& pos)
    {
        if (isPackageCompleted(actor, storage))
        {
            return true;
        }

        // Initialization to discover & store allowed node points for this actor.
        if (mPopulateAvailableNodes)
        {
            getAllowedNodes(actor, currentCell->getCell());
        }

        // Actor becomes stationary - see above URL's for previous research
        if(mAllowedNodes.empty())
            mDistance = 0;

        // Don't try to move if you are in a new cell (ie: positioncell command called) but still play idles.
        if(mDistance && cellChange)
            mDistance = 0;

        // For stationary NPCs, move back to the starting location if another AiPackage moved us elsewhere
        if (cellChange)
            mHasReturnPosition = false;
        if (mDistance == 0 && mHasReturnPosition 
            && (pos.asVec3() - mReturnPosition).length2() > (DESTINATION_TOLERANCE * DESTINATION_TOLERANCE))
        {
            returnToStartLocation(actor, storage, pos);
        }

        // Allow interrupting a walking actor to trigger a greeting
        WanderState& wanderState = storage.mState;
        if ((wanderState == Wander_IdleNow) || (wanderState == Wander_Walking))
        {
            playGreetingIfPlayerGetsTooClose(actor, storage);
        }

        if ((wanderState == Wander_MoveNow) && mDistance)
        {
            // Construct a new path if there isn't one
            if(!storage.mPathFinder.isPathConstructed())
            {
                if (mAllowedNodes.size())
                {
                    setPathToAnAllowedNode(actor, storage, pos);
                }
            } 
        }

        return false; // AiWander package not yet completed
    }

    bool AiWander::isPackageCompleted(const MWWorld::Ptr& actor, AiWanderStorage& storage)
    {
        if (mDuration)
        {
            // End package if duration is complete or mid-night hits:
            MWWorld::TimeStamp currentTime = MWBase::Environment::get().getWorld()->getTimeStamp();
            if ((currentTime.getHour() >= mStartTime.getHour() + mDuration) ||
                (int(currentTime.getHour()) == 0 && currentTime.getDay() != mStartTime.getDay()))
            {
                if (!mRepeat)
                {
                    stopWalking(actor, storage);
                    return true;
                }
                else
                {
                    mStartTime = currentTime;
                }
            }
        }
        // if get here, not yet completed
        return false;
    }

    void AiWander::returnToStartLocation(const MWWorld::Ptr& actor, AiWanderStorage& storage, ESM::Position& pos)
    {
        if (!storage.mPathFinder.isPathConstructed())
        {
            ESM::Pathgrid::Point dest(PathFinder::MakePathgridPoint(mReturnPosition));

            // actor position is already in world co-ordinates
            ESM::Pathgrid::Point start(PathFinder::MakePathgridPoint(pos));

            // don't take shortcuts for wandering
            storage.mPathFinder.buildSyncedPath(start, dest, actor.getCell(), false);

            if (storage.mPathFinder.isPathConstructed())
            {
                storage.mState = Wander_Walking;
            }
        }
    }

    void AiWander::doPerFrameActionsForState(const MWWorld::Ptr& actor, float duration, AiWanderStorage& storage, ESM::Position& pos)
    {
        switch (storage.mState)
        {
            case Wander_IdleNow:
                onIdleStatePerFrameActions(actor, duration, storage);
                break;

            case Wander_Walking:
                onWalkingStatePerFrameActions(actor, duration, storage, pos);
                break;

            case Wander_ChooseAction:
                onChooseActionStatePerFrameActions(actor, storage);
                break;

            case Wander_MoveNow:
                break;  // nothing to do

            default:
                // should never get here
                assert(false);
                break;
        }
    }

    void AiWander::onIdleStatePerFrameActions(const MWWorld::Ptr& actor, float duration, AiWanderStorage& storage)
    {
        // Check if an idle actor is  too close to a door - if so start walking
        mDoorCheckDuration += duration;
        if (mDoorCheckDuration >= DOOR_CHECK_INTERVAL)
        {
            mDoorCheckDuration = 0;    // restart timer
            if (mDistance &&            // actor is not intended to be stationary
                proximityToDoor(actor, MIN_DIST_TO_DOOR_SQUARED*1.6f*1.6f)) // NOTE: checks interior cells only
            {
                storage.mState = Wander_MoveNow;
                mTrimCurrentNode = false; // just in case
                return;
            }
        }

        bool& rotate = storage.mTurnActorGivingGreetingToFacePlayer;
        if (rotate)
        {
            // Reduce the turning animation glitch by using a *HUGE* value of
            // epsilon...  TODO: a proper fix might be in either the physics or the
            // animation subsystem
            if (zTurn(actor, storage.mTargetAngleRadians, osg::DegreesToRadians(5.f)))
                rotate = false;
        }

        // Check if idle animation finished
        GreetingState& greetingState = storage.mSaidGreeting;
        if (!checkIdle(actor, storage.mIdleAnimation) && (greetingState == Greet_Done || greetingState == Greet_None))
        {
            storage.mState = Wander_ChooseAction;
        }
    }

    void AiWander::onWalkingStatePerFrameActions(const MWWorld::Ptr& actor, 
        float duration, AiWanderStorage& storage, ESM::Position& pos)
    {
        // Are we there yet?
        if (storage.mPathFinder.checkPathCompleted(pos.pos[0], pos.pos[1], DESTINATION_TOLERANCE))
        {
            stopWalking(actor, storage);
            storage.mState = Wander_ChooseAction;
            mHasReturnPosition = false;
        }
        else
        {
            // have not yet reached the destination
            evadeObstacles(actor, storage, duration, pos);
        }
    }

    void AiWander::onChooseActionStatePerFrameActions(const MWWorld::Ptr& actor, AiWanderStorage& storage)
    {

        short unsigned& idleAnimation = storage.mIdleAnimation;
        idleAnimation = getRandomIdle();

        if (!idleAnimation && mDistance)
        {
            storage.mState = Wander_MoveNow;
            return;
        }
        if(idleAnimation)
        {
            if(std::find(storage.mBadIdles.begin(), storage.mBadIdles.end(), idleAnimation)==storage.mBadIdles.end())
            {
                if(!playIdle(actor, idleAnimation))
                {
                    storage.mBadIdles.push_back(idleAnimation);
                    storage.mState = Wander_ChooseAction;
                    return;
                }
            }
        }
        // Recreate vanilla (broken?) behavior of resetting start time of AIWander:
        mStartTime = MWBase::Environment::get().getWorld()->getTimeStamp();
        storage.mState = Wander_IdleNow;
    }

    void AiWander::evadeObstacles(const MWWorld::Ptr& actor, AiWanderStorage& storage, float duration, ESM::Position& pos)
    {
        // turn towards the next point in mPath
        zTurn(actor, storage.mPathFinder.getZAngleToNext(pos.pos[0], pos.pos[1]));

        MWMechanics::Movement& movement = actor.getClass().getMovementSettings(actor);
        if (mObstacleCheck.check(actor, duration))
        {
            // first check if we're walking into a door
            if (proximityToDoor(actor)) // NOTE: checks interior cells only
            {
                // remove allowed points then select another random destination
                mTrimCurrentNode = true;
                trimAllowedNodes(mAllowedNodes, storage.mPathFinder);
                mObstacleCheck.clear();
                storage.mPathFinder.clearPath();
                storage.mState = Wander_MoveNow;
            }
            else // probably walking into another NPC
            {
                // TODO: diagonal should have same animation as walk forward
                //       but doesn't seem to do that?
                mObstacleCheck.takeEvasiveAction(movement);
            }
            mStuckCount++;  // TODO: maybe no longer needed
        }
        else
        {
            movement.mPosition[1] = 1;
        }

        // if stuck for sufficiently long, act like current location was the destination
        if (mStuckCount >= COUNT_BEFORE_RESET) // something has gone wrong, reset
        {
            //std::cout << "Reset \""<< cls.getName(actor) << "\"" << std::endl;
            mObstacleCheck.clear();

            stopWalking(actor, storage);
            storage.mState = Wander_ChooseAction;
            mStuckCount = 0;
        }
    }

    void AiWander::playIdleDialogueRandomly(const MWWorld::Ptr& actor)
    {
        int hello = actor.getClass().getCreatureStats(actor).getAiSetting(CreatureStats::AI_Hello).getModified();
        if (hello > 0 && !MWBase::Environment::get().getWorld()->isSwimming(actor)
            && MWBase::Environment::get().getSoundManager()->sayDone(actor))
        {
            MWWorld::Ptr player = getPlayer();

            static float fVoiceIdleOdds = MWBase::Environment::get().getWorld()->getStore()
                .get<ESM::GameSetting>().find("fVoiceIdleOdds")->getFloat();

            float roll = Misc::Rng::rollProbability() * 10000.0f;

            // In vanilla MW the chance was FPS dependent, and did not allow proper changing of fVoiceIdleOdds
            // due to the roll being an integer.
            // Our implementation does not have these issues, so needs to be recalibrated. We chose to
            // use the chance MW would have when run at 60 FPS with the default value of the GMST for calibration.
            float x = fVoiceIdleOdds * 0.6f * (MWBase::Environment::get().getFrameDuration() / 0.1f);

            // Only say Idle voices when player is in LOS
            // A bit counterintuitive, likely vanilla did this to reduce the appearance of
            // voices going through walls?
            const ESM::Position& pos = actor.getRefData().getPosition();
            if (roll < x && (player.getRefData().getPosition().asVec3() - pos.asVec3()).length2()
                < 3000 * 3000 // maybe should be fAudioVoiceDefaultMaxDistance*fAudioMaxDistanceMult instead
                && MWBase::Environment::get().getWorld()->getLOS(player, actor))
                MWBase::Environment::get().getDialogueManager()->say(actor, "idle");
        }
    }

    void AiWander::playGreetingIfPlayerGetsTooClose(const MWWorld::Ptr& actor, AiWanderStorage& storage)
    {
        // Play a random voice greeting if the player gets too close
        int hello = actor.getClass().getCreatureStats(actor).getAiSetting(CreatureStats::AI_Hello).getModified();
        float helloDistance = static_cast<float>(hello);
        static int iGreetDistanceMultiplier = MWBase::Environment::get().getWorld()->getStore()
            .get<ESM::GameSetting>().find("iGreetDistanceMultiplier")->getInt();

        helloDistance *= iGreetDistanceMultiplier;

        MWWorld::Ptr player = getPlayer();
        osg::Vec3f playerPos(player.getRefData().getPosition().asVec3());
        osg::Vec3f actorPos(actor.getRefData().getPosition().asVec3());
        float playerDistSqr = (playerPos - actorPos).length2();

        int& greetingTimer = storage.mGreetingTimer;
        GreetingState& greetingState = storage.mSaidGreeting;
        if (greetingState == Greet_None)
        {
            if ((playerDistSqr <= helloDistance*helloDistance) &&
                !player.getClass().getCreatureStats(player).isDead() && MWBase::Environment::get().getWorld()->getLOS(player, actor)
                && MWBase::Environment::get().getMechanicsManager()->awarenessCheck(player, actor))
                greetingTimer++;

            if (greetingTimer >= GREETING_SHOULD_START)
            {
                greetingState = Greet_InProgress;
                MWBase::Environment::get().getDialogueManager()->say(actor, "hello");
                greetingTimer = 0;
            }
        }

        if (greetingState == Greet_InProgress)
        {
            greetingTimer++;

            if (storage.mState == Wander_Walking)
            {
                stopWalking(actor, storage);
                mObstacleCheck.clear();
                storage.mState = Wander_IdleNow;
            }

            turnActorToFacePlayer(actorPos, playerPos, storage);

            if (greetingTimer >= GREETING_SHOULD_END)
            {
                greetingState = Greet_Done;
                greetingTimer = 0;
            }
        }

        if (greetingState == MWMechanics::AiWander::Greet_Done)
        {
            float resetDist = 2 * helloDistance;
            if (playerDistSqr >= resetDist*resetDist)
                greetingState = Greet_None;
        }
    }

    void AiWander::turnActorToFacePlayer(const osg::Vec3f& actorPosition, const osg::Vec3f& playerPosition, AiWanderStorage& storage)
    {
        osg::Vec3f dir = playerPosition - actorPosition;

        float faceAngleRadians = std::atan2(dir.x(), dir.y());
        storage.mTargetAngleRadians = faceAngleRadians;
        storage.mTurnActorGivingGreetingToFacePlayer = true;
    }

    void AiWander::setPathToAnAllowedNode(const MWWorld::Ptr& actor, AiWanderStorage& storage, const ESM::Position& actorPos)
    {
        unsigned int randNode = Misc::Rng::rollDice(mAllowedNodes.size());
        ESM::Pathgrid::Point dest(mAllowedNodes[randNode]);
        ToWorldCoordinates(dest, storage.mCell->getCell());

        // actor position is already in world co-ordinates
        ESM::Pathgrid::Point start(PathFinder::MakePathgridPoint(actorPos));

        // don't take shortcuts for wandering
        storage.mPathFinder.buildSyncedPath(start, dest, actor.getCell(), false);

        if (storage.mPathFinder.isPathConstructed())
        {
            // Remove this node as an option and add back the previously used node (stops NPC from picking the same node):
            ESM::Pathgrid::Point temp = mAllowedNodes[randNode];
            mAllowedNodes.erase(mAllowedNodes.begin() + randNode);
            // check if mCurrentNode was taken out of mAllowedNodes
            if (mTrimCurrentNode && mAllowedNodes.size() > 1)
                mTrimCurrentNode = false;
            else
                mAllowedNodes.push_back(mCurrentNode);
            mCurrentNode = temp;

            storage.mState = Wander_Walking;
        }
        // Choose a different node and delete this one from possible nodes because it is uncreachable:
        else
            mAllowedNodes.erase(mAllowedNodes.begin() + randNode);
    }

    void AiWander::ToWorldCoordinates(ESM::Pathgrid::Point& point, const ESM::Cell * cell)
    {
        CoordinateConverter(cell).toWorld(point);
    }

    void AiWander::trimAllowedNodes(std::vector<ESM::Pathgrid::Point>& nodes,
                                    const PathFinder& pathfinder)
    {
        // TODO: how to add these back in once the door opens?
        // Idea: keep a list of detected closed doors (see aicombat.cpp)
        // Every now and then check whether one of the doors is opened. (maybe
        // at the end of playing idle?) If the door is opened then re-calculate
        // allowed nodes starting from the spawn point.
        std::list<ESM::Pathgrid::Point> paths = pathfinder.getPath();
        while(paths.size() >= 2)
        {
            ESM::Pathgrid::Point pt = paths.back();
            for(unsigned int j = 0; j < nodes.size(); j++)
            {
                // FIXME: doesn't hadle a door with the same X/Y
                //        co-ordinates but with a different Z
                if(nodes[j].mX == pt.mX && nodes[j].mY == pt.mY)
                {
                    nodes.erase(nodes.begin() + j);
                    break;
                }
            }
            paths.pop_back();
        }
    }

    int AiWander::getTypeId() const
    {
        return TypeIdWander;
    }

    void AiWander::stopWalking(const MWWorld::Ptr& actor, AiWanderStorage& storage)
    {
        storage.mPathFinder.clearPath();
        actor.getClass().getMovementSettings(actor).mPosition[1] = 0;
    }

    bool AiWander::playIdle(const MWWorld::Ptr& actor, unsigned short idleSelect)
    {
        if ((GroupIndex_MinIdle <= idleSelect) && (idleSelect <= GroupIndex_MaxIdle))
        {
            const std::string& groupName = sIdleSelectToGroupName[idleSelect - GroupIndex_MinIdle];
            return MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(actor, groupName, 0, 1);
        }
        else
        {
            std::cerr<< "Attempted to play out of range idle animation \""<<idleSelect<<"\" for " << actor.getCellRef().getRefId() << std::endl;
            return false;
        }
    }

    bool AiWander::checkIdle(const MWWorld::Ptr& actor, unsigned short idleSelect)
    {
        if ((GroupIndex_MinIdle <= idleSelect) && (idleSelect <= GroupIndex_MaxIdle))
        {
            const std::string& groupName = sIdleSelectToGroupName[idleSelect - GroupIndex_MinIdle];
            return MWBase::Environment::get().getMechanicsManager()->checkAnimationPlaying(actor, groupName);
        }
        else
        {
            return false;
        }
    }

    void AiWander::setReturnPosition(const osg::Vec3f& position)
    {
        if (!mHasReturnPosition)
        {
            mHasReturnPosition = true;
            mReturnPosition = position;
        }
    }

    short unsigned AiWander::getRandomIdle()
    {
        unsigned short idleRoll = 0;
        short unsigned selectedAnimation = 0;

        for(unsigned int counter = 0; counter < mIdle.size(); counter++)
        {
            static float fIdleChanceMultiplier = MWBase::Environment::get().getWorld()->getStore()
                .get<ESM::GameSetting>().find("fIdleChanceMultiplier")->getFloat();

            unsigned short idleChance = static_cast<unsigned short>(fIdleChanceMultiplier * mIdle[counter]);
            unsigned short randSelect = (int)(Misc::Rng::rollProbability() * int(100 / fIdleChanceMultiplier));
            if(randSelect < idleChance && randSelect > idleRoll)
            {
                selectedAnimation = counter + GroupIndex_MinIdle;
                idleRoll = randSelect;
            }
        }
        return selectedAnimation;
    }

    void AiWander::fastForward(const MWWorld::Ptr& actor, AiState &state)
    {
        if (mDistance == 0)
            return;

        if (mPopulateAvailableNodes)
            getAllowedNodes(actor, actor.getCell()->getCell());

        if (mAllowedNodes.empty())
            return;

        state.moveIn(new AiWanderStorage());

        int index = Misc::Rng::rollDice(mAllowedNodes.size());
        ESM::Pathgrid::Point dest = mAllowedNodes[index];

        dest.mX += OffsetToPreventOvercrowding();
        dest.mY += OffsetToPreventOvercrowding();
        ToWorldCoordinates(dest, actor.getCell()->getCell());

        MWBase::Environment::get().getWorld()->moveObject(actor, static_cast<float>(dest.mX), 
            static_cast<float>(dest.mY), static_cast<float>(dest.mZ));
        actor.getClass().adjustPosition(actor, false);

        // may have changed cell
        mPopulateAvailableNodes = true;
    }

    int AiWander::OffsetToPreventOvercrowding()
    {
        return static_cast<int>(DESTINATION_TOLERANCE * (Misc::Rng::rollProbability() * 2.0f - 1.0f));
    }

    void AiWander::getAllowedNodes(const MWWorld::Ptr& actor, const ESM::Cell* cell)
    {
        if (!mStoredInitialActorPosition)
        {
            mInitialActorPosition = actor.getRefData().getPosition().asVec3();
            mStoredInitialActorPosition = true;
        }

        // infrequently used, therefore no benefit in caching it as a member
        const ESM::Pathgrid *
            pathgrid = MWBase::Environment::get().getWorld()->getStore().get<ESM::Pathgrid>().search(*cell);

        // If there is no path this actor doesn't go anywhere. See:
        // https://forum.openmw.org/viewtopic.php?t=1556
        // http://www.fliggerty.com/phpBB3/viewtopic.php?f=30&t=5833
        // Note: In order to wander, need at least two points.
        if(!pathgrid || (pathgrid->mPoints.size() < 2))
            mDistance = 0;

        // A distance value passed into the constructor indicates how far the
        // actor can  wander from the spawn position.  AiWander assumes that
        // pathgrid points are available, and uses them to randomly select wander
        // destinations within the allowed set of pathgrid points (nodes).
        // ... pathgrids don't usually include water, so swimmers ignore them
        if (mDistance && !actor.getClass().isPureWaterCreature(actor))
        {
            // get NPC's position in local (i.e. cell) co-ordinates
            osg::Vec3f npcPos(mInitialActorPosition);
            CoordinateConverter(cell).toLocal(npcPos);

            // mAllowedNodes for this actor with pathgrid point indexes based on mDistance
            // NOTE: mPoints and mAllowedNodes are in local co-ordinates
            int pointIndex = 0;
            for(unsigned int counter = 0; counter < pathgrid->mPoints.size(); counter++)
            {
                osg::Vec3f nodePos(PathFinder::MakeOsgVec3(pathgrid->mPoints[counter]));
                if((npcPos - nodePos).length2() <= mDistance * mDistance)
                {
                    mAllowedNodes.push_back(pathgrid->mPoints[counter]);
                    pointIndex = counter;
                }
            }
            if (mAllowedNodes.size() == 1)
            {
                AddNonPathGridAllowedPoints(npcPos, pathgrid, pointIndex);
            }
            if(!mAllowedNodes.empty())
            {
                SetCurrentNodeToClosestAllowedNode(npcPos);
            }
        }

        mPopulateAvailableNodes = false;
    }

    // When only one path grid point in wander distance, 
    // additional points for NPC to wander to are:
    // 1. NPC's initial location
    // 2. Partway along the path between the point and its connected points.
    void AiWander::AddNonPathGridAllowedPoints(osg::Vec3f npcPos, const ESM::Pathgrid * pathGrid, int pointIndex)
    {
        mAllowedNodes.push_back(PathFinder::MakePathgridPoint(npcPos));
        for (std::vector<ESM::Pathgrid::Edge>::const_iterator it = pathGrid->mEdges.begin(); it != pathGrid->mEdges.end(); ++it)
        {
            if (it->mV0 == pointIndex)
            {
                AddPointBetweenPathGridPoints(pathGrid->mPoints[it->mV0], pathGrid->mPoints[it->mV1]);
            }
        }
    }

    void AiWander::AddPointBetweenPathGridPoints(const ESM::Pathgrid::Point& start, const ESM::Pathgrid::Point& end)
    {
        osg::Vec3f vectorStart = PathFinder::MakeOsgVec3(start);
        osg::Vec3f delta = PathFinder::MakeOsgVec3(end) - vectorStart;
        float length = delta.length();
        delta.normalize();

        int distance = std::max(mDistance / 2, MINIMUM_WANDER_DISTANCE);
        
        // must not travel longer than distance between waypoints or NPC goes past waypoint
        distance = std::min(distance, static_cast<int>(length));
        delta *= distance;
        mAllowedNodes.push_back(PathFinder::MakePathgridPoint(vectorStart + delta));
    }

    void AiWander::SetCurrentNodeToClosestAllowedNode(osg::Vec3f npcPos)
    {
        float distanceToClosestNode = std::numeric_limits<float>::max();
        unsigned int index = 0;
        for (unsigned int counterThree = 0; counterThree < mAllowedNodes.size(); counterThree++)
        {
            osg::Vec3f nodePos(PathFinder::MakeOsgVec3(mAllowedNodes[counterThree]));
            float tempDist = (npcPos - nodePos).length2();
            if (tempDist < distanceToClosestNode)
            {
                index = counterThree;
                distanceToClosestNode = tempDist;
            }
        }
        mCurrentNode = mAllowedNodes[index];
        mAllowedNodes.erase(mAllowedNodes.begin() + index);
    }

    void AiWander::writeState(ESM::AiSequence::AiSequence &sequence) const
    {
        std::auto_ptr<ESM::AiSequence::AiWander> wander(new ESM::AiSequence::AiWander());
        wander->mData.mDistance = mDistance;
        wander->mData.mDuration = mDuration;
        wander->mData.mTimeOfDay = mTimeOfDay;
        wander->mStartTime = mStartTime.toEsm();
        assert (mIdle.size() == 8);
        for (int i=0; i<8; ++i)
            wander->mData.mIdle[i] = mIdle[i];
        wander->mData.mShouldRepeat = mRepeat;
        wander->mStoredInitialActorPosition = mStoredInitialActorPosition;
        if (mStoredInitialActorPosition)
            wander->mInitialActorPosition = mInitialActorPosition;

        ESM::AiSequence::AiPackageContainer package;
        package.mType = ESM::AiSequence::Ai_Wander;
        package.mPackage = wander.release();
        sequence.mPackages.push_back(package);
    }

    AiWander::AiWander (const ESM::AiSequence::AiWander* wander)
        : mDistance(wander->mData.mDistance)
        , mDuration(wander->mData.mDuration)
        , mTimeOfDay(wander->mData.mTimeOfDay)
        , mRepeat(wander->mData.mShouldRepeat != 0)
        , mStoredInitialActorPosition(wander->mStoredInitialActorPosition)
        , mStartTime(MWWorld::TimeStamp(wander->mStartTime))
    {
        if (mStoredInitialActorPosition)
            mInitialActorPosition = wander->mInitialActorPosition;
        for (int i=0; i<8; ++i)
            mIdle.push_back(wander->mData.mIdle[i]);

        init();
    }
}

