#include "aicombat.hpp"

#include <components/misc/rng.hpp>

#include <components/esm/aisequence.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/dialoguemanager.hpp"

#include "../mwrender/animation.hpp"

#include "creaturestats.hpp"
#include "steering.hpp"
#include "movement.hpp"
#include "character.hpp"

#include "aicombataction.hpp"
#include "combat.hpp"

namespace
{

    //chooses an attack depending on probability to avoid uniformity
    ESM::Weapon::AttackType chooseBestAttack(const ESM::Weapon* weapon, MWMechanics::Movement &movement);

    osg::Vec3f AimDirToMovingTarget(const MWWorld::Ptr& actor, const MWWorld::Ptr& target, const osg::Vec3f& vLastTargetPos,
        float duration, int weapType, float strength);

    float getZAngleToDir(const osg::Vec3f& dir)
    {
        return std::atan2(dir.x(), dir.y());
    }

    float getXAngleToDir(const osg::Vec3f& dir, float dirLen = 0.0f)
    {
        float len = (dirLen > 0.0f)? dirLen : dir.length();
        return -std::asin(dir.z() / len);
    }

    const float REACTION_INTERVAL = 0.25f;

    const float PATHFIND_Z_REACH = 50.0f;
    // distance at which actor pays more attention to decide whether to shortcut or stick to pathgrid
    const float PATHFIND_CAUTION_DIST = 500.0f;
    // distance after which actor (failed previously to shortcut) will try again
    const float PATHFIND_SHORTCUT_RETRY_DIST = 300.0f;

    // cast up-down ray with some offset from actor position to check for pits/obstacles on the way to target;
    // magnitude of pits/obstacles is defined by PATHFIND_Z_REACH
    bool checkWayIsClear(const osg::Vec3f& from, const osg::Vec3f& to, float offsetXY)
    {
        if((to - from).length() >= PATHFIND_CAUTION_DIST || std::abs(from.z() - to.z()) <= PATHFIND_Z_REACH)
        {
            osg::Vec3f dir = to - from;
            dir.z() = 0;
            dir.normalize();
			float verticalOffset = 200; // instead of '200' here we want the height of the actor
            osg::Vec3f _from = from + dir*offsetXY + osg::Vec3f(0,0,1) * verticalOffset;

            // cast up-down ray and find height in world space of hit
            float h = _from.z() - MWBase::Environment::get().getWorld()->getDistToNearestRayHit(_from, osg::Vec3f(0,0,-1), verticalOffset + PATHFIND_Z_REACH + 1);

            if(std::abs(from.z() - h) <= PATHFIND_Z_REACH)
                return true;
        }

        return false;
    }
}

namespace MWMechanics
{
    
    /// \brief This class holds the variables AiCombat needs which are deleted if the package becomes inactive.
    struct AiCombatStorage : AiTemporaryBase
    {
        float mAttackCooldown;
        float mTimerReact;
        float mTimerCombatMove;
        bool mReadyToAttack;
        bool mAttack;
        bool mFollowTarget;
        bool mCombatMove;
        osg::Vec3f mLastTargetPos;
        const MWWorld::CellStore* mCell;
        boost::shared_ptr<Action> mCurrentAction;
        float mActionCooldown;
        float mStrength;
        bool mForceNoShortcut;
        ESM::Position mShortcutFailPos;
        osg::Vec3f mLastActorPos;
        MWMechanics::Movement mMovement;
        
        AiCombatStorage():
        mAttackCooldown(0),
        mTimerReact(0),
        mTimerCombatMove(0),
        mReadyToAttack(false),
        mAttack(false),
        mFollowTarget(false),
        mCombatMove(false),
        mLastTargetPos(0,0,0),
        mCell(NULL),
        mCurrentAction(),
        mActionCooldown(0),
        mStrength(),
        mForceNoShortcut(false),
        mLastActorPos(0,0,0),
        mMovement(){}    
    };
    
    AiCombat::AiCombat(const MWWorld::Ptr& actor) :
        mTargetActorId(actor.getClass().getCreatureStats(actor).getActorId())
    {}

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
     * Use the Observer Pattern to co-ordinate attacks, provide intelligence on
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


