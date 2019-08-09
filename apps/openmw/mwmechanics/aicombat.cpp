#include "aicombat.hpp"

#include <components/misc/rng.hpp>

#include <components/esm/aisequence.hpp>

#include <components/sceneutil/positionattitudetransform.hpp>

#include "../mwphysics/collisiontype.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "pathgrid.hpp"
#include "creaturestats.hpp"
#include "steering.hpp"
#include "movement.hpp"
#include "character.hpp"
#include "aicombataction.hpp"
#include "coordinateconverter.hpp"
#include "actorutil.hpp"

namespace
{

    //chooses an attack depending on probability to avoid uniformity
    std::string chooseBestAttack(const ESM::Weapon* weapon);

    osg::Vec3f AimDirToMovingTarget(const MWWorld::Ptr& actor, const MWWorld::Ptr& target, const osg::Vec3f& vLastTargetPos,
        float duration, int weapType, float strength);
}

namespace MWMechanics
{
    AiCombat::AiCombat(const MWWorld::Ptr& actor)
    {
        mTargetActorId = actor.getClass().getCreatureStats(actor).getActorId();
    }

    AiCombat::AiCombat(const ESM::AiSequence::AiCombat *combat)
    {
        mTargetActorId = combat->mTargetActorId;
    }

    void AiCombat::init()
    {

    }

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

    bool AiCombat::execute (const MWWorld::Ptr& actor, CharacterController& characterController, AiState& state, float duration)
    {
        // get or create temporary storage
        AiCombatStorage& storage = state.get<AiCombatStorage>();
        
        //General description
        if (actor.getClass().getCreatureStats(actor).isDead())
            return true;

        MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtrViaActorId(mTargetActorId);
        if (target.isEmpty())
            return false;

        if(!target.getRefData().getCount() || !target.getRefData().isEnabled()  // Really we should be checking whether the target is currently registered
                                                                                // with the MechanicsManager
                || target.getClass().getCreatureStats(target).isDead())
            return true;

        if (actor == target) // This should never happen.
            return true;

        if (!storage.isFleeing())
        {
            if (storage.mCurrentAction.get()) // need to wait to init action with its attack range
            {
                //Update every frame. UpdateLOS uses a timer, so the LOS check does not happen every frame.
                updateLOS(actor, target, duration, storage);
                float targetReachedTolerance = 0.0f;
                if (storage.mLOS)
                    targetReachedTolerance = storage.mAttackRange;
                const bool is_target_reached = pathTo(actor, target.getRefData().getPosition().asVec3(), duration, targetReachedTolerance);
                if (is_target_reached) storage.mReadyToAttack = true;
            }

            storage.updateCombatMove(duration);
            if (storage.mReadyToAttack) updateActorsMovement(actor, duration, storage);
            storage.updateAttack(characterController);
        }
        else
        {
            updateFleeing(actor, target, duration, storage);
        }
        storage.mActionCooldown -= duration;

        float& timerReact = storage.mTimerReact;
        if (timerReact < AI_REACTION_TIME)
        {
            timerReact += duration;
        }
        else
        {
            timerReact = 0;
            if (attack(actor, target, storage, characterController))
                return true;
        }

        return false;
    }

    bool AiCombat::attack(const MWWorld::Ptr& actor, const MWWorld::Ptr& target, AiCombatStorage& storage, CharacterController& characterController)
    {
        const MWWorld::CellStore*& currentCell = storage.mCell;
        bool cellChange = currentCell && (actor.getCell() != currentCell);
        if(!currentCell || cellChange)
        {
            currentCell = actor.getCell();
        }

        bool forceFlee = false;
        if (!canFight(actor, target))
        {
            storage.stopAttack();
            characterController.setAttackingOrSpell(false);
            storage.mActionCooldown = 0.f;
            // Continue combat if target is player or player follower/escorter and an attack has been attempted
            const std::list<MWWorld::Ptr>& playerFollowersAndEscorters = MWBase::Environment::get().getMechanicsManager()->getActorsSidingWith(MWMechanics::getPlayer());
            bool targetSidesWithPlayer = (std::find(playerFollowersAndEscorters.begin(), playerFollowersAndEscorters.end(), target) != playerFollowersAndEscorters.end());
            if ((target == MWMechanics::getPlayer() || targetSidesWithPlayer)
                && ((actor.getClass().getCreatureStats(actor).getHitAttemptActorId() == target.getClass().getCreatureStats(target).getActorId())
                || (target.getClass().getCreatureStats(target).getHitAttemptActorId() == actor.getClass().getCreatureStats(actor).getActorId())))
                forceFlee = true;
            else // Otherwise end combat
                return true;
        }

        const MWWorld::Class& actorClass = actor.getClass();
        actorClass.getCreatureStats(actor).setMovementFlag(CreatureStats::Flag_Run, true);

        float& actionCooldown = storage.mActionCooldown;
        std::shared_ptr<Action>& currentAction = storage.mCurrentAction;

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
            currentAction.reset(new ActionFlee());
            actionCooldown = currentAction->getActionCooldown();
        }

