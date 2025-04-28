#include "aiwander.hpp"

#include <algorithm>

#include <osg/Matrixf>

#include <components/debug/debuglog.hpp>
#include <components/detournavigator/navigatorutils.hpp>
#include <components/esm3/aisequence.hpp>
#include <components/misc/coordinateconverter.hpp>
#include <components/misc/rng.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/datetimemanager.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwphysics/raycasting.hpp"

#include "actorutil.hpp"
#include "character.hpp"
#include "creaturestats.hpp"
#include "movement.hpp"
#include "pathgrid.hpp"

namespace MWMechanics
{
    static const int COUNT_BEFORE_RESET = 10;
    static const float IDLE_POSITION_CHECK_INTERVAL = 1.5f;

    // to prevent overcrowding
    static const int DESTINATION_TOLERANCE = 64;

    // distance must be long enough that NPC will need to move to get there.
    static const int MINIMUM_WANDER_DISTANCE = DESTINATION_TOLERANCE * 2;

    static const std::size_t MAX_IDLE_SIZE = 8;

    const std::string_view AiWander::sIdleSelectToGroupName[GroupIndex_MaxIdle - GroupIndex_MinIdle + 1] = {
        "idle2",
        "idle3",
        "idle4",
        "idle5",
        "idle6",
        "idle7",
        "idle8",
        "idle9",
    };

    namespace
    {
        inline int getCountBeforeReset(const MWWorld::ConstPtr& actor)
        {
            if (actor.getClass().isPureWaterCreature(actor) || actor.getClass().isPureFlyingCreature(actor))
                return 1;
            return COUNT_BEFORE_RESET;
        }

        osg::Vec3f getRandomPointAround(const osg::Vec3f& position, const float distance)
        {
            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
            const float randomDirection = Misc::Rng::rollClosedProbability(prng) * 2.0f * osg::PI;
            osg::Matrixf rotation;
            rotation.makeRotate(randomDirection, osg::Vec3f(0.0, 0.0, 1.0));
            return position + osg::Vec3f(distance, 0.0, 0.0) * rotation;
        }

        bool isDestinationHidden(const MWWorld::ConstPtr& actor, const osg::Vec3f& destination)
        {
            const auto position = actor.getRefData().getPosition().asVec3();
            const bool isWaterCreature = actor.getClass().isPureWaterCreature(actor);
            const bool isFlyingCreature = actor.getClass().isPureFlyingCreature(actor);
            const osg::Vec3f halfExtents
                = MWBase::Environment::get().getWorld()->getPathfindingAgentBounds(actor).mHalfExtents;
            osg::Vec3f direction = destination - position;
            direction.normalize();
            const auto visibleDestination
                = (isWaterCreature || isFlyingCreature ? destination : destination + osg::Vec3f(0, 0, halfExtents.z()))
                + direction * std::max(halfExtents.x(), std::max(halfExtents.y(), halfExtents.z()));
            const int mask = MWPhysics::CollisionType_World | MWPhysics::CollisionType_HeightMap
                | MWPhysics::CollisionType_Door | MWPhysics::CollisionType_Actor;
            return MWBase::Environment::get()
                .getWorld()
                ->getRayCasting()
                ->castRay(position, visibleDestination, { actor }, {}, mask)
                .mHit;
        }

        void stopMovement(const MWWorld::Ptr& actor)
        {
            auto& movementSettings = actor.getClass().getMovementSettings(actor);
            movementSettings.mPosition[0] = 0;
            movementSettings.mPosition[1] = 0;
        }

        std::vector<unsigned char> getInitialIdle(const std::vector<unsigned char>& idle)
        {
            std::vector<unsigned char> result(MAX_IDLE_SIZE, 0);
            std::copy_n(idle.begin(), std::min(MAX_IDLE_SIZE, idle.size()), result.begin());
            return result;
        }

        std::vector<unsigned char> getInitialIdle(const unsigned char (&idle)[MAX_IDLE_SIZE])
        {
            return std::vector<unsigned char>(std::begin(idle), std::end(idle));
        }

    }

    AiWanderStorage::AiWanderStorage()
        : mReaction(MWBase::Environment::get().getWorld()->getPrng())
        , mState(Wander_ChooseAction)
        , mIsWanderingManually(false)
        , mCanWanderAlongPathGrid(true)
        , mIdleAnimation(0)
        , mBadIdles()
        , mPopulateAvailableNodes(true)
        , mAllowedNodes()
        , mTrimCurrentNode(false)
        , mCheckIdlePositionTimer(0)
        , mStuckCount(0)
    {
    }

