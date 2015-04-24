#include "aicombat.hpp"

#include <OgreMath.h>

#include <openengine/misc/rng.hpp>

#include <components/esm/aisequence.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/timestamp.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"

#include "../mwrender/animation.hpp"


#include "creaturestats.hpp"
#include "steering.hpp"
#include "movement.hpp"
#include "character.hpp" // fixme: for getActiveWeapon

#include "aicombataction.hpp"
#include "combat.hpp"

namespace
{

    //chooses an attack depending on probability to avoid uniformity
    ESM::Weapon::AttackType chooseBestAttack(const ESM::Weapon* weapon, MWMechanics::Movement &movement);

    void getMinMaxAttackDuration(const MWWorld::Ptr& actor, float (*fMinMaxDurations)[2]);

    Ogre::Vector3 AimDirToMovingTarget(const MWWorld::Ptr& actor, const MWWorld::Ptr& target, const Ogre::Vector3& vLastTargetPos, 
        float duration, int weapType, float strength);

    float getZAngleToDir(const Ogre::Vector3& dir)
    {
        return Ogre::Math::ATan2(dir.x,dir.y).valueDegrees();
    }

    float getXAngleToDir(const Ogre::Vector3& dir, float dirLen = 0.0f)
    {
        float len = (dirLen > 0.0f)? dirLen : dir.length();
        return -Ogre::Math::ASin(dir.z / len).valueDegrees();
    }


    const float PATHFIND_Z_REACH = 50.0f;
    // distance at which actor pays more attention to decide whether to shortcut or stick to pathgrid
    const float PATHFIND_CAUTION_DIST = 500.0f;
    // distance after which actor (failed previously to shortcut) will try again
    const float PATHFIND_SHORTCUT_RETRY_DIST = 300.0f;

    // cast up-down ray with some offset from actor position to check for pits/obstacles on the way to target;
    // magnitude of pits/obstacles is defined by PATHFIND_Z_REACH
    bool checkWayIsClear(const Ogre::Vector3& from, const Ogre::Vector3& to, float offsetXY)
    {
        if((to - from).length() >= PATHFIND_CAUTION_DIST || std::abs(from.z - to.z) <= PATHFIND_Z_REACH)
        {
            Ogre::Vector3 dir = to - from;
            dir.z = 0;
            dir.normalise();
			float verticalOffset = 200; // instead of '200' here we want the height of the actor
            Ogre::Vector3 _from = from + dir*offsetXY + Ogre::Vector3::UNIT_Z * verticalOffset;

            // cast up-down ray and find height in world space of hit
            float h = _from.z - MWBase::Environment::get().getWorld()->getDistToNearestRayHit(_from, -Ogre::Vector3::UNIT_Z, verticalOffset + PATHFIND_Z_REACH + 1);

            if(std::abs(from.z - h) <= PATHFIND_Z_REACH)
                return true;
        }

        return false;
    }
}

namespace MWMechanics
{
    static const float DOOR_CHECK_INTERVAL = 1.5f; // same as AiWander
    // NOTE: MIN_DIST_TO_DOOR_SQUARED is defined in obstacle.hpp

    
    /// \brief This class holds the variables AiCombat needs which are deleted if the package becomes inactive.
    struct AiCombatStorage : AiTemporaryBase
    {
        float mTimerAttack;
        float mTimerReact;
        float mTimerCombatMove;
        bool mReadyToAttack;
        bool mAttack;
        bool mFollowTarget;
        bool mCombatMove;
        Ogre::Vector3 mLastTargetPos;
        const MWWorld::CellStore* mCell;
        boost::shared_ptr<Action> mCurrentAction;
        float mActionCooldown;
        float mStrength;
        float mMinMaxAttackDuration[3][2];
        bool mMinMaxAttackDurationInitialised;
        bool mForceNoShortcut;
        ESM::Position mShortcutFailPos;
        Ogre::Vector3 mLastActorPos;
        MWMechanics::Movement mMovement;
        