        if (!currentAction)
            return false;

        if (storage.isFleeing() != currentAction->isFleeing())
        {
            if (currentAction->isFleeing())
            {
                storage.startFleeing();
                MWBase::Environment::get().getDialogueManager()->say(actor, "flee");
                return false;
            }
            else
                storage.stopFleeing();
        }

        bool isRangedCombat = false;
        float &rangeAttack = storage.mAttackRange;

        rangeAttack = currentAction->getCombatRange(isRangedCombat);

        // Get weapon characteristics
        const ESM::Weapon* weapon = currentAction->getWeapon();

        ESM::Position pos = actor.getRefData().getPosition();
        osg::Vec3f vActorPos(pos.asVec3());
        osg::Vec3f vTargetPos(target.getRefData().getPosition().asVec3());

        osg::Vec3f vAimDir = MWBase::Environment::get().getWorld()->aimToTarget(actor, target);
        float distToTarget = MWBase::Environment::get().getWorld()->getHitDistance(actor, target);

        storage.mReadyToAttack = (currentAction->isAttackingOrSpell() && distToTarget <= rangeAttack && storage.mLOS);

        if (storage.mReadyToAttack)
        {
            storage.startCombatMove(isRangedCombat, distToTarget, rangeAttack, actor, target);
            // start new attack
            storage.startAttackIfReady(actor, characterController, weapon, isRangedCombat);

            if (isRangedCombat)
            {
                // rotate actor taking into account target movement direction and projectile speed
                osg::Vec3f& lastTargetPos = storage.mLastTargetPos;
                vAimDir = AimDirToMovingTarget(actor, target, lastTargetPos, AI_REACTION_TIME, (weapon ? weapon->mData.mType : 0), storage.mStrength);
                lastTargetPos = vTargetPos;

                storage.mMovement.mRotation[0] = getXAngleToDir(vAimDir);
                storage.mMovement.mRotation[2] = getZAngleToDir(vAimDir);
            }
            else
            {
                storage.mMovement.mRotation[0] = getXAngleToDir(vAimDir);
                storage.mMovement.mRotation[2] = getZAngleToDir((vTargetPos-vActorPos)); // using vAimDir results in spastic movements since the head is animated
            }
        }
        return false;
    }

    void MWMechanics::AiCombat::updateLOS(const MWWorld::Ptr& actor, const MWWorld::Ptr& target, float duration, MWMechanics::AiCombatStorage& storage)
    {
        static const float LOS_UPDATE_DURATION = 0.5f;
        if (storage.mUpdateLOSTimer <= 0.f)
        {
            storage.mLOS = MWBase::Environment::get().getWorld()->getLOS(actor, target);
            storage.mUpdateLOSTimer = LOS_UPDATE_DURATION;
        }
        else
            storage.mUpdateLOSTimer -= duration;
    }

    void MWMechanics::AiCombat::updateFleeing(const MWWorld::Ptr& actor, const MWWorld::Ptr& target, float duration, MWMechanics::AiCombatStorage& storage)
    {
        static const float BLIND_RUN_DURATION = 1.0f;

        updateLOS(actor, target, duration, storage);

        AiCombatStorage::FleeState& state = storage.mFleeState;
        switch (state)
        {
            case AiCombatStorage::FleeState_None:
                return;

            case AiCombatStorage::FleeState_Idle:
                {
                    float triggerDist = getMaxAttackDistance(target);

                    if (storage.mLOS &&
                            (triggerDist >= 1000 || getDistanceMinusHalfExtents(actor, target) <= triggerDist))
                    {
                        const ESM::Pathgrid* pathgrid =
                                MWBase::Environment::get().getWorld()->getStore().get<ESM::Pathgrid>().search(*storage.mCell->getCell());

                        bool runFallback = true;

                        if (pathgrid && !actor.getClass().isPureWaterCreature(actor))
                        {
                            ESM::Pathgrid::PointList points;
                            CoordinateConverter coords(storage.mCell->getCell());

                            osg::Vec3f localPos = actor.getRefData().getPosition().asVec3();
                            coords.toLocal(localPos);

                            int closestPointIndex = PathFinder::getClosestPoint(pathgrid, localPos);
                            for (int i = 0; i < static_cast<int>(pathgrid->mPoints.size()); i++)
                            {
                                if (i != closestPointIndex && getPathGridGraph(storage.mCell).isPointConnected(closestPointIndex, i))
                                {
                                    points.push_back(pathgrid->mPoints[static_cast<size_t>(i)]);
                                }
                            }

                            if (!points.empty())
                            {
                                ESM::Pathgrid::Point dest = points[Misc::Rng::rollDice(points.size())];
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
                    if (storage.mFleeBlindRunTimer < BLIND_RUN_DURATION)
                    {
                        storage.mFleeBlindRunTimer += duration;

                        storage.mMovement.mRotation[2] = osg::PI + getZAngleToDir(target.getRefData().getPosition().asVec3()-actor.getRefData().getPosition().asVec3());
                        storage.mMovement.mPosition[1] = 1;
                        updateActorsMovement(actor, duration, storage);
                    }
                    else
                        state = AiCombatStorage::FleeState_Idle;
                }
                break;

            case AiCombatStorage::FleeState_RunToDestination:
                {
                    static const float fFleeDistance = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fFleeDistance")->mValue.getFloat();

                    float dist = (actor.getRefData().getPosition().asVec3() - target.getRefData().getPosition().asVec3()).length();
                    if ((dist > fFleeDistance && !storage.mLOS)
                            || pathTo(actor, PathFinder::makeOsgVec3(storage.mFleeDest), duration))
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
        MWMechanics::Movement& actorMovementSettings = actor.getClass().getMovementSettings(actor);
        actorMovementSettings.mPosition[0] = storage.mMovement.mPosition[0];
        actorMovementSettings.mPosition[1] = storage.mMovement.mPosition[1];
        actorMovementSettings.mPosition[2] = storage.mMovement.mPosition[2];

        rotateActorOnAxis(actor, 2, actorMovementSettings, storage);
        rotateActorOnAxis(actor, 0, actorMovementSettings, storage);
    }

    void AiCombat::rotateActorOnAxis(const MWWorld::Ptr& actor, int axis, 
        MWMechanics::Movement& actorMovementSettings, AiCombatStorage& storage)
    {
        actorMovementSettings.mRotation[axis] = 0;
        float& targetAngleRadians = storage.mMovement.mRotation[axis];
        if (targetAngleRadians != 0)
        {
            // Some attack animations contain small amount of movement.
            // Since we use cone shapes for melee, we can use a threshold to avoid jittering
            std::shared_ptr<Action>& currentAction = storage.mCurrentAction;
            bool isRangedCombat = false;
            currentAction->getCombatRange(isRangedCombat);
            // Check if the actor now facing desired direction, no need to turn any more
            if (isRangedCombat)
            {
                if (smoothTurn(actor, targetAngleRadians, axis))
                    targetAngleRadians = 0;
            }
            else
            {
                if (smoothTurn(actor, targetAngleRadians, axis, osg::DegreesToRadians(3.f)))
                    targetAngleRadians = 0;
            }
        }
    }

    int AiCombat::getTypeId() const
    {
        return TypeIdCombat;
    }

    unsigned int AiCombat::getPriority() const
    {
        return 1;
    }

    MWWorld::Ptr AiCombat::getTarget() const
    {
        return MWBase::Environment::get().getWorld()->searchPtrViaActorId(mTargetActorId);
    }

    AiCombat *MWMechanics::AiCombat::clone() const
    {
        return new AiCombat(*this);
    }

    void AiCombat::writeState(ESM::AiSequence::AiSequence &sequence) const
    {
        std::unique_ptr<ESM::AiSequence::AiCombat> combat(new ESM::AiSequence::AiCombat());
        combat->mTargetActorId = mTargetActorId;

        ESM::AiSequence::AiPackageContainer package;
        package.mType = ESM::AiSequence::Ai_Combat;
        package.mPackage = combat.release();
        sequence.mPackages.push_back(package);
    }

    void AiCombatStorage::startCombatMove(bool isDistantCombat, float distToTarget, float rangeAttack, const MWWorld::Ptr& actor, const MWWorld::Ptr& target)
    {
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
        
        if (mMovement.mPosition[0] || mMovement.mPosition[1])
        {
            mTimerCombatMove = 0.1f + 0.1f * Misc::Rng::rollClosedProbability();
            mCombatMove = true;
        }
        else if (isDistantCombat)
        {
            // Backing up behaviour
            // Actor backs up slightly further away than opponent's weapon range
            // (in vanilla - only as far as oponent's weapon range),
            // or not at all if opponent is using a ranged weapon

            if (targetUsesRanged || distToTarget > rangeAttackOfTarget*1.5) // Don't back up if the target is wielding ranged weapon
                return;

            // actor should not back up into water
            if (MWBase::Environment::get().getWorld()->isUnderwater(MWWorld::ConstPtr(actor), 0.5f))
                return;

            int mask = MWPhysics::CollisionType_World | MWPhysics::CollisionType_HeightMap | MWPhysics::CollisionType_Door;

            // Actor can not back up if there is no free space behind
            // Currently we take the 35% of actor's height from the ground as vector height.
            // This approach allows us to detect small obstacles (e.g. crates) and curved walls.
            osg::Vec3f halfExtents = MWBase::Environment::get().getWorld()->getHalfExtents(actor);
            osg::Vec3f pos = actor.getRefData().getPosition().asVec3();
            osg::Vec3f source = pos + osg::Vec3f(0, 0, 0.75f * halfExtents.z());
            osg::Vec3f fallbackDirection = actor.getRefData().getBaseNode()->getAttitude() * osg::Vec3f(0,-1,0);
            osg::Vec3f destination = source + fallbackDirection * (halfExtents.y() + 16);

            bool isObstacleDetected = MWBase::Environment::get().getWorld()->castRay(source.x(), source.y(), source.z(), destination.x(), destination.y(), destination.z(), mask);
            if (isObstacleDetected)
                return;

            // Check if there is nothing behind - probably actor is near cliff.
            // A current approach: cast ray 1.5-yard ray down in 1.5 yard behind actor from 35% of actor's height.
            // If we did not hit anything, there is a cliff behind actor.
            source = pos + osg::Vec3f(0, 0, 0.75f * halfExtents.z()) + fallbackDirection * (halfExtents.y() + 96);
            destination = source - osg::Vec3f(0, 0, 0.75f * halfExtents.z() + 96);
            bool isCliffDetected = !MWBase::Environment::get().getWorld()->castRay(source.x(), source.y(), source.z(), destination.x(), destination.y(), destination.z(), mask);
            if (isCliffDetected)
                return;

            mMovement.mPosition[1] = -1;
        }
        // dodge movements (for NPCs and bipedal creatures)
        // Note: do not use for ranged combat yet since in couple with back up behaviour can move actor out of cliff
        else if (actor.getClass().isBipedal(actor))
        {
            // apply sideway movement (kind of dodging) with some probability
            // if actor is within range of target's weapon
            if (distToTarget <= rangeAttackOfTarget && Misc::Rng::rollClosedProbability() < 0.25)
            {
                mMovement.mPosition[0] = Misc::Rng::rollProbability() < 0.5 ? 1.0f : -1.0f; // to the left/right
                mTimerCombatMove = 0.1f + 0.1f * Misc::Rng::rollClosedProbability();
                mCombatMove = true;
            }
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
        mMovement.mPosition[1] = mMovement.mPosition[0] = 0;
        mCombatMove = false;
    }

    void AiCombatStorage::startAttackIfReady(const MWWorld::Ptr& actor, CharacterController& characterController, 
        const ESM::Weapon* weapon, bool distantCombat)
    {
        if (mReadyToAttack && characterController.readyToStartAttack())
        {
            if (mAttackCooldown <= 0)
            {
                mAttack = true; // attack starts just now
                characterController.setAttackingOrSpell(true);

                if (!distantCombat)
                    characterController.setAIAttackType(chooseBestAttack(weapon));

                mStrength = Misc::Rng::rollClosedProbability();

                const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();

                float baseDelay = store.get<ESM::GameSetting>().find("fCombatDelayCreature")->mValue.getFloat();
                if (actor.getClass().isNpc())
                {
                    baseDelay = store.get<ESM::GameSetting>().find("fCombatDelayNPC")->mValue.getFloat();
                }

                // Say a provoking combat phrase
                const int iVoiceAttackOdds = store.get<ESM::GameSetting>().find("iVoiceAttackOdds")->mValue.getInteger();
                if (Misc::Rng::roll0to99() < iVoiceAttackOdds)
                {
                    MWBase::Environment::get().getDialogueManager()->say(actor, "attack");
                }
                mAttackCooldown = std::min(baseDelay + 0.01 * Misc::Rng::roll0to99(), baseDelay + 0.9);
            }
            else
                mAttackCooldown -= AI_REACTION_TIME;
        }
    }

    void AiCombatStorage::updateAttack(CharacterController& characterController)
    {
        if (mAttack && (characterController.getAttackStrength() >= mStrength || characterController.readyToPrepareAttack()))
        {
            mAttack = false;
        }
        characterController.setAttackingOrSpell(mAttack);
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

    bool AiCombatStorage::isFleeing()
    {
        return mFleeState != FleeState_None;
    }
}


namespace
{

std::string chooseBestAttack(const ESM::Weapon* weapon)
{
    std::string attackType;

    if (weapon != nullptr)
    {
        //the more damage attackType deals the more probability it has
        int slash = (weapon->mData.mSlash[0] + weapon->mData.mSlash[1])/2;
        int chop = (weapon->mData.mChop[0] + weapon->mData.mChop[1])/2;
        int thrust = (weapon->mData.mThrust[0] + weapon->mData.mThrust[1])/2;

        float roll = Misc::Rng::rollClosedProbability() * (slash + chop + thrust);
        if(roll <= slash)
            attackType = "slash";
        else if(roll <= (slash + thrust))
            attackType = "thrust";
        else
            attackType = "chop";
    }
    else
        MWMechanics::CharacterController::setAttackTypeRandomly(attackType);

    return attackType;
}

osg::Vec3f AimDirToMovingTarget(const MWWorld::Ptr& actor, const MWWorld::Ptr& target, const osg::Vec3f& vLastTargetPos,
    float duration, int weapType, float strength)
{
    float projSpeed;
    const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

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

    // idea: perpendicular to dir to target speed components of target move vector and projectile vector should be the same

    osg::Vec3f vTargetPos = target.getRefData().getPosition().asVec3();
    osg::Vec3f vDirToTarget = MWBase::Environment::get().getWorld()->aimToTarget(actor, target);
    float distToTarget = vDirToTarget.length();

    osg::Vec3f vTargetMoveDir = vTargetPos - vLastTargetPos;
    vTargetMoveDir /= duration; // |vTargetMoveDir| is target real speed in units/sec now

    osg::Vec3f vPerpToDir = vDirToTarget ^ osg::Vec3f(0,0,1); // cross product

    vPerpToDir.normalize();
    osg::Vec3f vDirToTargetNormalized = vDirToTarget;
    vDirToTargetNormalized.normalize();

    // dot product
    float velPerp = vTargetMoveDir * vPerpToDir;
    float velDir = vTargetMoveDir * vDirToTargetNormalized;

    // time to collision between target and projectile
    float t_collision;

    float projVelDirSquared = projSpeed * projSpeed - velPerp * velPerp;

    osg::Vec3f vTargetMoveDirNormalized = vTargetMoveDir;
    vTargetMoveDirNormalized.normalize();

    float projDistDiff = vDirToTarget * vTargetMoveDirNormalized; // dot product
    projDistDiff = std::sqrt(distToTarget * distToTarget - projDistDiff * projDistDiff);

    if (projVelDirSquared > 0)
        t_collision = projDistDiff / (std::sqrt(projVelDirSquared) - velDir);
    else t_collision = 0; // speed of projectile is not enough to reach moving target

    return vDirToTarget + vTargetMoveDir * t_collision;
}

}