        //Update every frame
        bool& combatMove = storage.mCombatMove;
        float& timerCombatMove = storage.mTimerCombatMove; 
        MWMechanics::Movement& movement = storage.mMovement;
        if(combatMove)
        {
            timerCombatMove -= duration;
            if( timerCombatMove <= 0)
            {
                timerCombatMove = 0;
                movement.mPosition[1] = movement.mPosition[0] = 0;
                combatMove = false;
            }
        }

        UpdateActorsMovement(actor, movement);

        bool& attack = storage.mAttack;
        if (attack && (characterController.getAttackStrength() >= storage.mStrength || characterController.readyToPrepareAttack()))
            attack = false;

        characterController.setAttackingOrSpell(attack);

        float& actionCooldown = storage.mActionCooldown;
        actionCooldown -= duration;
        
        float& timerReact = storage.mTimerReact;
        if(timerReact < REACTION_INTERVAL)
        {
            timerReact += duration;
            return false;
        }
        else
        {
            timerReact = 0;
            return reactionTimeActions(actor, characterController, storage, target);
        }
    }

    bool AiCombat::reactionTimeActions(const MWWorld::Ptr& actor, CharacterController& characterController, 
        AiCombatStorage& storage, MWWorld::Ptr target)
    {
        MWMechanics::Movement& movement = storage.mMovement;

        // Stop attacking if target is not seen
        if (target.getClass().getCreatureStats(target).getMagicEffects().get(ESM::MagicEffect::Invisibility).getMagnitude() > 0
                || target.getClass().getCreatureStats(target).getMagicEffects().get(ESM::MagicEffect::Chameleon).getMagnitude() > 75)
        {
            movement.mPosition[1] = movement.mPosition[0] = 0;
            return false; // TODO: run away instead of doing nothing
        }

        const MWWorld::CellStore*& currentCell = storage.mCell;
        bool cellChange = currentCell && (actor.getCell() != currentCell);
        if(!currentCell || cellChange)
        {
            currentCell = actor.getCell();
        }

        const MWWorld::Class& actorClass = actor.getClass();
        actorClass.getCreatureStats(actor).setMovementFlag(CreatureStats::Flag_Run, true);

        float& actionCooldown = storage.mActionCooldown;
        if (actionCooldown > 0)
            return false;

        float rangeAttack = 0;
        float rangeFollow = 0;
        boost::shared_ptr<Action>& currentAction = storage.mCurrentAction;
        if (characterController.readyToPrepareAttack())
        {
            currentAction = prepareNextAction(actor, target);
            actionCooldown = currentAction->getActionCooldown();
        }

        if (currentAction.get())
            currentAction->getCombatRange(rangeAttack, rangeFollow);

        // FIXME: consider moving this stuff to ActionWeapon::getCombatRange
        const ESM::Weapon *weapon = NULL;
        MWMechanics::WeaponType weaptype = WeapType_None;
        float weapRange = 1.0f;

        // Get weapon characteristics
        MWBase::World* world = MWBase::Environment::get().getWorld();
        if (actorClass.hasInventoryStore(actor))
        {
            //Get weapon range
            MWWorld::ContainerStoreIterator weaponSlot =
                MWMechanics::getActiveWeapon(actorClass.getCreatureStats(actor), actorClass.getInventoryStore(actor), &weaptype);

            if (weaptype == WeapType_HandToHand)
            {
                static float fHandToHandReach =
                    world->getStore().get<ESM::GameSetting>().find("fHandToHandReach")->getFloat();
                weapRange = fHandToHandReach;
            }
            else if (weaptype != WeapType_PickProbe && weaptype != WeapType_Spell && weaptype != WeapType_None)
            {
                // All other WeapTypes are actually weapons, so get<ESM::Weapon> is safe.
                weapon = weaponSlot->get<ESM::Weapon>()->mBase;
                weapRange = weapon->mData.mReach;
            }
            weapRange *= 100.0f;
        }
        else //is creature
        {
            weaptype = actorClass.getCreatureStats(actor).getDrawState() == DrawState_Spell ? WeapType_Spell : WeapType_HandToHand;
            weapRange = 150.0f; //TODO: use true attack range (the same problem in Creature::hit)
        }

        bool distantCombat = false;
        if (weaptype != WeapType_Spell)
        {
            // TODO: move to ActionWeapon
            if (weaptype == WeapType_BowAndArrow || weaptype == WeapType_Crossbow || weaptype == WeapType_Thrown)
            {
                rangeAttack = 1000;
                rangeFollow = 0; // not needed in ranged combat
                distantCombat = true;
            }
            else
            {
                rangeAttack = weapRange;
                rangeFollow = 300;
            }
        }
        else
        {
            distantCombat = (rangeAttack > 500);
            weapRange = 150.f;
        }

        
        float& strength = storage.mStrength;
        bool& readyToAttack = storage.mReadyToAttack;
        // start new attack
        if(readyToAttack && characterController.readyToStartAttack())
        {
            if (storage.mAttackCooldown <= 0)
            {
                storage.mAttack = true; // attack starts just now
                characterController.setAttackingOrSpell(true);

                if (!distantCombat)
                    chooseBestAttack(weapon, movement);

                strength = Misc::Rng::rollClosedProbability();

                const MWWorld::ESMStore &store = world->getStore();

                //say a provoking combat phrase
                if (actor.getClass().isNpc())
                {
                    int chance = store.get<ESM::GameSetting>().find("iVoiceAttackOdds")->getInt();
                    if (Misc::Rng::roll0to99() < chance)
                    {
                        MWBase::Environment::get().getDialogueManager()->say(actor, "attack");
                    }
                }
                float baseDelay = store.get<ESM::GameSetting>().find("fCombatDelayCreature")->getFloat();
                if (actor.getClass().isNpc())
                    baseDelay = store.get<ESM::GameSetting>().find("fCombatDelayNPC")->getFloat();
                storage.mAttackCooldown = std::min(baseDelay + 0.01 * Misc::Rng::roll0to99(), baseDelay + 0.9);
            }
            else
                storage.mAttackCooldown -= REACTION_INTERVAL;
        }


        /*
         * Some notes on meanings of variables:
         *
         * rangeAttack:
         *
         *  - Distance where attack using the actor's weapon is possible:
         *    longer for ranged weapons (obviously?) vs. melee weapons
         *  - Determined by weapon's reach parameter; hardcoded value
         *    for ranged weapon and for creatures
         *  - Once within this distance mFollowTarget is triggered
         *
         * rangeFollow:
         *
         *  - Applies to melee weapons or hand to hand only (or creatures without
         *    weapons)
         *  - Distance a little further away than the actor's weapon reach
         *    i.e. rangeFollow > rangeAttack for melee weapons
         *  - Hardcoded value (0 for ranged weapons)
         *  - Once the target gets beyond this distance mFollowTarget is cleared
         *    and a path to the target needs to be found
         *
         * mFollowTarget:
         *
         *  - Once triggered, the actor follows the target with LOS shortcut
         *    (the shortcut really only applies to cells where pathgrids are
         *    available, since the default path without pathgrids is direct to
         *    target even if LOS is not achieved)
         */

        ESM::Position pos = actor.getRefData().getPosition();
        osg::Vec3f vActorPos(pos.asVec3());
        osg::Vec3f vTargetPos(target.getRefData().getPosition().asVec3());
        osg::Vec3f vDirToTarget = vTargetPos - vActorPos;
        float distToTarget = vDirToTarget.length();
        
        osg::Vec3f& lastActorPos = storage.mLastActorPos;
        bool& followTarget = storage.mFollowTarget;

        bool isStuck = false;
        float speed = 0.0f;
        if(movement.mPosition[1] && (lastActorPos - vActorPos).length() < (speed = actorClass.getSpeed(actor)) * REACTION_INTERVAL / 2)
            isStuck = true;

        lastActorPos = vActorPos;

        // check if actor can move along z-axis
        bool canMoveByZ = (actorClass.canSwim(actor) && world->isSwimming(actor))
            || world->isFlying(actor);

        // for distant combat we should know if target is in LOS even if distToTarget < rangeAttack 
        bool inLOS = distantCombat ? world->getLOS(actor, target) : true;

        // can't fight if attacker can't go where target is.  E.g. A fish can't attack person on land.
        if (distToTarget >= rangeAttack
                && !actorClass.isNpc() && !MWMechanics::isEnvironmentCompatible(actor, target))
        {
            // TODO: start fleeing?
            movement.mPosition[0] = 0;
            movement.mPosition[1] = 0;
            movement.mPosition[2] = 0;
            readyToAttack = false;
            characterController.setAttackingOrSpell(false);
            return false;
        }

        // (within attack dist) || (not quite attack dist while following)
        if(inLOS && (distToTarget < rangeAttack || (distToTarget <= rangeFollow && followTarget && !isStuck)))
        {
            //Melee and Close-up combat
            
            // getXAngleToDir determines vertical angle to target:
            // if actor can move along z-axis it will control movement dir
            // if can't - it will control correct aiming.
            // note: in getZAngleToDir if we preserve dir.z then horizontal angle can be inaccurate
            if (distantCombat)
            {
                osg::Vec3f& lastTargetPos = storage.mLastTargetPos;
                osg::Vec3f vAimDir = AimDirToMovingTarget(actor, target, lastTargetPos, REACTION_INTERVAL, weaptype, strength);
                lastTargetPos = vTargetPos;
                movement.mRotation[0] = getXAngleToDir(vAimDir);
                movement.mRotation[2] = getZAngleToDir(vAimDir);
            }
            else
            {
                movement.mRotation[0] = getXAngleToDir(vDirToTarget, distToTarget);
                movement.mRotation[2] = getZAngleToDir(vDirToTarget);
            }

            // (not quite attack dist while following)
            if (followTarget && distToTarget > rangeAttack)
            {
                //Close-up combat: just run up on target
                movement.mPosition[1] = 1;
            }
            else // (within attack dist)
            {
                if(movement.mPosition[0] || movement.mPosition[1])
                {
                    storage.mTimerCombatMove = 0.1f + 0.1f * Misc::Rng::rollClosedProbability();
                    storage.mCombatMove = true;
                }
                // only NPCs are smart enough to use dodge movements
                else if(actorClass.isNpc() && (!distantCombat || (distantCombat && distToTarget < rangeAttack/2)))
                {
                    //apply sideway movement (kind of dodging) with some probability
                    if (Misc::Rng::rollClosedProbability() < 0.25)
                    {
                        movement.mPosition[0] = Misc::Rng::rollProbability() < 0.5 ? 1.0f : -1.0f;
                        storage.mTimerCombatMove = 0.05f + 0.15f * Misc::Rng::rollClosedProbability();
                        storage.mCombatMove = true;
                    }
                }

                if(distantCombat && distToTarget < rangeAttack/4)
                {
                    movement.mPosition[1] = -1;
                }

                readyToAttack = true;
                //only once got in melee combat, actor is allowed to use close-up shortcutting
                followTarget = true;
            }
        }
        else // remote pathfinding
        {
            bool preferShortcut = false;
            if (!distantCombat) inLOS = world->getLOS(actor, target);

            // check if shortcut is available
            bool& forceNoShortcut = storage.mForceNoShortcut;
            ESM::Position& shortcutFailPos = storage.mShortcutFailPos;
            
            if(inLOS && (!isStuck || readyToAttack)
                && (!forceNoShortcut || (shortcutFailPos.asVec3() - vActorPos).length() >= PATHFIND_SHORTCUT_RETRY_DIST))
            {
                if(speed == 0.0f) speed = actorClass.getSpeed(actor);
                // maximum dist before pit/obstacle for actor to avoid them depending on his speed
                float maxAvoidDist = REACTION_INTERVAL * speed + speed / MAX_VEL_ANGULAR_RADIANS * 2; // *2 - for reliability
                preferShortcut = checkWayIsClear(vActorPos, vTargetPos, osg::Vec3f(vDirToTarget.x(), vDirToTarget.y(), 0).length() > maxAvoidDist*1.5? maxAvoidDist : maxAvoidDist/2);
            }

            // don't use pathgrid when actor can move in 3 dimensions
            if (canMoveByZ)
            {
                preferShortcut = true;
                movement.mRotation[0] = getXAngleToDir(vDirToTarget, distToTarget);
            }

            if(preferShortcut)
            {
                movement.mRotation[2] = getZAngleToDir(vDirToTarget);
                forceNoShortcut = false;
                shortcutFailPos.pos[0] = shortcutFailPos.pos[1] = shortcutFailPos.pos[2] = 0;
                mPathFinder.clearPath();
            }
            else // if shortcut failed stick to path grid
            {
                if(!isStuck && shortcutFailPos.pos[0] == 0.0f && shortcutFailPos.pos[1] == 0.0f && shortcutFailPos.pos[2] == 0.0f)
                {
                    forceNoShortcut = true;
                    shortcutFailPos = pos;
                }

                followTarget = false;

                buildNewPath(actor, target); //may fail to build a path, check before use

                //delete visited path node
                mPathFinder.checkPathCompleted(pos.pos[0],pos.pos[1]);

                // This works on the borders between the path grid and areas with no waypoints.
                if(inLOS && mPathFinder.getPath().size() > 1)
                {
                    // get point just before target
                    std::list<ESM::Pathgrid::Point>::const_iterator pntIter = --mPathFinder.getPath().end();
                    --pntIter;
                    osg::Vec3f vBeforeTarget(PathFinder::MakeOsgVec3(*pntIter));

                    // if current actor pos is closer to target then last point of path (excluding target itself) then go straight on target
                    if(distToTarget <= (vTargetPos - vBeforeTarget).length())
                    {
                        movement.mRotation[2] = getZAngleToDir(vDirToTarget);
                        preferShortcut = true;
                    }
                }

                // if there is no new path, then go straight on target
                if(!preferShortcut)
                {
                    if(!mPathFinder.getPath().empty())
                        movement.mRotation[2] = mPathFinder.getZAngleToNext(pos.pos[0], pos.pos[1]);
                    else
                        movement.mRotation[2] = getZAngleToDir(vDirToTarget);
                }
            }

            movement.mPosition[1] = 1;
            if (readyToAttack)
            {
                // to stop possible sideway moving after target moved out of attack range
                storage.mCombatMove = true;
                storage.mTimerCombatMove = 0;
            }
            readyToAttack = false;
        }

        return false;
    }

    void AiCombat::UpdateActorsMovement(const MWWorld::Ptr& actor, MWMechanics::Movement& desiredMovement)
    {
        MWMechanics::Movement& actorMovementSettings = actor.getClass().getMovementSettings(actor);
        actorMovementSettings = desiredMovement;
        RotateActorOnAxis(actor, 2, actorMovementSettings, desiredMovement);
        RotateActorOnAxis(actor, 0, actorMovementSettings, desiredMovement);
    }

    void AiCombat::RotateActorOnAxis(const MWWorld::Ptr& actor, int axis, 
        MWMechanics::Movement& actorMovementSettings, MWMechanics::Movement& desiredMovement)
    {
        actorMovementSettings.mRotation[axis] = 0;
        float& targetAngleRadians = desiredMovement.mRotation[axis];
        if (targetAngleRadians != 0)
        {
            if (smoothTurn(actor, targetAngleRadians, axis))
            {
                // actor now facing desired direction, no need to turn any more
                targetAngleRadians = 0;
            }
        }
    }

    bool AiCombat::doesPathNeedRecalc(ESM::Pathgrid::Point dest, const ESM::Cell *cell)
    {
        if (!mPathFinder.getPath().empty())
        {
            osg::Vec3f currPathTarget(PathFinder::MakeOsgVec3(mPathFinder.getPath().back()));
            osg::Vec3f newPathTarget = PathFinder::MakeOsgVec3(dest);
            float dist = (newPathTarget - currPathTarget).length();
            float targetPosThreshold = (cell->isExterior()) ? 300.0f : 100.0f;
            return dist > targetPosThreshold;
        }
        else
        {
            // necessarily construct a new path
            return true;
        }
    }

    void AiCombat::buildNewPath(const MWWorld::Ptr& actor, const MWWorld::Ptr& target)
    {
        ESM::Pathgrid::Point newPathTarget = PathFinder::MakePathgridPoint(target.getRefData().getPosition());

        //construct new path only if target has moved away more than on [targetPosThreshold]
        if (doesPathNeedRecalc(newPathTarget, actor.getCell()->getCell()))
        {
            ESM::Pathgrid::Point start(PathFinder::MakePathgridPoint(actor.getRefData().getPosition()));
            mPathFinder.buildSyncedPath(start, newPathTarget, actor.getCell(), false);
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
        std::auto_ptr<ESM::AiSequence::AiCombat> combat(new ESM::AiSequence::AiCombat());
        combat->mTargetActorId = mTargetActorId;

        ESM::AiSequence::AiPackageContainer package;
        package.mType = ESM::AiSequence::Ai_Combat;
        package.mPackage = combat.release();
        sequence.mPackages.push_back(package);
    }
}


