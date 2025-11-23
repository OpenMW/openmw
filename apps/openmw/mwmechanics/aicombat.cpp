#include "aicombat.hpp"

#include <components/detournavigator/navigatorutils.hpp>
#include <components/esm3/aisequence.hpp>
#include <components/misc/coordinateconverter.hpp>
#include <components/misc/mathutil.hpp>
#include <components/misc/pathgridutils.hpp>
#include <components/misc/rng.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>

#include "../mwphysics/raycasting.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"

#include "actorutil.hpp"
#include "aicombataction.hpp"
#include "character.hpp"
#include "combat.hpp"
#include "creaturestats.hpp"
#include "movement.hpp"
#include "pathgrid.hpp"
#include "steering.hpp"
#include "weapontype.hpp"

namespace
{

    // chooses an attack depending on probability to avoid uniformity
    std::string_view chooseBestAttack(const ESM::Weapon* weapon);

    osg::Vec3f AimDirToMovingTarget(const MWWorld::Ptr& actor, const MWWorld::Ptr& target,
        const osg::Vec3f& vLastTargetPos, float duration, int weapType, float strength);
}

namespace MWMechanics
{
    AiCombat::AiCombat(const MWWorld::Ptr& actor)
    {
        mTargetActorId = actor.getClass().getCreatureStats(actor).getActorId();
    }

    AiCombat::AiCombat(const ESM::AiSequence::AiCombat* combat)
    {
        mTargetActorId = combat->mTargetActorId;
    }

    void AiCombat::init() {}

    /*
     * Current AiCombat movement states (as of 0.29.0), ignoring the details of the
     * attack states such as CombatMove, Strike and ReadyToAttack:
     *
     *    +----(within strike range)----->attack--(beyond strike range)-->follow
     *    |                                 | ^                            | |
     *    |                                 | |                            | |
     *  pursue<---(beyond follow range)-----+ +----(within strike range)---+ |
     *    ^                                                                  |
     *    |                                                                  |
     *    +-------------------------(beyond follow range)--------------------+
     *
     *
     * Below diagram is high level only, the code detail is a little different
     * (but including those detail will just complicate the diagram w/o adding much)
     *
     *    +----------(same)-------------->attack---------(same)---------->follow
     *    |                                 |^^                            |||
     *    |                                 |||                            |||
     *    |       +--(same)-----------------+|+----------(same)------------+||
     *    |       |                          |                              ||
     *    |       |                          | (in range)                   ||
     *    |   <---+         (too far)        |                              ||
     *  pursue<-------------------------[door open]<-----+                  ||
     *    ^^^                                            |                  ||
     *    |||                                            |                  ||
     *    ||+----------evade-----+                       |                  ||
     *    ||                     |    [closed door]      |                  ||
     *    |+----> maybe stuck, check --------------> back up, check door    ||
     *    |         ^   |   ^                          |   ^                ||
     *    |         |   |   |                          |   |                ||
     *    |         |   +---+                          +---+                ||
     *    |         +-------------------------------------------------------+|
     *    |                                                                  |
     *    +---------------------------(same)---------------------------------+
     *
     * FIXME:
     *
     * The new scheme is way too complicated, should really be implemented as a
     * proper state machine.
     *
     * TODO:
     *
     * Use the observer pattern to coordinate attacks, provide intelligence on
     * whether the target was hit, etc.
     */