    AiWander::AiWander(int distance, int duration, int timeOfDay, const std::vector<unsigned char>& idle, bool repeat)
        : TypedAiPackage<AiWander>(repeat)
        , mDistance(std::max(0, distance))
        , mDuration(std::max(0, duration))
        , mRemainingDuration(duration)
        , mTimeOfDay(timeOfDay)
        , mIdle(getInitialIdle(idle))
        , mStoredInitialActorPosition(false)
        , mInitialActorPosition(osg::Vec3f(0, 0, 0))
        , mHasDestination(false)
        , mDestination(osg::Vec3f(0, 0, 0))
        , mUsePathgrid(false)
    {
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
    bool AiWander::execute(
        const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
    {
        MWMechanics::CreatureStats& cStats = actor.getClass().getCreatureStats(actor);
        if (cStats.isDead() || cStats.getHealth().getCurrent() <= 0)
            return true; // Don't bother with dead actors

        // get or create temporary storage
        AiWanderStorage& storage = state.get<AiWanderStorage>();

        mRemainingDuration
            -= ((duration * MWBase::Environment::get().getWorld()->getTimeManager()->getGameTimeScale()) / 3600);

        cStats.setDrawState(DrawState::Nothing);
        cStats.setMovementFlag(CreatureStats::Flag_Run, false);

        ESM::Position pos = actor.getRefData().getPosition();

        // If there is already a destination due to the package having been interrupted by a combat or pursue package,
        // rebuild a path to it
        if (!mPathFinder.isPathConstructed() && mHasDestination)
        {
            const ESM::Pathgrid* pathgrid
                = MWBase::Environment::get().getESMStore()->get<ESM::Pathgrid>().search(*actor.getCell()->getCell());
            if (mUsePathgrid)
            {
                mPathFinder.buildPathByPathgrid(
                    pos.asVec3(), mDestination, actor.getCell(), getPathGridGraph(pathgrid));
            }
            else
            {
                const auto agentBounds = MWBase::Environment::get().getWorld()->getPathfindingAgentBounds(actor);
                constexpr float endTolerance = 0;
                const DetourNavigator::Flags navigatorFlags = getNavigatorFlags(actor);
                const DetourNavigator::AreaCosts areaCosts = getAreaCosts(actor, navigatorFlags);
                mPathFinder.buildPath(actor, pos.asVec3(), mDestination, actor.getCell(), getPathGridGraph(pathgrid),
                    agentBounds, navigatorFlags, areaCosts, endTolerance, PathType::Full);
            }

            if (mPathFinder.isPathConstructed())
                storage.setState(AiWanderStorage::Wander_Walking, !mUsePathgrid);
        }

        if (!cStats.getMovementFlag(CreatureStats::Flag_ForceJump)
            && !cStats.getMovementFlag(CreatureStats::Flag_ForceSneak))
        {
            GreetingState greetingState = MWBase::Environment::get().getMechanicsManager()->getGreetingState(actor);
            if (greetingState == Greet_InProgress)
            {
                if (storage.mState == AiWanderStorage::Wander_Walking)
                {
                    stopMovement(actor);
                    mObstacleCheck.clear();
                    storage.setState(AiWanderStorage::Wander_IdleNow);
                }
            }
        }

        doPerFrameActionsForState(actor, duration, characterController.getSupportedMovementDirections(), storage);

        if (storage.mReaction.update(duration) == Misc::TimerStatus::Waiting)
            return false;

        return reactionTimeActions(actor, storage, pos);
    }

    bool AiWander::reactionTimeActions(const MWWorld::Ptr& actor, AiWanderStorage& storage, ESM::Position& pos)
    {
        if (mDistance <= 0)
            storage.mCanWanderAlongPathGrid = false;

        if (isPackageCompleted())
        {
            stopWalking(actor);
            // Reset package so it can be used again
            mRemainingDuration = mDuration;
            return true;
        }

        if (!mStoredInitialActorPosition)
        {
            mInitialActorPosition = actor.getRefData().getPosition().asVec3();
            mStoredInitialActorPosition = true;
        }

        // Initialization to discover & store allowed node points for this actor.
        if (storage.mPopulateAvailableNodes)
        {
            getAllowedNodes(actor, storage);
        }

        auto& prng = MWBase::Environment::get().getWorld()->getPrng();
        if (canActorMoveByZAxis(actor) && mDistance > 0)
        {
            // Typically want to idle for a short time before the next wander
            if (Misc::Rng::rollDice(100, prng) >= 92 && storage.mState != AiWanderStorage::Wander_Walking)
            {
                wanderNearStart(actor, storage, mDistance);
            }

            storage.mCanWanderAlongPathGrid = false;
        }
        // If the package has a wander distance but no pathgrid is available,
        // randomly idle or wander near spawn point
        else if (storage.mAllowedNodes.empty() && mDistance > 0 && !storage.mIsWanderingManually)
        {
            // Typically want to idle for a short time before the next wander
            if (Misc::Rng::rollDice(100, prng) >= 96)
            {
                wanderNearStart(actor, storage, mDistance);
            }
            else
            {
                storage.setState(AiWanderStorage::Wander_IdleNow);
            }
        }
        else if (storage.mAllowedNodes.empty() && !storage.mIsWanderingManually)
        {
            storage.mCanWanderAlongPathGrid = false;
        }

        // If Wandering manually and hit an obstacle, stop
        if (storage.mIsWanderingManually && mObstacleCheck.isEvading())
        {
            completeManualWalking(actor, storage);
        }

        if (storage.mState == AiWanderStorage::Wander_MoveNow && storage.mCanWanderAlongPathGrid)
        {
            // Construct a new path if there isn't one
            if (!mPathFinder.isPathConstructed())
            {
                if (!storage.mAllowedNodes.empty())
                {
                    setPathToAnAllowedNode(actor, storage, pos);
                }
            }
        }
        else if (storage.mIsWanderingManually && mPathFinder.checkPathCompleted())
        {
            completeManualWalking(actor, storage);
        }

        if (storage.mIsWanderingManually && storage.mState == AiWanderStorage::Wander_Walking
            && (mPathFinder.getPathSize() == 0 || isDestinationHidden(actor, mPathFinder.getPath().back())
                || isAreaOccupiedByOtherActor(actor, mPathFinder.getPath().back())))
            completeManualWalking(actor, storage);

        return false; // AiWander package not yet completed
    }

    osg::Vec3f AiWander::getDestination(const MWWorld::Ptr& actor) const
    {
        if (mHasDestination)
            return mDestination;

        return actor.getRefData().getPosition().asVec3();
    }

    bool AiWander::isPackageCompleted() const
    {
        // End package if duration is complete
        return mDuration && mRemainingDuration <= 0;
    }

    /*
     * Commands actor to walk to a random location near original spawn location.
     */
    void AiWander::wanderNearStart(const MWWorld::Ptr& actor, AiWanderStorage& storage, int wanderDistance)
    {
        const auto currentPosition = actor.getRefData().getPosition().asVec3();

        std::size_t attempts = 10; // If a unit can't wander out of water, don't want to hang here
        const bool isWaterCreature = actor.getClass().isPureWaterCreature(actor);
        const bool isFlyingCreature = actor.getClass().isPureFlyingCreature(actor);
        const auto world = MWBase::Environment::get().getWorld();
        const auto agentBounds = world->getPathfindingAgentBounds(actor);
        const auto navigator = world->getNavigator();
        const DetourNavigator::Flags navigatorFlags = getNavigatorFlags(actor);
        const DetourNavigator::AreaCosts areaCosts = getAreaCosts(actor, navigatorFlags);
        auto& prng = MWBase::Environment::get().getWorld()->getPrng();

        do
        {

            // Determine a random location within radius of original position
            const float wanderRadius = (0.2f + Misc::Rng::rollClosedProbability(prng) * 0.8f) * wanderDistance;
            if (!isWaterCreature && !isFlyingCreature)
            {
                // findRandomPointAroundCircle uses wanderDistance as limit for random and not as exact distance
                const auto getRandom
                    = []() { return Misc::Rng::rollProbability(MWBase::Environment::get().getWorld()->getPrng()); };
                auto destination = DetourNavigator::findRandomPointAroundCircle(
                    *navigator, agentBounds, mInitialActorPosition, wanderRadius, navigatorFlags, getRandom);
                if (destination.has_value())
                {
                    osg::Vec3f direction = *destination - mInitialActorPosition;
                    if (direction.length() > wanderDistance)
                    {
                        direction.normalize();
                        const osg::Vec3f adjustedDestination = mInitialActorPosition + direction * wanderRadius;
                        destination = DetourNavigator::raycast(
                            *navigator, agentBounds, currentPosition, adjustedDestination, navigatorFlags);
                        if (destination.has_value() && (*destination - mInitialActorPosition).length() > wanderDistance)
                            continue;
                    }
                }
                mDestination = destination.has_value() ? *destination
                                                       : getRandomPointAround(mInitialActorPosition, wanderRadius);
            }
            else
                mDestination = getRandomPointAround(mInitialActorPosition, wanderRadius);

            // Check if land creature will walk onto water or if water creature will swim onto land
            if (!isWaterCreature && destinationIsAtWater(actor, mDestination))
                continue;

            if (isDestinationHidden(actor, mDestination))
                continue;

            if (isAreaOccupiedByOtherActor(actor, mDestination))
                continue;

            constexpr float endTolerance = 0;

            if (isWaterCreature || isFlyingCreature)
                mPathFinder.buildStraightPath(mDestination);
            else
                mPathFinder.buildPathByNavMesh(actor, currentPosition, mDestination, agentBounds, navigatorFlags,
                    areaCosts, endTolerance, PathType::Full);

            if (mPathFinder.isPathConstructed())
            {
                storage.setState(AiWanderStorage::Wander_Walking, true);
                mHasDestination = true;
                mUsePathgrid = false;
            }

            break;
        } while (--attempts);
    }

    /*
     * Returns true if the position provided is above water.
     */
    bool AiWander::destinationIsAtWater(const MWWorld::Ptr& actor, const osg::Vec3f& destination)
    {
        float heightToGroundOrWater = MWBase::Environment::get().getWorld()->getDistToNearestRayHit(
            destination, osg::Vec3f(0, 0, -1), 1000.0, true);
        osg::Vec3f positionBelowSurface = destination;
        positionBelowSurface[2] = positionBelowSurface[2] - heightToGroundOrWater - 1.0f;
        return MWBase::Environment::get().getWorld()->isUnderwater(actor.getCell(), positionBelowSurface);
    }

    void AiWander::completeManualWalking(const MWWorld::Ptr& actor, AiWanderStorage& storage)
    {
        stopWalking(actor);
        mObstacleCheck.clear();
        storage.setState(AiWanderStorage::Wander_IdleNow);
    }

    void AiWander::doPerFrameActionsForState(const MWWorld::Ptr& actor, float duration,
        MWWorld::MovementDirectionFlags supportedMovementDirections, AiWanderStorage& storage)
    {
        // Attempt to fast forward to the next state instead of remaining in an intermediate state for a frame
        for (int i = 0; i < 2; ++i)
        {
            switch (storage.mState)
            {
                case AiWanderStorage::Wander_IdleNow:
                {
                    onIdleStatePerFrameActions(actor, duration, storage);
                    if (storage.mState != AiWanderStorage::Wander_ChooseAction)
                        return;
                    continue;
                }
                case AiWanderStorage::Wander_Walking:
                    onWalkingStatePerFrameActions(actor, duration, supportedMovementDirections, storage);
                    return;

                case AiWanderStorage::Wander_ChooseAction:
                {
                    onChooseActionStatePerFrameActions(actor, storage);
                    if (storage.mState != AiWanderStorage::Wander_IdleNow)
                        return;
                    continue;
                }
                case AiWanderStorage::Wander_MoveNow:
                    return; // nothing to do

                default:
                    // should never get here
                    assert(false);
                    return;
            }
        }
    }

    void AiWander::onIdleStatePerFrameActions(const MWWorld::Ptr& actor, float duration, AiWanderStorage& storage)
    {
        // Check if an idle actor is too far from all allowed nodes or too close to a door - if so start walking.
        storage.mCheckIdlePositionTimer += duration;

        if (storage.mCheckIdlePositionTimer >= IDLE_POSITION_CHECK_INTERVAL && !isStationary())
        {
            storage.mCheckIdlePositionTimer = 0; // restart timer
            static float distance = MWBase::Environment::get().getWorld()->getMaxActivationDistance() * 1.6f;
            if (proximityToDoor(actor, distance) || !isNearAllowedNode(actor, storage, distance))
            {
                storage.setState(AiWanderStorage::Wander_MoveNow);
                storage.mTrimCurrentNode = false; // just in case
                return;
            }
        }

        // Check if idle animation finished
        GreetingState greetingState = MWBase::Environment::get().getMechanicsManager()->getGreetingState(actor);
        if (!checkIdle(actor, storage.mIdleAnimation) && (greetingState == Greet_Done || greetingState == Greet_None))
        {
            if (mPathFinder.isPathConstructed())
                storage.setState(AiWanderStorage::Wander_Walking, !mUsePathgrid);
            else
                storage.setState(AiWanderStorage::Wander_ChooseAction);
        }
    }

    bool AiWander::isNearAllowedNode(const MWWorld::Ptr& actor, const AiWanderStorage& storage, float distance) const
    {
        const osg::Vec3f actorPos = actor.getRefData().getPosition().asVec3();
        for (const ESM::Pathgrid::Point& node : storage.mAllowedNodes)
        {
            osg::Vec3f point(node.mX, node.mY, node.mZ);
            if ((actorPos - point).length2() < distance * distance)
                return true;
        }
        return false;
    }

    void AiWander::onWalkingStatePerFrameActions(const MWWorld::Ptr& actor, float duration,
        MWWorld::MovementDirectionFlags supportedMovementDirections, AiWanderStorage& storage)
    {
        // Is there no destination or are we there yet?
        if ((!mPathFinder.isPathConstructed())
            || pathTo(actor, osg::Vec3f(mPathFinder.getPath().back()), duration, supportedMovementDirections,
                DESTINATION_TOLERANCE))
        {
            stopWalking(actor);
            storage.setState(AiWanderStorage::Wander_ChooseAction);
        }
        else
        {
            // have not yet reached the destination
            evadeObstacles(actor, storage);
        }
    }

    void AiWander::onChooseActionStatePerFrameActions(const MWWorld::Ptr& actor, AiWanderStorage& storage)
    {
        // Wait while fully stop before starting idle animation (important if "smooth movement" is enabled).
        if (actor.getClass().getCurrentSpeed(actor) > 0)
            return;

        unsigned short idleAnimation = getRandomIdle();
        storage.mIdleAnimation = idleAnimation;

        if (!idleAnimation && mDistance)
        {
            storage.setState(AiWanderStorage::Wander_MoveNow);
            return;
        }
        if (idleAnimation)
        {
            if (std::find(storage.mBadIdles.begin(), storage.mBadIdles.end(), idleAnimation) == storage.mBadIdles.end())
            {
                if (!playIdle(actor, idleAnimation))
                {
                    storage.mBadIdles.push_back(idleAnimation);
                    storage.setState(AiWanderStorage::Wander_ChooseAction);
                    return;
                }
            }
        }

        storage.setState(AiWanderStorage::Wander_IdleNow);
    }

    void AiWander::evadeObstacles(const MWWorld::Ptr& actor, AiWanderStorage& storage)
    {
        if (mObstacleCheck.isEvading())
        {
            // first check if we're walking into a door
            static float distance = MWBase::Environment::get().getWorld()->getMaxActivationDistance();
            if (proximityToDoor(actor, distance))
            {
                // remove allowed points then select another random destination
                storage.mTrimCurrentNode = true;
                trimAllowedNodes(storage.mAllowedNodes, mPathFinder);
                mObstacleCheck.clear();
                stopWalking(actor);
                storage.setState(AiWanderStorage::Wander_MoveNow);
            }

            storage.mStuckCount++; // TODO: maybe no longer needed
        }

        // if stuck for sufficiently long, act like current location was the destination
        if (storage.mStuckCount >= getCountBeforeReset(actor)) // something has gone wrong, reset
        {
            mObstacleCheck.clear();
            stopWalking(actor);
            storage.setState(AiWanderStorage::Wander_ChooseAction);
            storage.mStuckCount = 0;
        }
    }

    void AiWander::setPathToAnAllowedNode(
        const MWWorld::Ptr& actor, AiWanderStorage& storage, const ESM::Position& actorPos)
    {
        auto world = MWBase::Environment::get().getWorld();
        auto& prng = world->getPrng();
        unsigned int randNode = Misc::Rng::rollDice(storage.mAllowedNodes.size(), prng);
        const ESM::Pathgrid::Point& dest = storage.mAllowedNodes[randNode];

        const osg::Vec3f start = actorPos.asVec3();

        // don't take shortcuts for wandering
        const ESM::Pathgrid* pathgrid = world->getStore().get<ESM::Pathgrid>().search(*actor.getCell()->getCell());
        const osg::Vec3f destVec3f = PathFinder::makeOsgVec3(dest);
        mPathFinder.buildPathByPathgrid(start, destVec3f, actor.getCell(), getPathGridGraph(pathgrid));

        if (mPathFinder.isPathConstructed())
        {
            mDestination = destVec3f;
            mHasDestination = true;
            mUsePathgrid = true;
            // Remove this node as an option and add back the previously used node (stops NPC from picking the same
            // node):
            ESM::Pathgrid::Point temp = storage.mAllowedNodes[randNode];
            storage.mAllowedNodes.erase(storage.mAllowedNodes.begin() + randNode);
            // check if mCurrentNode was taken out of mAllowedNodes
            if (storage.mTrimCurrentNode && storage.mAllowedNodes.size() > 1)
                storage.mTrimCurrentNode = false;
            else
                storage.mAllowedNodes.push_back(storage.mCurrentNode);
            storage.mCurrentNode = temp;

            storage.setState(AiWanderStorage::Wander_Walking);
        }
        // Choose a different node and delete this one from possible nodes because it is uncreachable:
        else
            storage.mAllowedNodes.erase(storage.mAllowedNodes.begin() + randNode);
    }

    void AiWander::trimAllowedNodes(std::vector<ESM::Pathgrid::Point>& nodes, const PathFinder& pathfinder)
    {
        // TODO: how to add these back in once the door opens?
        // Idea: keep a list of detected closed doors (see aicombat.cpp)
        // Every now and then check whether one of the doors is opened. (maybe
        // at the end of playing idle?) If the door is opened then re-calculate
        // allowed nodes starting from the spawn point.
        auto paths = pathfinder.getPath();
        while (paths.size() >= 2)
        {
            const auto pt = paths.back();
            for (unsigned int j = 0; j < nodes.size(); j++)
            {
                // FIXME: doesn't handle a door with the same X/Y
                //        coordinates but with a different Z
                if (std::abs(nodes[j].mX - pt.x()) <= 0.5 && std::abs(nodes[j].mY - pt.y()) <= 0.5)
                {
                    nodes.erase(nodes.begin() + j);
                    break;
                }
            }
            paths.pop_back();
        }
    }

    void AiWander::stopWalking(const MWWorld::Ptr& actor)
    {
        mPathFinder.clearPath();
        mHasDestination = false;
        stopMovement(actor);
    }

    bool AiWander::playIdle(const MWWorld::Ptr& actor, unsigned short idleSelect)
    {
        if ((GroupIndex_MinIdle <= idleSelect) && (idleSelect <= GroupIndex_MaxIdle))
        {
            const std::string_view groupName = sIdleSelectToGroupName[idleSelect - GroupIndex_MinIdle];
            return MWBase::Environment::get().getMechanicsManager()->playAnimationGroup(actor, groupName, 0, 1);
        }
        else
        {
            Log(Debug::Verbose) << "Attempted to play out of range idle animation \"" << idleSelect << "\" for "
                                << actor.getCellRef().getRefId();
            return false;
        }
    }

    bool AiWander::checkIdle(const MWWorld::Ptr& actor, unsigned short idleSelect)
    {
        if ((GroupIndex_MinIdle <= idleSelect) && (idleSelect <= GroupIndex_MaxIdle))
        {
            const std::string_view groupName = sIdleSelectToGroupName[idleSelect - GroupIndex_MinIdle];
            return MWBase::Environment::get().getMechanicsManager()->checkAnimationPlaying(actor, groupName);
        }
        else
        {
            return false;
        }
    }

    int AiWander::getRandomIdle() const
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        static const float fIdleChanceMultiplier
            = world->getStore().get<ESM::GameSetting>().find("fIdleChanceMultiplier")->mValue.getFloat();
        if (Misc::Rng::rollClosedProbability(world->getPrng()) > fIdleChanceMultiplier)
            return 0;

        int newIdle = 0;
        float maxRoll = 0.f;
        for (size_t i = 0; i < mIdle.size(); i++)
        {
            float roll = Misc::Rng::rollClosedProbability(world->getPrng()) * 100.f;
            if (roll <= mIdle[i] && roll > maxRoll)
            {
                newIdle = GroupIndex_MinIdle + i;
                maxRoll = roll;
            }
        }

        return newIdle;
    }

    void AiWander::fastForward(const MWWorld::Ptr& actor, AiState& state)
    {
        // Update duration counter
        mRemainingDuration--;
        if (mDistance == 0)
            return;

        AiWanderStorage& storage = state.get<AiWanderStorage>();
        if (storage.mPopulateAvailableNodes)
            getAllowedNodes(actor, storage);

        if (storage.mAllowedNodes.empty())
            return;

        auto& prng = MWBase::Environment::get().getWorld()->getPrng();
        int index = Misc::Rng::rollDice(storage.mAllowedNodes.size(), prng);
        ESM::Pathgrid::Point worldDest = storage.mAllowedNodes[index];
        const Misc::CoordinateConverter converter = Misc::makeCoordinateConverter(*actor.getCell()->getCell());
        ESM::Pathgrid::Point dest = converter.toLocalPoint(worldDest);

        bool isPathGridOccupied = MWBase::Environment::get().getMechanicsManager()->isAnyActorInRange(
            PathFinder::makeOsgVec3(worldDest), 60);

        // add offset only if the selected pathgrid is occupied by another actor
        if (isPathGridOccupied)
        {
            ESM::Pathgrid::PointList points;
            getNeighbouringNodes(dest, actor.getCell(), points);

            // there are no neighbouring nodes, nowhere to move
            if (points.empty())
                return;

            bool isOccupied = false;
            // AI will try to move the NPC towards every neighboring node until suitable place will be found
            while (!points.empty())
            {
                int randomIndex = Misc::Rng::rollDice(points.size(), prng);
                const ESM::Pathgrid::Point& connDest = points[randomIndex];

                // add an offset towards random neighboring node
                osg::Vec3f dir = PathFinder::makeOsgVec3(connDest) - PathFinder::makeOsgVec3(dest);
                float length = dir.length();
                dir.normalize();

                for (int j = 1; j <= 3; j++)
                {
                    // move for 5-15% towards random neighboring node
                    dest
                        = PathFinder::makePathgridPoint(PathFinder::makeOsgVec3(dest) + dir * (j * 5 * length / 100.f));
                    worldDest = converter.toWorldPoint(dest);

                    isOccupied = MWBase::Environment::get().getMechanicsManager()->isAnyActorInRange(
                        PathFinder::makeOsgVec3(worldDest), 60);

                    if (!isOccupied)
                        break;
                }

                if (!isOccupied)
                    break;

                // Will try an another neighboring node
                points.erase(points.begin() + randomIndex);
            }

            // there is no free space, nowhere to move
            if (isOccupied)
                return;
        }

        // place above to prevent moving inside objects, e.g. stairs, because a vector between pathgrids can be
        // underground. Adding 20 in adjustPosition() is not enough.
        dest.mZ += 60;

        converter.toWorld(dest);

        state.moveIn(std::make_unique<AiWanderStorage>());

        osg::Vec3f pos(static_cast<float>(dest.mX), static_cast<float>(dest.mY), static_cast<float>(dest.mZ));
        MWBase::Environment::get().getWorld()->moveObject(actor, pos);
        actor.getClass().adjustPosition(actor, false);
    }

    void AiWander::getNeighbouringNodes(
        ESM::Pathgrid::Point dest, const MWWorld::CellStore* currentCell, ESM::Pathgrid::PointList& points)
    {
        const ESM::Pathgrid* pathgrid
            = MWBase::Environment::get().getESMStore()->get<ESM::Pathgrid>().search(*currentCell->getCell());

        if (pathgrid == nullptr || pathgrid->mPoints.empty())
            return;

        int index = PathFinder::getClosestPoint(pathgrid, PathFinder::makeOsgVec3(dest));

        getPathGridGraph(pathgrid).getNeighbouringPoints(index, points);
    }

    void AiWander::getAllowedNodes(const MWWorld::Ptr& actor, AiWanderStorage& storage)
    {
        // infrequently used, therefore no benefit in caching it as a member
        const MWWorld::CellStore* cellStore = actor.getCell();
        const ESM::Pathgrid* pathgrid
            = MWBase::Environment::get().getESMStore()->get<ESM::Pathgrid>().search(*cellStore->getCell());

        storage.mAllowedNodes.clear();

        // If there is no path this actor doesn't go anywhere. See:
        // https://forum.openmw.org/viewtopic.php?t=1556
        // http://www.fliggerty.com/phpBB3/viewtopic.php?f=30&t=5833
        // Note: In order to wander, need at least two points.
        if (!pathgrid || (pathgrid->mPoints.size() < 2))
            storage.mCanWanderAlongPathGrid = false;

        // A distance value passed into the constructor indicates how far the
        // actor can  wander from the spawn position.  AiWander assumes that
        // pathgrid points are available, and uses them to randomly select wander
        // destinations within the allowed set of pathgrid points (nodes).
        // ... pathgrids don't usually include water, so swimmers ignore them
        if (mDistance && storage.mCanWanderAlongPathGrid && !actor.getClass().isPureWaterCreature(actor))
        {
            // get NPC's position in local (i.e. cell) coordinates
            const Misc::CoordinateConverter converter = Misc::makeCoordinateConverter(*cellStore->getCell());
            const osg::Vec3f npcPos = converter.toLocalVec3(mInitialActorPosition);

            // Find closest pathgrid point
            int closestPointIndex = PathFinder::getClosestPoint(pathgrid, npcPos);

            // mAllowedNodes for this actor with pathgrid point indexes based on mDistance
            // and if the point is connected to the closest current point
            // NOTE: mPoints is in local coordinates
            size_t pointIndex = 0;
            for (size_t counter = 0; counter < pathgrid->mPoints.size(); counter++)
            {
                osg::Vec3f nodePos(PathFinder::makeOsgVec3(pathgrid->mPoints[counter]));
                if ((npcPos - nodePos).length2() <= mDistance * mDistance
                    && getPathGridGraph(pathgrid).isPointConnected(closestPointIndex, counter))
                {
                    storage.mAllowedNodes.push_back(converter.toWorldPoint(pathgrid->mPoints[counter]));
                    pointIndex = counter;
                }
            }
            if (storage.mAllowedNodes.size() == 1)
            {
                storage.mAllowedNodes.push_back(PathFinder::makePathgridPoint(mInitialActorPosition));
                addNonPathGridAllowedPoints(pathgrid, pointIndex, storage, converter);
            }
            if (!storage.mAllowedNodes.empty())
            {
                setCurrentNodeToClosestAllowedNode(storage);
            }
        }

        storage.mPopulateAvailableNodes = false;
    }

    // When only one path grid point in wander distance,
    // additional points for NPC to wander to are:
    // 1. NPC's initial location
    // 2. Partway along the path between the point and its connected points.
    void AiWander::addNonPathGridAllowedPoints(const ESM::Pathgrid* pathGrid, size_t pointIndex,
        AiWanderStorage& storage, const Misc::CoordinateConverter& converter)
    {
        for (const auto& edge : pathGrid->mEdges)
        {
            if (edge.mV0 == pointIndex)
            {
                AddPointBetweenPathGridPoints(converter.toWorldPoint(pathGrid->mPoints[edge.mV0]),
                    converter.toWorldPoint(pathGrid->mPoints[edge.mV1]), storage);
            }
        }
    }

    void AiWander::AddPointBetweenPathGridPoints(
        const ESM::Pathgrid::Point& start, const ESM::Pathgrid::Point& end, AiWanderStorage& storage)
    {
        osg::Vec3f vectorStart = PathFinder::makeOsgVec3(start);
        osg::Vec3f delta = PathFinder::makeOsgVec3(end) - vectorStart;
        float length = delta.length();
        delta.normalize();

        int distance = std::max(mDistance / 2, MINIMUM_WANDER_DISTANCE);

        // must not travel longer than distance between waypoints or NPC goes past waypoint
        distance = std::min(distance, static_cast<int>(length));
        delta *= distance;
        storage.mAllowedNodes.push_back(PathFinder::makePathgridPoint(vectorStart + delta));
    }

    void AiWander::setCurrentNodeToClosestAllowedNode(AiWanderStorage& storage)
    {
        float distanceToClosestNode = std::numeric_limits<float>::max();
        size_t index = 0;
        for (size_t i = 0; i < storage.mAllowedNodes.size(); ++i)
        {
            osg::Vec3f nodePos(PathFinder::makeOsgVec3(storage.mAllowedNodes[i]));
            float tempDist = (mInitialActorPosition - nodePos).length2();
            if (tempDist < distanceToClosestNode)
            {
                index = i;
                distanceToClosestNode = tempDist;
            }
        }
        storage.mCurrentNode = storage.mAllowedNodes[index];
        storage.mAllowedNodes.erase(storage.mAllowedNodes.begin() + index);
    }

    void AiWander::writeState(ESM::AiSequence::AiSequence& sequence) const
    {
        float remainingDuration;
        if (mRemainingDuration > 0 && mRemainingDuration < 24)
            remainingDuration = mRemainingDuration;
        else
            remainingDuration = mDuration;

        auto wander = std::make_unique<ESM::AiSequence::AiWander>();
        wander->mData.mDistance = mDistance;
        wander->mData.mDuration = mDuration;
        wander->mData.mTimeOfDay = mTimeOfDay;
        wander->mDurationData.mRemainingDuration = remainingDuration;
        assert(mIdle.size() == 8);
        for (int i = 0; i < 8; ++i)
            wander->mData.mIdle[i] = mIdle[i];
        wander->mData.mShouldRepeat = mOptions.mRepeat;
        wander->mStoredInitialActorPosition = mStoredInitialActorPosition;
        if (mStoredInitialActorPosition)
            wander->mInitialActorPosition = mInitialActorPosition;

        ESM::AiSequence::AiPackageContainer package;
        package.mType = ESM::AiSequence::Ai_Wander;
        package.mPackage = std::move(wander);
        sequence.mPackages.push_back(std::move(package));
    }

    AiWander::AiWander(const ESM::AiSequence::AiWander* wander)
        : TypedAiPackage<AiWander>(makeDefaultOptions().withRepeat(wander->mData.mShouldRepeat != 0))
        , mDistance(std::max(static_cast<short>(0), wander->mData.mDistance))
        , mDuration(std::max(static_cast<short>(0), wander->mData.mDuration))
        , mRemainingDuration(wander->mDurationData.mRemainingDuration)
        , mTimeOfDay(wander->mData.mTimeOfDay)
        , mIdle(getInitialIdle(wander->mData.mIdle))
        , mStoredInitialActorPosition(wander->mStoredInitialActorPosition)
        , mHasDestination(false)
        , mDestination(osg::Vec3f(0, 0, 0))
        , mUsePathgrid(false)
    {
        if (mStoredInitialActorPosition)
            mInitialActorPosition = wander->mInitialActorPosition;
        if (mRemainingDuration <= 0 || mRemainingDuration >= 24)
            mRemainingDuration = mDuration;
    }
}