namespace
{

ESM::Weapon::AttackType chooseBestAttack(const ESM::Weapon* weapon, MWMechanics::Movement &movement)
{
    ESM::Weapon::AttackType attackType;

    if (weapon == NULL)
    {
        //hand-to-hand deal equal damage for each type
        float roll = Misc::Rng::rollClosedProbability();
        if(roll <= 0.333f)  //side punch
        {
            movement.mPosition[0] = Misc::Rng::rollClosedProbability() ? 1.0f : -1.0f;
            movement.mPosition[1] = 0;
            attackType = ESM::Weapon::AT_Slash;
        }
        else if(roll <= 0.666f) //forward punch
        {
            movement.mPosition[1] = 1;
            attackType = ESM::Weapon::AT_Thrust;
        }
        else
        {
            movement.mPosition[1] = movement.mPosition[0] = 0;
            attackType = ESM::Weapon::AT_Chop;
        }
    }
    else
    {
        //the more damage attackType deals the more probability it has
        int slash = (weapon->mData.mSlash[0] + weapon->mData.mSlash[1])/2;
        int chop = (weapon->mData.mChop[0] + weapon->mData.mChop[1])/2;
        int thrust = (weapon->mData.mThrust[0] + weapon->mData.mThrust[1])/2;

        float roll = Misc::Rng::rollClosedProbability() * (slash + chop + thrust);
        if(roll <= slash)
        {
            movement.mPosition[0] = (Misc::Rng::rollClosedProbability() < 0.5f) ? 1.0f : -1.0f;
            movement.mPosition[1] = 0;
            attackType = ESM::Weapon::AT_Slash;
        }
        else if(roll <= (slash + thrust))
        {
            movement.mPosition[1] = 1;
            attackType = ESM::Weapon::AT_Thrust;
        }
        else
        {
            movement.mPosition[1] = movement.mPosition[0] = 0;
            attackType = ESM::Weapon::AT_Chop;
        }
    }

    return attackType;
}

osg::Vec3f AimDirToMovingTarget(const MWWorld::Ptr& actor, const MWWorld::Ptr& target, const osg::Vec3f& vLastTargetPos,
    float duration, int weapType, float strength)
{
    float projSpeed;

    // get projectile speed (depending on weapon type)
    if (weapType == ESM::Weapon::MarksmanThrown)
    {
        static float fThrownWeaponMinSpeed = 
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fThrownWeaponMinSpeed")->getFloat();
        static float fThrownWeaponMaxSpeed = 
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fThrownWeaponMaxSpeed")->getFloat();

        projSpeed = 
            fThrownWeaponMinSpeed + (fThrownWeaponMaxSpeed - fThrownWeaponMinSpeed) * strength;
    }
    else
    {
        static float fProjectileMinSpeed = 
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fProjectileMinSpeed")->getFloat();
        static float fProjectileMaxSpeed = 
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fProjectileMaxSpeed")->getFloat();

        projSpeed = 
            fProjectileMinSpeed + (fProjectileMaxSpeed - fProjectileMinSpeed) * strength;
    }

    // idea: perpendicular to dir to target speed components of target move vector and projectile vector should be the same

    osg::Vec3f vActorPos = actor.getRefData().getPosition().asVec3();
    osg::Vec3f vTargetPos = target.getRefData().getPosition().asVec3();
    osg::Vec3f vDirToTarget = vTargetPos - vActorPos;
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

    return vTargetPos + vTargetMoveDir * t_collision - vActorPos;
}

}