    bool AiCombat::execute(
        const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
    {
        // Get or create temporary storage
        AiCombatStorage& storage = state.get<AiCombatStorage>();

        // No combat for dead creatures
        if (actor.getClass().getCreatureStats(actor).isDead())
            return true;

        MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtrViaActorId(mTargetActorId);
        if (target.isEmpty())
            return true;

        if (!target.getCellRef().getCount()
            || !target.getRefData().isEnabled() // Really we should be checking whether the target is currently
                                                // registered with the MechanicsManager
            || target.getClass().getCreatureStats(target).isDead())
            return true;

        if (actor == target) // This should never happen.
            return true;

        // No actions for totally static creatures
        if (!actor.getClass().isMobile(actor))
        {
            storage.mFleeState = AiCombatStorage::FleeState_Idle;
            return false;
        }

        if (!storage.isFleeing())
        {
            if (storage.mCurrentAction.get()) // need to wait to init action with its attack range
            {
                // Update every frame. UpdateLOS uses a timer, so the LOS check does not happen every frame.
                updateLOS(actor, target, duration, storage);
                const float targetReachedTolerance
                    = storage.mLOS && !storage.mUseCustomDestination ? storage.mAttackRange : 0.0f;
                const osg::Vec3f destination = storage.mUseCustomDestination
                    ? storage.mCustomDestination
                    : target.getRefData().getPosition().asVec3();
                const bool isTargetReached = pathTo(actor, destination, duration,
                    characterController.getSupportedMovementDirections(), targetReachedTolerance);
                if (isTargetReached)
                    storage.mReadyToAttack = true;
            }

            storage.updateCombatMove(duration);
            storage.mRotateMove = false;
            if (storage.mReadyToAttack)
                updateActorsMovement(actor, duration, storage);
            if (storage.mRotateMove)
                return false;
            storage.updateAttack(actor, characterController);
        }
        else
        {
            updateFleeing(actor, target, duration, characterController.getSupportedMovementDirections(), storage);
        }
        storage.mActionCooldown -= duration;

        if (storage.mReaction.update(duration) == Misc::TimerStatus::Waiting)
            return false;

        return attack(actor, target, storage, characterController);
    }

    bool AiCombat::attack(const MWWorld::Ptr& actor, const MWWorld::Ptr& target, AiCombatStorage& storage,
        CharacterController& characterController)
    {
        const MWWorld::CellStore*& currentCell = storage.mCell;
        bool cellChange = currentCell && (actor.getCell() != currentCell);
        if (!currentCell || cellChange)
        {
            currentCell = actor.getCell();
        }

        const MWWorld::Class& actorClass = actor.getClass();
        MWMechanics::CreatureStats& stats = actorClass.getCreatureStats(actor);
        if (stats.isParalyzed() || stats.getKnockedDown())
            return false;

        bool forceFlee = false;
        if (!canFight(actor, target))
        {
            storage.stopAttack();
            stats.setAttackingOrSpell(false);
            storage.mActionCooldown = 0.f;
            // Continue combat if target is player or player follower/escorter and an attack has been attempted
            const auto& playerFollowersAndEscorters
                = MWBase::Environment::get().getMechanicsManager()->getActorsSidingWith(MWMechanics::getPlayer());
            bool targetSidesWithPlayer
                = (std::find(playerFollowersAndEscorters.begin(), playerFollowersAndEscorters.end(), target)
                    != playerFollowersAndEscorters.end());
            if ((target == MWMechanics::getPlayer() || targetSidesWithPlayer)
                && ((stats.getHitAttemptActorId() == target.getClass().getCreatureStats(target).getActorId())
                    || (target.getClass().getCreatureStats(target).getHitAttemptActorId() == stats.getActorId())))
                forceFlee = true;
            else // Otherwise end combat
                return true;
        }

        stats.setMovementFlag(CreatureStats::Flag_Run, true);
        float& actionCooldown = storage.mActionCooldown;
        std::unique_ptr<Action>& currentAction = storage.mCurrentAction;

        if (!forceFlee)
        {
            if (actionCooldown > 0)
                return false;

            if (characterController.readyToPrepareAttack())
            {
                currentAction = prepareNextAction(actor, target);
                actionCooldown = currentAction->getActionCooldown();
            }
        }
        else
        {
            currentAction = std::make_unique<ActionFlee>();
            actionCooldown = currentAction->getActionCooldown();
        }

        if (!currentAction)
            return false;

        if (storage.isFleeing() != currentAction->isFleeing())
        {
            if (currentAction->isFleeing())
            {
                storage.startFleeing();
                MWBase::Environment::get().getDialogueManager()->say(actor, ESM::RefId::stringRefId("flee"));
                return false;
            }
            else
                storage.stopFleeing();
        }

        bool isRangedCombat = false;
        float& rangeAttack = storage.mAttackRange;

        rangeAttack = currentAction->getCombatRange(isRangedCombat);

        // Get weapon characteristics
        const ESM::Weapon* weapon = currentAction->getWeapon();

        ESM::Position pos = actor.getRefData().getPosition();
        const osg::Vec3f vActorPos(pos.asVec3());
        const osg::Vec3f vTargetPos(target.getRefData().getPosition().asVec3());

        float distToTarget = getDistanceToBounds(actor, target);

        storage.mReadyToAttack = (currentAction->isAttackingOrSpell() && distToTarget <= rangeAttack && storage.mLOS);

        if (isRangedCombat)
        {
            // rotate actor taking into account target movement direction and projectile speed
            osg::Vec3f vAimDir = AimDirToMovingTarget(actor, target, storage.mLastTargetPos, AI_REACTION_TIME,
                (weapon ? weapon->mData.mType : 0), storage.mStrength);

            storage.mMovement.mRotation[0] = getXAngleToDir(vAimDir);
            storage.mMovement.mRotation[2] = getZAngleToDir(vAimDir);
        }
        else
        {
            osg::Vec3f vAimDir = MWBase::Environment::get().getWorld()->aimToTarget(actor, target, false);
            storage.mMovement.mRotation[0] = getXAngleToDir(vAimDir);
            storage.mMovement.mRotation[2] = getZAngleToDir(
                (vTargetPos - vActorPos)); // using vAimDir results in spastic movements since the head is animated
        }

        storage.mLastTargetPos = vTargetPos;

        if (storage.mReadyToAttack)
        {
            storage.startCombatMove(isRangedCombat, distToTarget, rangeAttack, actor, target);
            // start new attack
            bool canShout = true;
            ESM::RefId spellId = storage.mCurrentAction->getSpell();
            if (!spellId.empty())
            {
                const ESM::Spell* spell = MWBase::Environment::get().getESMStore()->get<ESM::Spell>().find(spellId);
                if (spell->mEffects.mList.empty() || spell->mEffects.mList[0].mData.mRange != ESM::RT_Target)
                    canShout = false;
            }
            storage.startAttackIfReady(actor, characterController, weapon, isRangedCombat, canShout);
        }

        // If actor uses custom destination it has to try to rebuild path because environment can change
        // (door is opened between actor and target) or target position has changed and current custom destination
        // is not good enough to attack target.
        if (storage.mCurrentAction->isAttackingOrSpell()
            && ((!storage.mReadyToAttack && !mPathFinder.isPathConstructed())
                || (storage.mUseCustomDestination && (storage.mCustomDestination - vTargetPos).length() > rangeAttack)))
        {
            const MWBase::World* world = MWBase::Environment::get().getWorld();
            // Try to build path to the target.
            const auto agentBounds = world->getPathfindingAgentBounds(actor);
            const DetourNavigator::Flags navigatorFlags = getNavigatorFlags(actor);
            const DetourNavigator::AreaCosts areaCosts = getAreaCosts(actor, navigatorFlags);
            const ESM::Pathgrid* pathgrid = world->getStore().get<ESM::Pathgrid>().search(*actor.getCell()->getCell());
            const auto& pathGridGraph = getPathGridGraph(pathgrid);
            mPathFinder.buildPath(actor, vActorPos, vTargetPos, pathGridGraph, agentBounds, navigatorFlags, areaCosts,
                storage.mAttackRange, PathType::Full);

            if (!mPathFinder.isPathConstructed())
            {
                // If there is no path, try to find a point on a line from the actor position to target projected
                // on navmesh to attack the target from there.
                const auto navigator = world->getNavigator();
                const auto hit
                    = DetourNavigator::raycast(*navigator, agentBounds, vActorPos, vTargetPos, navigatorFlags);

                if (hit.has_value() && (*hit - vTargetPos).length() <= rangeAttack)
                {
                    // If the point is close enough, try to find a path to that point.
                    mPathFinder.buildPath(actor, vActorPos, *hit, pathGridGraph, agentBounds, navigatorFlags, areaCosts,
                        storage.mAttackRange, PathType::Full);
                    if (mPathFinder.isPathConstructed())
                    {
                        // If path to that point is found use it as custom destination.
                        storage.mCustomDestination = *hit;
                        storage.mUseCustomDestination = true;
                    }
                }

                if (!mPathFinder.isPathConstructed())
                {
                    storage.mUseCustomDestination = false;
                    storage.stopAttack();
                    stats.setAttackingOrSpell(false);
                    currentAction = std::make_unique<ActionFlee>();
                    actionCooldown = currentAction->getActionCooldown();
                    storage.startFleeing();
                    MWBase::Environment::get().getDialogueManager()->say(actor, ESM::RefId::stringRefId("flee"));
                }
            }
            else
            {
                storage.mUseCustomDestination = false;
            }
        }

        return false;
    }

    void MWMechanics::AiCombat::updateLOS(
        const MWWorld::Ptr& actor, const MWWorld::Ptr& target, float duration, MWMechanics::AiCombatStorage& storage)
    {
        const float losUpdateDuration = 0.5f;
        if (storage.mUpdateLOSTimer <= 0.f)
        {
            storage.mLOS = MWBase::Environment::get().getWorld()->getLOS(actor, target);
            storage.mUpdateLOSTimer = losUpdateDuration;
        }
        else
            storage.mUpdateLOSTimer -= duration;
    }

    void MWMechanics::AiCombat::updateFleeing(const MWWorld::Ptr& actor, const MWWorld::Ptr& target, float duration,
        MWWorld::MovementDirectionFlags supportedMovementDirections, AiCombatStorage& storage)
    {
        const float blindRunDuration = 1.0f;

        updateLOS(actor, target, duration, storage);

        AiCombatStorage::FleeState& state = storage.mFleeState;
        switch (state)
        {
            case AiCombatStorage::FleeState_None:
                return;

            case AiCombatStorage::FleeState_Idle:
            {
                float triggerDist = getMaxAttackDistance(target);
                const MWWorld::Cell* cellVariant = storage.mCell->getCell();
                if (storage.mLOS && (triggerDist >= 1000 || getDistanceMinusHalfExtents(actor, target) <= triggerDist))
                {
                    const ESM::Pathgrid* pathgrid
                        = MWBase::Environment::get().getESMStore()->get<ESM::Pathgrid>().search(*cellVariant);

                    bool runFallback = true;

                    if (pathgrid != nullptr && !pathgrid->mPoints.empty()
                        && !actor.getClass().isPureWaterCreature(actor))
                    {
                        ESM::Pathgrid::PointList points;
                        const Misc::CoordinateConverter coords
                            = Misc::makeCoordinateConverter(*storage.mCell->getCell());

                        osg::Vec3f localPos = actor.getRefData().getPosition().asVec3();
                        coords.toLocal(localPos);

                        const std::size_t closestPointIndex = Misc::getClosestPoint(*pathgrid, localPos);
                        for (std::size_t i = 0; i < pathgrid->mPoints.size(); i++)
                        {
                            if (i != closestPointIndex
                                && getPathGridGraph(pathgrid).isPointConnected(closestPointIndex, i))
                            {
                                points.push_back(pathgrid->mPoints[i]);
                            }
                        }

                        if (!points.empty())
                        {
                            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
                            ESM::Pathgrid::Point dest = points[Misc::Rng::rollDice(points.size(), prng)];
                            coords.toWorld(dest);

                            state = AiCombatStorage::FleeState_RunToDestination;
                            storage.mFleeDest = ESM::Pathgrid::Point(dest.mX, dest.mY, dest.mZ);

                            runFallback = false;
                        }
                    }

                    if (runFallback)
                    {
                        state = AiCombatStorage::FleeState_RunBlindly;
                        storage.mFleeBlindRunTimer = 0.0f;
                    }
                }
            }
            break;

            case AiCombatStorage::FleeState_RunBlindly:
            {
                // timer to prevent twitchy movement that can be observed in vanilla MW
                if (storage.mFleeBlindRunTimer < blindRunDuration)
                {
                    storage.mFleeBlindRunTimer += duration;

                    storage.mMovement.mRotation[0] = -actor.getRefData().getPosition().rot[0];
                    storage.mMovement.mRotation[2] = osg::PIf
                        + getZAngleToDir(
                            target.getRefData().getPosition().asVec3() - actor.getRefData().getPosition().asVec3());
                    storage.mMovement.mPosition[1] = 1;
                    updateActorsMovement(actor, duration, storage);
                }
                else
                    state = AiCombatStorage::FleeState_Idle;
            }
            break;

            case AiCombatStorage::FleeState_RunToDestination:
            {
                static const float fFleeDistance = MWBase::Environment::get()
                                                       .getESMStore()
                                                       ->get<ESM::GameSetting>()
                                                       .find("fFleeDistance")
                                                       ->mValue.getFloat();

                float dist
                    = (actor.getRefData().getPosition().asVec3() - target.getRefData().getPosition().asVec3()).length();
                if ((dist > fFleeDistance && !storage.mLOS)
                    || pathTo(
                        actor, Misc::Convert::makeOsgVec3f(storage.mFleeDest), duration, supportedMovementDirections))
                {
                    state = AiCombatStorage::FleeState_Idle;
                }
            }
            break;
        };
    }

    void AiCombat::updateActorsMovement(const MWWorld::Ptr& actor, float duration, AiCombatStorage& storage)
    {
        // apply combat movement
        float deltaAngle = storage.mMovement.mRotation[2] - actor.getRefData().getPosition().rot[2];
        osg::Vec2f movement = Misc::rotateVec2f(
            osg::Vec2f(storage.mMovement.mPosition[0], storage.mMovement.mPosition[1]), -deltaAngle);

        MWMechanics::Movement& actorMovementSettings = actor.getClass().getMovementSettings(actor);
        actorMovementSettings.mPosition[0] = movement.x();
        actorMovementSettings.mPosition[1] = movement.y();
        actorMovementSettings.mPosition[2] = storage.mMovement.mPosition[2];

        rotateActorOnAxis(actor, 2, actorMovementSettings, storage);
        rotateActorOnAxis(actor, 0, actorMovementSettings, storage);
    }

    void AiCombat::rotateActorOnAxis(
        const MWWorld::Ptr& actor, int axis, MWMechanics::Movement& actorMovementSettings, AiCombatStorage& storage)
    {
        actorMovementSettings.mRotation[axis] = 0;
        bool isRangedCombat = false;
        storage.mCurrentAction->getCombatRange(isRangedCombat);
        float eps = isRangedCombat ? osg::DegreesToRadians(0.5f) : osg::DegreesToRadians(3.f);
        float targetAngleRadians = storage.mMovement.mRotation[axis];
        storage.mRotateMove = !smoothTurn(actor, targetAngleRadians, axis, eps);
    }

    MWWorld::Ptr AiCombat::getTarget() const
    {
        if (mCachedTarget.isEmpty() || mCachedTarget.mRef->isDeleted() || !mCachedTarget.getRefData().isEnabled())
        {
            mCachedTarget = MWBase::Environment::get().getWorld()->searchPtrViaActorId(mTargetActorId);
        }
        return mCachedTarget;
    }

    void AiCombat::writeState(ESM::AiSequence::AiSequence& sequence) const
    {
        auto combat = std::make_unique<ESM::AiSequence::AiCombat>();
        combat->mTargetActorId = mTargetActorId;

        ESM::AiSequence::AiPackageContainer package;
        package.mType = ESM::AiSequence::Ai_Combat;
        package.mPackage = std::move(combat);
        sequence.mPackages.push_back(std::move(package));
    }

    AiCombatStorage::AiCombatStorage()
        : mAttackCooldown(0.0f)
        , mReaction(MWBase::Environment::get().getWorld()->getPrng())
        , mTimerCombatMove(0.0f)
        , mReadyToAttack(false)
        , mAttack(false)
        , mAttackRange(0.0f)
        , mCombatMove(false)
        , mRotateMove(false)
        , mLastTargetPos(0, 0, 0)
        , mCell(nullptr)
        , mCurrentAction()
        , mActionCooldown(0.0f)
        , mStrength()
        , mForceNoShortcut(false)
        , mShortcutFailPos()
        , mMovement()
        , mFleeState(FleeState_None)
        , mLOS(false)
        , mUpdateLOSTimer(0.0f)
        , mFleeBlindRunTimer(0.0f)
        , mUseCustomDestination(false)
        , mCustomDestination()
    {
    }

    void AiCombatStorage::startCombatMove(bool isDistantCombat, float distToTarget, float rangeAttack,
        const MWWorld::Ptr& actor, const MWWorld::Ptr& target)
    {
        auto& prng = MWBase::Environment::get().getWorld()->getPrng();

        // get the range of the target's weapon
        MWWorld::Ptr targetWeapon = MWWorld::Ptr();
        const MWWorld::Class& targetClass = target.getClass();

        if (targetClass.hasInventoryStore(target))
        {
            int weapType = ESM::Weapon::None;
            MWWorld::ContainerStoreIterator weaponSlot = MWMechanics::getActiveWeapon(target, &weapType);
            if (weapType > ESM::Weapon::None)
                targetWeapon = *weaponSlot;
        }

        bool targetUsesRanged = false;
        float rangeAttackOfTarget = ActionWeapon(targetWeapon).getCombatRange(targetUsesRanged);

        if (mMovement.mPosition[0])
        {
            mTimerCombatMove = 0.1f + 0.1f * Misc::Rng::rollClosedProbability(prng);
            mCombatMove = true;
        }
        // dodge movements (for NPCs and bipedal creatures)
        // Note: do not use for ranged combat yet since in couple with back up behaviour can move actor out of cliff
        else if (actor.getClass().isBipedal(actor) && !isDistantCombat)
        {
            float moveDuration = 0;
            double angleToTarget
                = Misc::normalizeAngle(mMovement.mRotation[2] - actor.getRefData().getPosition().rot[2]);
            // Apply a big side step if enemy tries to get around and come from behind.
            // Otherwise apply a random side step (kind of dodging) with some probability
            // if actor is within range of target's weapon.
            if (std::abs(angleToTarget) > osg::PI / 4)
                moveDuration = 0.2f;
            else if (distToTarget <= rangeAttackOfTarget && Misc::Rng::rollClosedProbability(prng) < 0.25)
                moveDuration = 0.1f + 0.1f * Misc::Rng::rollClosedProbability(prng);
            if (moveDuration > 0)
            {
                mMovement.mPosition[0] = Misc::Rng::rollProbability(prng) < 0.5 ? 1.0f : -1.0f; // to the left/right
                mTimerCombatMove = moveDuration;
                mCombatMove = true;
            }
        }

        mMovement.mPosition[1] = 0;
        if (isDistantCombat)
        {
            // Backing up behaviour
            // Actor backs up slightly further away than opponent's weapon range
            // (in vanilla - only as far as opponent's weapon range),
            // or not at all if opponent is using a ranged weapon

            if (targetUsesRanged
                || distToTarget > rangeAttackOfTarget * 1.5) // Don't back up if the target is wielding ranged weapon
                return;

            // actor should not back up into water
            if (MWBase::Environment::get().getWorld()->isUnderwater(MWWorld::ConstPtr(actor), 0.5f))
                return;

            int mask
                = MWPhysics::CollisionType_World | MWPhysics::CollisionType_HeightMap | MWPhysics::CollisionType_Door;

            // Actor can not back up if there is no free space behind
            // Currently we take the 35% of actor's height from the ground as vector height.
            // This approach allows us to detect small obstacles (e.g. crates) and curved walls.
            osg::Vec3f halfExtents = MWBase::Environment::get().getWorld()->getHalfExtents(actor);
            osg::Vec3f pos = actor.getRefData().getPosition().asVec3();
            osg::Vec3f source = pos + osg::Vec3f(0, 0, 0.75f * halfExtents.z());
            osg::Vec3f fallbackDirection = actor.getRefData().getBaseNode()->getAttitude() * osg::Vec3f(0, -1, 0);
            osg::Vec3f destination = source + fallbackDirection * (halfExtents.y() + 16);

            const auto* rayCasting = MWBase::Environment::get().getWorld()->getRayCasting();
            bool isObstacleDetected = rayCasting->castRay(source, destination, mask).mHit;
            if (isObstacleDetected)
                return;

            // Check if there is nothing behind - probably actor is near cliff.
            // A current approach: cast ray 1.5-yard ray down in 1.5 yard behind actor from 35% of actor's height.
            // If we did not hit anything, there is a cliff behind actor.
            source = pos + osg::Vec3f(0, 0, 0.75f * halfExtents.z()) + fallbackDirection * (halfExtents.y() + 96);
            destination = source - osg::Vec3f(0, 0, 0.75f * halfExtents.z() + 96);
            bool isCliffDetected = !rayCasting->castRay(source, destination, mask).mHit;
            if (isCliffDetected)
                return;

            mMovement.mPosition[1] = -1;
        }
    }

    void AiCombatStorage::updateCombatMove(float duration)
    {
        if (mCombatMove)
        {
            mTimerCombatMove -= duration;
            if (mTimerCombatMove <= 0)
            {
                stopCombatMove();
            }
        }
    }

    void AiCombatStorage::stopCombatMove()
    {
        mTimerCombatMove = 0;
        mMovement.mPosition[0] = 0;
        mCombatMove = false;
    }

    void AiCombatStorage::startAttackIfReady(const MWWorld::Ptr& actor, CharacterController& characterController,
        const ESM::Weapon* weapon, bool distantCombat, bool canShout)
    {
        if (mReadyToAttack && characterController.readyToStartAttack())
        {
            if (mAttackCooldown <= 0)
            {
                mAttack = true; // attack starts just now
                actor.getClass().getCreatureStats(actor).setAttackingOrSpell(true);

                if (!distantCombat)
                    characterController.setAIAttackType(chooseBestAttack(weapon));

                auto& prng = MWBase::Environment::get().getWorld()->getPrng();
                mStrength = Misc::Rng::rollClosedProbability(prng);

                const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();

                float baseDelay = store.get<ESM::GameSetting>().find("fCombatDelayCreature")->mValue.getFloat();
                if (actor.getClass().isNpc())
                {
                    baseDelay = store.get<ESM::GameSetting>().find("fCombatDelayNPC")->mValue.getFloat();
                }

                if (canShout)
                {
                    // Say a provoking combat phrase
                    const int iVoiceAttackOdds
                        = store.get<ESM::GameSetting>().find("iVoiceAttackOdds")->mValue.getInteger();
                    if (Misc::Rng::roll0to99(prng) < iVoiceAttackOdds)
                    {
                        MWBase::Environment::get().getDialogueManager()->say(actor, ESM::RefId::stringRefId("attack"));
                    }
                }
                mAttackCooldown = std::min(baseDelay + 0.01f * Misc::Rng::roll0to99(prng), baseDelay + 0.9f);
            }
            else
                mAttackCooldown -= AI_REACTION_TIME;
        }
    }

    void AiCombatStorage::updateAttack(const MWWorld::Ptr& actor, CharacterController& characterController)
    {
        if (mAttack)
        {
            float attackStrength = characterController.calculateWindUp();
            mAttack
                = !characterController.readyToPrepareAttack() && attackStrength < mStrength && attackStrength != -1.f;
        }
        actor.getClass().getCreatureStats(actor).setAttackingOrSpell(mAttack);
    }

    void AiCombatStorage::stopAttack()
    {
        mMovement.mPosition[0] = 0;
        mMovement.mPosition[1] = 0;
        mMovement.mPosition[2] = 0;
        mReadyToAttack = false;
        mAttack = false;
    }

    void AiCombatStorage::startFleeing()
    {
        stopFleeing();
        mFleeState = FleeState_Idle;
    }

    void AiCombatStorage::stopFleeing()
    {
        mMovement.mPosition[0] = 0;
        mMovement.mPosition[1] = 0;
        mMovement.mPosition[2] = 0;
        mFleeState = FleeState_None;
        mFleeDest = ESM::Pathgrid::Point(0, 0, 0);
    }

    bool AiCombatStorage::isFleeing() const
    {
        return mFleeState != FleeState_None;
    }
}

namespace
{