        AiCombatStorage():
        mTimerAttack(0),
        mTimerReact(0),
        mTimerCombatMove(0),
        mAttack(false),
        mFollowTarget(false),
        mCombatMove(false),
        mReadyToAttack(false),
        mForceNoShortcut(false),
        mCell(NULL),
        mCurrentAction(),
        mActionCooldown(0),
        mStrength(),
        mMinMaxAttackDurationInitialised(false),
        mLastTargetPos(0,0,0),
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
    bool AiCombat::execute (const MWWorld::Ptr& actor, AiState& state, float duration)
    {
        // get or create temporary storage
        AiCombatStorage& storage = state.get<AiCombatStorage>();
        

        
        //General description
        if(actor.getClass().getCreatureStats(actor).isDead())
            return true;

        MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtrViaActorId(mTargetActorId);
        if (target.isEmpty())
            return false;

        if(!target.getRefData().getCount() || !target.getRefData().isEnabled()  // Really we should be checking whether the target is currently registered
                                                                                // with the MechanicsManager
                || target.getClass().getCreatureStats(target).isDead())
            return true;

        const MWWorld::Class& actorClass = actor.getClass();
        MWBase::World* world = MWBase::Environment::get().getWorld();


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

        actorClass.getMovementSettings(actor) = movement;
        actorClass.getMovementSettings(actor).mRotation[0] = 0;
        actorClass.getMovementSettings(actor).mRotation[2] = 0;

        if(movement.mRotation[2] != 0)
        {
            if(zTurn(actor, Ogre::Degree(movement.mRotation[2]))) movement.mRotation[2] = 0;
        }

        if(movement.mRotation[0] != 0)
        {
            if(smoothTurn(actor, Ogre::Degree(movement.mRotation[0]), 0)) movement.mRotation[0] = 0;
        }

        //TODO: Some skills affect period of strikes.For berserk-like style period ~ 0.25f
        float attacksPeriod = 1.0f;

        ESM::Weapon::AttackType attackType;


        
        
        bool& attack = storage.mAttack;
        bool& readyToAttack = storage.mReadyToAttack;
        float& timerAttack = storage.mTimerAttack;
        
        bool& minMaxAttackDurationInitialised = storage.mMinMaxAttackDurationInitialised;
        float (&minMaxAttackDuration)[3][2] = storage.mMinMaxAttackDuration;
        
        if(readyToAttack)
        {
            if (!minMaxAttackDurationInitialised)
            {
                // TODO: this must be updated when a different weapon is equipped
                getMinMaxAttackDuration(actor, minMaxAttackDuration);
                minMaxAttackDurationInitialised = true;
            }

            if (timerAttack < 0) attack = false;

            timerAttack -= duration;
        }
        else
        {
            timerAttack = -attacksPeriod;
            attack = false;
        }

        actorClass.getCreatureStats(actor).setAttackingOrSpell(attack);


        float& actionCooldown = storage.mActionCooldown;
        actionCooldown -= duration;
        
        float& timerReact = storage.mTimerReact;
        float tReaction = 0.25f;
        if(timerReact < tReaction)
        {
            timerReact += duration;
            return false;
        }

        //Update with period = tReaction

        // Stop attacking if target is not seen
        if (target.getClass().getCreatureStats(target).getMagicEffects().get(ESM::MagicEffect::Invisibility).getMagnitude() > 0
                || target.getClass().getCreatureStats(target).getMagicEffects().get(ESM::MagicEffect::Chameleon).getMagnitude() > 75)
        {
            movement.mPosition[1] = movement.mPosition[0] = 0;
            return false; // TODO: run away instead of doing nothing
        }

        timerReact = 0;
        const MWWorld::CellStore*& currentCell = storage.mCell;
        bool cellChange = currentCell && (actor.getCell() != currentCell);
        if(!currentCell || cellChange)
        {
            currentCell = actor.getCell();
        }

        MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(actor);
        if (!anim) // shouldn't happen
            return false;

        actorClass.getCreatureStats(actor).setMovementFlag(CreatureStats::Flag_Run, true);

        if (actionCooldown > 0)
            return false;

        float rangeAttack = 0;
        float rangeFollow = 0;
        boost::shared_ptr<Action>& currentAction = storage.mCurrentAction;
        if (anim->upperBodyReady())
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
        if (actorClass.hasInventoryStore(actor))
        {
            // TODO: Check equipped weapon and equip a different one if we can't attack with it
            // (e.g. no ammunition, or wrong type of ammunition equipped, etc. autoEquip is not very smart in this regard))

            //Get weapon speed and range
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
        // start new attack
        if(readyToAttack)
        {
            if(timerAttack <= -attacksPeriod)
            {
                attack = true; // attack starts just now

                if (!distantCombat) attackType = chooseBestAttack(weapon, movement);
                else attackType = ESM::Weapon::AT_Chop; // cause it's =0

                strength = OEngine::Misc::Rng::rollClosedProbability();

                // Note: may be 0 for some animations
                timerAttack = minMaxAttackDuration[attackType][0] + 
                    (minMaxAttackDuration[attackType][1] - minMaxAttackDuration[attackType][0]) * strength;

                //say a provoking combat phrase
                if (actor.getClass().isNpc())
                {
                    const MWWorld::ESMStore &store = world->getStore();
                    int chance = store.get<ESM::GameSetting>().find("iVoiceAttackOdds")->getInt();
                    if (OEngine::Misc::Rng::roll0to99() < chance)
                    {
                        MWBase::Environment::get().getDialogueManager()->say(actor, "attack");
                    }
                }
            }
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
        Ogre::Vector3 vActorPos(pos.pos);
        Ogre::Vector3 vTargetPos(target.getRefData().getPosition().pos);
        Ogre::Vector3 vDirToTarget = vTargetPos - vActorPos;
        float distToTarget = vDirToTarget.length();
        
        Ogre::Vector3& lastActorPos = storage.mLastActorPos;
        bool& followTarget = storage.mFollowTarget;

        bool isStuck = false;
        float speed = 0.0f;
        if(movement.mPosition[1] && (lastActorPos - vActorPos).length() < (speed = actorClass.getSpeed(actor)) * tReaction / 2)
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
            actorClass.getCreatureStats(actor).setAttackingOrSpell(false);
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
                Ogre::Vector3& lastTargetPos = storage.mLastTargetPos;
                Ogre::Vector3 vAimDir = AimDirToMovingTarget(actor, target, lastTargetPos, tReaction, weaptype, strength);
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
                    timerCombatMove = 0.1f + 0.1f * OEngine::Misc::Rng::rollClosedProbability();
                    combatMove = true;
                }
                // only NPCs are smart enough to use dodge movements
                else if(actorClass.isNpc() && (!distantCombat || (distantCombat && distToTarget < rangeAttack/2)))
                {
                    //apply sideway movement (kind of dodging) with some probability
                    if (OEngine::Misc::Rng::rollClosedProbability() < 0.25)
                    {
                        movement.mPosition[0] = OEngine::Misc::Rng::rollProbability() < 0.5 ? 1.0f : -1.0f;
                        timerCombatMove = 0.05f + 0.15f * OEngine::Misc::Rng::rollClosedProbability();
                        combatMove = true;
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
                && (!forceNoShortcut || (Ogre::Vector3(shortcutFailPos.pos) - vActorPos).length() >= PATHFIND_SHORTCUT_RETRY_DIST))
            {
                if(speed == 0.0f) speed = actorClass.getSpeed(actor);
                // maximum dist before pit/obstacle for actor to avoid them depending on his speed
                float maxAvoidDist = tReaction * speed + speed / MAX_VEL_ANGULAR.valueRadians() * 2; // *2 - for reliability
				preferShortcut = checkWayIsClear(vActorPos, vTargetPos, Ogre::Vector3(vDirToTarget.x, vDirToTarget.y, 0).length() > maxAvoidDist*1.5? maxAvoidDist : maxAvoidDist/2);
            }

            // don't use pathgrid when actor can move in 3 dimensions
            if(canMoveByZ) preferShortcut = true;

            if(preferShortcut)
            {
                if (canMoveByZ)
                    movement.mRotation[0] = getXAngleToDir(vDirToTarget, distToTarget);
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
                    Ogre::Vector3 vBeforeTarget(PathFinder::MakeOgreVector3(*pntIter));

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
                combatMove = true;
                timerCombatMove = 0;
            }
            readyToAttack = false;
        }

        if(!isStuck && distToTarget > rangeAttack && !distantCombat)
        {
            //special run attack; it shouldn't affect melee combat tactics
            if(actorClass.getMovementSettings(actor).mPosition[1] == 1)
            {
                /*  check if actor can overcome the distance = distToTarget - attackerWeapRange
                    less than in time of swinging with weapon (t_swing), then start attacking 
                */
                float speed1 = actorClass.getSpeed(actor);
                float speed2 = target.getClass().getSpeed(target);
                if(target.getClass().getMovementSettings(target).mPosition[0] == 0
                        && target.getClass().getMovementSettings(target).mPosition[1] == 0)
                    speed2 = 0;

                float s1 = distToTarget - weapRange;
                float t = s1/speed1;
                float s2 = speed2 * t;
                float t_swing = 
                    minMaxAttackDuration[ESM::Weapon::AT_Thrust][0] + 
                    (minMaxAttackDuration[ESM::Weapon::AT_Thrust][1] - minMaxAttackDuration[ESM::Weapon::AT_Thrust][0]) * OEngine::Misc::Rng::rollClosedProbability();

                if (t + s2/speed1 <= t_swing)
                {
                    readyToAttack = true;
                    if(timerAttack <= -attacksPeriod)
                    {
                        timerAttack = t_swing;
                        attack = true;
                    }
                }
            }
        }

        // NOTE: This section gets updated every tReaction, which is currently hard
        //       coded at 250ms or 1/4 second
        //
        // TODO: Add a parameter to vary DURATION_SAME_SPOT?
        if((distToTarget > rangeAttack || followTarget) &&
            mObstacleCheck.check(actor, tReaction)) // check if evasive action needed
        {
            // probably walking into another NPC TODO: untested in combat situation
            // TODO: diagonal should have same animation as walk forward
            //       but doesn't seem to do that?
            actor.getClass().getMovementSettings(actor).mPosition[0] = 1;
            actor.getClass().getMovementSettings(actor).mPosition[1] = 0.1f;
            // change the angle a bit, too
            if(mPathFinder.isPathConstructed())
                zTurn(actor, Ogre::Degree(mPathFinder.getZAngleToNext(pos.pos[0] + 1, pos.pos[1])));

            if(followTarget)
                followTarget = false;
            // FIXME: can fool actors to stay behind doors, etc.
            // Related to Bug#1102 and to some degree #1155 as well
        }

        return false;
    }

    void AiCombat::buildNewPath(const MWWorld::Ptr& actor, const MWWorld::Ptr& target)
    {
        Ogre::Vector3 newPathTarget = Ogre::Vector3(target.getRefData().getPosition().pos);

        float dist;

        if(!mPathFinder.getPath().empty())
        {
            ESM::Pathgrid::Point lastPt = mPathFinder.getPath().back();
            Ogre::Vector3 currPathTarget(PathFinder::MakeOgreVector3(lastPt));
            dist = (newPathTarget - currPathTarget).length();
        }
        else dist = 1e+38F; // necessarily construct a new path

        float targetPosThreshold = (actor.getCell()->getCell()->isExterior())? 300.0f : 100.0f;

        //construct new path only if target has moved away more than on [targetPosThreshold]
        if(dist > targetPosThreshold)
        {
            ESM::Position pos = actor.getRefData().getPosition();

            ESM::Pathgrid::Point start(PathFinder::MakePathgridPoint(pos));

            ESM::Pathgrid::Point dest(PathFinder::MakePathgridPoint(newPathTarget));

            if(!mPathFinder.isPathConstructed())
                mPathFinder.buildPath(start, dest, actor.getCell(), false);
            else
            {
                PathFinder newPathFinder;
                newPathFinder.buildPath(start, dest, actor.getCell(), false);

                if(!mPathFinder.getPath().empty())
                {
                    newPathFinder.syncStart(mPathFinder.getPath());
                    mPathFinder = newPathFinder;
                }
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
        float roll = OEngine::Misc::Rng::rollClosedProbability();
        if(roll <= 0.333f)  //side punch
        {
            movement.mPosition[0] = OEngine::Misc::Rng::rollClosedProbability() ? 1.0f : -1.0f;
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

        float total = static_cast<float>(slash + chop + thrust);

        float roll = OEngine::Misc::Rng::rollClosedProbability();
        if(roll <= (slash/total))
        {
            movement.mPosition[0] = (OEngine::Misc::Rng::rollClosedProbability() < 0.5f) ? 1.0f : -1.0f;
            movement.mPosition[1] = 0;
            attackType = ESM::Weapon::AT_Slash;
        }
        else if(roll <= (slash + (thrust/total)))
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

void getMinMaxAttackDuration(const MWWorld::Ptr& actor, float (*fMinMaxDurations)[2])
{
    if (!actor.getClass().hasInventoryStore(actor)) // creatures
    {
        fMinMaxDurations[0][0] = fMinMaxDurations[0][1] = 0.1f;
        fMinMaxDurations[1][0] = fMinMaxDurations[1][1] = 0.1f;
        fMinMaxDurations[2][0] = fMinMaxDurations[2][1] = 0.1f;

        return;
    }

    // get weapon information: type and speed
    const ESM::Weapon *weapon = NULL;
    MWMechanics::WeaponType weaptype = MWMechanics::WeapType_None;

    MWWorld::ContainerStoreIterator weaponSlot =
        MWMechanics::getActiveWeapon(actor.getClass().getCreatureStats(actor), actor.getClass().getInventoryStore(actor), &weaptype);

    float weapSpeed;
    if (weaptype != MWMechanics::WeapType_HandToHand
            && weaptype != MWMechanics::WeapType_Spell
            && weaptype != MWMechanics::WeapType_None)
    {
        weapon = weaponSlot->get<ESM::Weapon>()->mBase;
        weapSpeed = weapon->mData.mSpeed;
    }
    else  weapSpeed = 1.0f;

    MWRender::Animation *anim = MWBase::Environment::get().getWorld()->getAnimation(actor);

    std::string weapGroup;
    MWMechanics::getWeaponGroup(weaptype, weapGroup);
    weapGroup = weapGroup + ": ";

    bool bRangedWeap = (weaptype >= MWMechanics::WeapType_BowAndArrow && weaptype <= MWMechanics::WeapType_Thrown);

    const char *attackType[] = {"chop ", "slash ", "thrust ", "shoot "};

    std::string textKey = "start";
    std::string textKey2;

    // get durations for each attack type
    for (int i = 0; i < (bRangedWeap ? 1 : 3); i++)
    {
        float start1 = anim->getTextKeyTime(weapGroup + (bRangedWeap ? attackType[3] : attackType[i]) + textKey);

        if (start1 < 0) 
        {
            fMinMaxDurations[i][0] = fMinMaxDurations[i][1] = 0.1f;
            continue;
        }

        textKey2 = "min attack";
        float start2 = anim->getTextKeyTime(weapGroup + (bRangedWeap ? attackType[3] : attackType[i]) + textKey2);

        fMinMaxDurations[i][0] = (start2 - start1) / weapSpeed;

        textKey2 = "max attack";
        start1 = anim->getTextKeyTime(weapGroup + (bRangedWeap ? attackType[3] : attackType[i]) + textKey2);

        fMinMaxDurations[i][1] = fMinMaxDurations[i][0] + (start1 - start2) / weapSpeed;
    }

}

Ogre::Vector3 AimDirToMovingTarget(const MWWorld::Ptr& actor, const MWWorld::Ptr& target, const Ogre::Vector3& vLastTargetPos, 
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

    Ogre::Vector3 vActorPos = Ogre::Vector3(actor.getRefData().getPosition().pos);
    Ogre::Vector3 vTargetPos = Ogre::Vector3(target.getRefData().getPosition().pos);
    Ogre::Vector3 vDirToTarget = vTargetPos - vActorPos;
    float distToTarget = vDirToTarget.length();

    Ogre::Vector3 vTargetMoveDir = vTargetPos - vLastTargetPos;
    vTargetMoveDir /= duration; // |vTargetMoveDir| is target real speed in units/sec now

    Ogre::Vector3 vPerpToDir = vDirToTarget.crossProduct(Ogre::Vector3::UNIT_Z);

    float velPerp = vTargetMoveDir.dotProduct(vPerpToDir.normalisedCopy());
    float velDir = vTargetMoveDir.dotProduct(vDirToTarget.normalisedCopy());

    // time to collision between target and projectile
    float t_collision;

    float projVelDirSquared = projSpeed * projSpeed - velPerp * velPerp;
    float projDistDiff = vDirToTarget.dotProduct(vTargetMoveDir.normalisedCopy());
    projDistDiff = sqrt(distToTarget * distToTarget - projDistDiff * projDistDiff);

    if (projVelDirSquared > 0)
        t_collision = projDistDiff / (sqrt(projVelDirSquared) - velDir);
    else t_collision = 0; // speed of projectile is not enough to reach moving target

    return vTargetPos + vTargetMoveDir * t_collision - vActorPos;
}

}