    std::string_view chooseBestAttack(const ESM::Weapon* weapon)
    {
        if (weapon != nullptr)
        {
            // the more damage attackType deals the more probability it has
            int slash = (weapon->mData.mSlash[0] + weapon->mData.mSlash[1]) / 2;
            int chop = (weapon->mData.mChop[0] + weapon->mData.mChop[1]) / 2;
            int thrust = (weapon->mData.mThrust[0] + weapon->mData.mThrust[1]) / 2;

            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
            float roll = Misc::Rng::rollClosedProbability(prng) * (slash + chop + thrust);
            if (roll <= slash)
                return "slash";
            else if (roll <= (slash + thrust))
                return "thrust";
            else
                return "chop";
        }
        return MWMechanics::CharacterController::getRandomAttackType();
    }

    osg::Vec3f AimDirToMovingTarget(const MWWorld::Ptr& actor, const MWWorld::Ptr& target,
        const osg::Vec3f& vLastTargetPos, float duration, int weapType, float strength)
    {
        float projSpeed;
        const MWWorld::Store<ESM::GameSetting>& gmst
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();

        // get projectile speed (depending on weapon type)
        if (MWMechanics::getWeaponType(weapType)->mWeaponClass == ESM::WeaponType::Thrown)
        {
            static float fThrownWeaponMinSpeed = gmst.find("fThrownWeaponMinSpeed")->mValue.getFloat();
            static float fThrownWeaponMaxSpeed = gmst.find("fThrownWeaponMaxSpeed")->mValue.getFloat();

            projSpeed = fThrownWeaponMinSpeed + (fThrownWeaponMaxSpeed - fThrownWeaponMinSpeed) * strength;
        }
        else if (weapType != 0)
        {
            static float fProjectileMinSpeed = gmst.find("fProjectileMinSpeed")->mValue.getFloat();
            static float fProjectileMaxSpeed = gmst.find("fProjectileMaxSpeed")->mValue.getFloat();

            projSpeed = fProjectileMinSpeed + (fProjectileMaxSpeed - fProjectileMinSpeed) * strength;
        }
        else // weapType is 0 ==> it's a target spell projectile
        {
            projSpeed = gmst.find("fTargetSpellMaxSpeed")->mValue.getFloat();
        }

        // idea: perpendicular to dir to target speed components of target move vector and projectile vector should be
        // the same

        osg::Vec3f vTargetPos = target.getRefData().getPosition().asVec3();
        osg::Vec3f vDirToTarget = MWBase::Environment::get().getWorld()->aimToTarget(actor, target, true);
        float distToTarget = vDirToTarget.length();

        osg::Vec3f vTargetMoveDir = vTargetPos - vLastTargetPos;
        vTargetMoveDir /= duration; // |vTargetMoveDir| is target real speed in units/sec now

        osg::Vec3f vPerpToDir = vDirToTarget ^ osg::Vec3f(0, 0, 1); // cross product

        vPerpToDir.normalize();
        osg::Vec3f vDirToTargetNormalized = vDirToTarget;
        vDirToTargetNormalized.normalize();

        // dot product
        float velPerp = vTargetMoveDir * vPerpToDir;
        float velDir = vTargetMoveDir * vDirToTargetNormalized;

        // time to collision between target and projectile
        float tCollision;

        float projVelDirSquared = projSpeed * projSpeed - velPerp * velPerp;
        if (projVelDirSquared > 0)
        {
            osg::Vec3f vTargetMoveDirNormalized = vTargetMoveDir;
            vTargetMoveDirNormalized.normalize();

            float projDistDiff = vDirToTarget * vTargetMoveDirNormalized; // dot product
            projDistDiff = std::sqrt(distToTarget * distToTarget - projDistDiff * projDistDiff);

            tCollision = projDistDiff / (std::sqrt(projVelDirSquared) - velDir);
        }
        else
            tCollision = 0; // speed of projectile is not enough to reach moving target

        return vDirToTarget + vTargetMoveDir * tCollision;
    }

}
