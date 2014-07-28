#include "aicombat.hpp"

#include <OgreMath.h>

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

namespace
{
    static float sgn(Ogre::Radian a)
    {
        if(a.valueDegrees() > 0)
            return 1.0;
        return -1.0;
    }

    //chooses an attack depending on probability to avoid uniformity
    ESM::Weapon::AttackType chooseBestAttack(const ESM::Weapon* weapon, MWMechanics::Movement &movement);

    void getMinMaxAttackDuration(const MWWorld::Ptr& actor, float (*fMinMaxDurations)[2]);

    Ogre::Vector3 AimDirToMovingTarget(const MWWorld::Ptr& actor, const MWWorld::Ptr& target, const Ogre::Vector3& vLastTargetPos, 
        float duration, int weapType, float strength);

    float getZAngleToDir(const Ogre::Vector3& dir, float dirLen = 0.0f)
    {
        float len = (dirLen > 0.0f)? dirLen : dir.length();
        return Ogre::Radian( Ogre::Math::ACos(dir.y / len) * sgn(Ogre::Math::ASin(dir.x / len)) ).valueDegrees();
    }

    float getXAngleToDir(const Ogre::Vector3& dir, float dirLen = 0.0f)
    {
        float len = (dirLen > 0.0f)? dirLen : dir.length();
        return Ogre::Radian(-Ogre::Math::ASin(dir.z / len)).valueDegrees();
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
        if((to - from).length() >= PATHFIND_CAUTION_DIST || abs(from.z - to.z) <= PATHFIND_Z_REACH)
        {
            Ogre::Vector3 dir = to - from;
            dir.z = 0;
            dir.normalise();
			float verticalOffset = 200; // instead of '200' here we want the height of the actor
            Ogre::Vector3 _from = from + dir*offsetXY + Ogre::Vector3::UNIT_Z * verticalOffset;

            // cast up-down ray and find height in world space of hit
            float h = _from.z - MWBase::Environment::get().getWorld()->getDistToNearestRayHit(_from, -Ogre::Vector3::UNIT_Z, verticalOffset + PATHFIND_Z_REACH + 1);

            if(abs(from.z - h) <= PATHFIND_Z_REACH)
                return true;
        }

        return false;
    }
}

namespace MWMechanics
{
    static const float DOOR_CHECK_INTERVAL = 1.5f; // same as AiWander
    // NOTE: MIN_DIST_TO_DOOR_SQUARED is defined in obstacle.hpp

    AiCombat::AiCombat(const MWWorld::Ptr& actor) :
        mTargetActorId(actor.getClass().getCreatureStats(actor).getActorId())
      , mMinMaxAttackDuration()
      , mMovement()
    {
        init();

        mLastTargetPos = Ogre::Vector3(actor.getRefData().getPosition().pos);
    }

    AiCombat::AiCombat(const ESM::AiSequence::AiCombat *combat)
        : mMinMaxAttackDuration()
        , mMovement()
    {
        mTargetActorId = combat->mTargetActorId;

        init();
    }

    void AiCombat::init()
    {
        mTimerAttack = 0;
        mTimerReact = 0;
        mTimerCombatMove = 0;
        mFollowTarget = false;
        mReadyToAttack = false;
        mAttack = false;
        mCombatMove = false;
        mForceNoShortcut = false;
        mStrength = 0;
        mCell = NULL;
        mLastTargetPos = Ogre::Vector3(0,0,0);
        mMinMaxAttackDurationInitialised = false;
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
    bool AiCombat::execute (const MWWorld::Ptr& actor,float duration)
    {
        //General description
        if(actor.getClass().getCreatureStats(actor).isDead())
            return true;

        MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtrViaActorId(mTargetActorId);
        if (target.isEmpty())
            return false;

        if(target.getClass().getCreatureStats(target).isDead())
            return true;

        const MWWorld::Class& actorClass = actor.getClass();
        MWBase::World* world = MWBase::Environment::get().getWorld();

        if (!actorClass.isNpc() &&
            // 1. pure water creature and Player moved out of water
            ((target == world->getPlayerPtr() &&
            actorClass.canSwim(actor) && !actor.getClass().canWalk(actor) && !world->isSwimming(target))
            // 2. creature can't swim to target
            || (!actorClass.canSwim(actor) && world->isSwimming(target))))
        {
            actorClass.getCreatureStats(actor).setAttackingOrSpell(false);
            return true;
        }  

        //Update every frame
        if(mCombatMove)
        {
            mTimerCombatMove -= duration;
            if( mTimerCombatMove <= 0)
            {
                mTimerCombatMove = 0;
                mMovement.mPosition[1] = mMovement.mPosition[0] = 0;
                mCombatMove = false;
            }
        }

        actorClass.getMovementSettings(actor) = mMovement;
        actorClass.getMovementSettings(actor).mRotation[0] = 0;
        actorClass.getMovementSettings(actor).mRotation[2] = 0;

        if(mMovement.mRotation[2] != 0)
        {
            if(zTurn(actor, Ogre::Degree(mMovement.mRotation[2]))) mMovement.mRotation[2] = 0;
        }

        if(mMovement.mRotation[0] != 0)
        {
            if(smoothTurn(actor, Ogre::Degree(mMovement.mRotation[0]), 0)) mMovement.mRotation[0] = 0;
        }

        //TODO: Some skills affect period of strikes.For berserk-like style period ~ 0.25f
        float attacksPeriod = 1.0f;

        ESM::Weapon::AttackType attackType;

        if(mReadyToAttack)
        {
            if (!mMinMaxAttackDurationInitialised)
            {
                // TODO: this must be updated when a different weapon is equipped
                getMinMaxAttackDuration(actor, mMinMaxAttackDuration);
                mMinMaxAttackDurationInitialised = true;
            }

            if (mTimerAttack < 0) mAttack = false;

            mTimerAttack -= duration;
        }
        else
        {
            mTimerAttack = -attacksPeriod;
            mAttack = false;
        }

        actorClass.getCreatureStats(actor).setAttackingOrSpell(mAttack);

        float tReaction = 0.25f;
        if(mTimerReact < tReaction)
        {
            mTimerReact += duration;
            return false;
        }

        //Update with period = tReaction

        mTimerReact = 0;

        bool cellChange = mCell && (actor.getCell() != mCell);
        if(!mCell || cellChange)
        {
            mCell = actor.getCell();
        }

        const ESM::Weapon *weapon = NULL;
        MWMechanics::WeaponType weaptype;
        float weapRange = 1.0f;

        actorClass.getCreatureStats(actor).setMovementFlag(CreatureStats::Flag_Run, true);

        // Get weapon characteristics
        if (actorClass.hasInventoryStore(actor))
        {
            MWMechanics::DrawState_ state = actorClass.getCreatureStats(actor).getDrawState();
            if (state == MWMechanics::DrawState_Spell || state == MWMechanics::DrawState_Nothing)
                actorClass.getCreatureStats(actor).setDrawState(MWMechanics::DrawState_Weapon);

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
            else if (weaptype != WeapType_PickProbe && weaptype != WeapType_Spell)
            {
                // All other WeapTypes are actually weapons, so get<ESM::Weapon> is safe.
                weapon = weaponSlot->get<ESM::Weapon>()->mBase;
                weapRange = weapon->mData.mReach;
            }
            weapRange *= 100.0f;
        }
        else //is creature
        {
            weaptype = WeapType_HandToHand; //doesn't matter, should only reflect if it is melee or distant weapon
            weapRange = 150.0f; //TODO: use true attack range (the same problem in Creature::hit)
        }

        float rangeAttack;
        float rangeFollow;
        bool distantCombat = false;
        if (weaptype == WeapType_BowAndArrow || weaptype == WeapType_Crossbow || weaptype == WeapType_Thrown)
        {
            rangeAttack = 1000; // TODO: should depend on archer skill
            rangeFollow = 0; // not needed in ranged combat
            distantCombat = true;
        }
        else
        {
            rangeAttack = weapRange;
            rangeFollow = 300;
        }

        // start new attack
        if(mReadyToAttack)
        {
            if(mTimerAttack <= -attacksPeriod)
            {
                mAttack = true; // attack starts just now

                if (!distantCombat) attackType = chooseBestAttack(weapon, mMovement);
                else attackType = ESM::Weapon::AT_Chop; // cause it's =0

                mStrength = static_cast<float>(rand()) / RAND_MAX;

                // Note: may be 0 for some animations
                mTimerAttack = mMinMaxAttackDuration[attackType][0] + 
                    (mMinMaxAttackDuration[attackType][1] - mMinMaxAttackDuration[attackType][0]) * mStrength;

                //say a provoking combat phrase
                if (actor.getClass().isNpc())
                {
                    const MWWorld::ESMStore &store = world->getStore();
                    int chance = store.get<ESM::GameSetting>().find("iVoiceAttackOdds")->getInt();
                    int roll = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 100; // [0, 99]
                    if (roll < chance)
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

        bool isStuck = false;
        float speed = 0.0f;
        if(mMovement.mPosition[1] && (mLastActorPos - vActorPos).length() < (speed = actorClass.getSpeed(actor)) * tReaction / 2)
            isStuck = true;

        mLastActorPos = vActorPos;

        // check if actor can move along z-axis
        bool canMoveByZ = (actorClass.canSwim(actor) && world->isSwimming(actor))
            || world->isFlying(actor);

        // for distant combat we should know if target is in LOS even if distToTarget < rangeAttack 
        bool inLOS = distantCombat ? world->getLOS(actor, target) : true;

        // (within attack dist) || (not quite attack dist while following)
        if(inLOS && (distToTarget < rangeAttack || (distToTarget <= rangeFollow && mFollowTarget && !isStuck)))
        {
            //Melee and Close-up combat
            
            // getXAngleToDir determines vertical angle to target:
            // if actor can move along z-axis it will control movement dir
            // if can't - it will control correct aiming.
            // note: in getZAngleToDir if we preserve dir.z then horizontal angle can be inaccurate
            if (distantCombat)
            {
                Ogre::Vector3 vAimDir = AimDirToMovingTarget(actor, target, mLastTargetPos, tReaction, weaptype, mStrength);
                mLastTargetPos = vTargetPos;
                mMovement.mRotation[0] = getXAngleToDir(vAimDir);
                mMovement.mRotation[2] = getZAngleToDir(Ogre::Vector3(vAimDir.x, vAimDir.y, 0));
            }
            else
            {
                mMovement.mRotation[0] = getXAngleToDir(vDirToTarget, distToTarget);
                mMovement.mRotation[2] = getZAngleToDir(Ogre::Vector3(vDirToTarget.x, vDirToTarget.y, 0));
            }

            // (not quite attack dist while following)
            if (mFollowTarget && distToTarget > rangeAttack)
            {
                //Close-up combat: just run up on target
                mMovement.mPosition[1] = 1;
            }
            else // (within attack dist)
            {
                if(mMovement.mPosition[0] || mMovement.mPosition[1])
                {
                    mTimerCombatMove = 0.1f + 0.1f * static_cast<float>(rand())/RAND_MAX;
                    mCombatMove = true;
                }
                // only NPCs are smart enough to use dodge movements
                else if(actorClass.isNpc() && (!distantCombat || (distantCombat && distToTarget < rangeAttack/2)))
                {
                    //apply sideway movement (kind of dodging) with some probability
                    if(static_cast<float>(rand())/RAND_MAX < 0.25)
                    {
                        mMovement.mPosition[0] = static_cast<float>(rand())/RAND_MAX < 0.5? 1: -1;
                        mTimerCombatMove = 0.05f + 0.15f * static_cast<float>(rand())/RAND_MAX;
                        mCombatMove = true;
                    }
                }

                if(distantCombat && distToTarget < rangeAttack/4)
                {
                    mMovement.mPosition[1] = -1;
                }

                mReadyToAttack = true;
                //only once got in melee combat, actor is allowed to use close-up shortcutting
                mFollowTarget = true;
            }
        }
        else // remote pathfinding
        {
            bool preferShortcut = false;
            if (!distantCombat) inLOS = world->getLOS(actor, target);

            // check if shortcut is available
            if(inLOS && (!isStuck || mReadyToAttack)
                && (!mForceNoShortcut || (Ogre::Vector3(mShortcutFailPos.pos) - vActorPos).length() >= PATHFIND_SHORTCUT_RETRY_DIST))
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
                    mMovement.mRotation[0] = getXAngleToDir(vDirToTarget, distToTarget);
                mMovement.mRotation[2] = getZAngleToDir(vDirToTarget, distToTarget);
                mForceNoShortcut = false;
                mShortcutFailPos.pos[0] = mShortcutFailPos.pos[1] = mShortcutFailPos.pos[2] = 0;
                mPathFinder.clearPath();
            }
            else // if shortcut failed stick to path grid
            {
                if(!isStuck && mShortcutFailPos.pos[0] == 0.0f && mShortcutFailPos.pos[1] == 0.0f && mShortcutFailPos.pos[2] == 0.0f)
                {
                    mForceNoShortcut = true;
                    mShortcutFailPos = pos;
                }

                mFollowTarget = false;

                buildNewPath(actor, target); //may fail to build a path, check before use

                //delete visited path node
                mPathFinder.checkWaypoint(pos.pos[0],pos.pos[1],pos.pos[2]);

                // This works on the borders between the path grid and areas with no waypoints.
                if(inLOS && mPathFinder.getPath().size() > 1)
                {
                    // get point just before target
                    std::list<ESM::Pathgrid::Point>::const_iterator pntIter = --mPathFinder.getPath().end();
                    --pntIter;
                    Ogre::Vector3 vBeforeTarget = Ogre::Vector3(pntIter->mX, pntIter->mY, pntIter->mZ);

                    // if current actor pos is closer to target then last point of path (excluding target itself) then go straight on target
                    if(distToTarget <= (vTargetPos - vBeforeTarget).length())
                    {
                        mMovement.mRotation[2] = getZAngleToDir(vDirToTarget, distToTarget);
                        preferShortcut = true;
                    }
                }

                // if there is no new path, then go straight on target
                if(!preferShortcut)
                {
                    if(!mPathFinder.getPath().empty())
                        mMovement.mRotation[2] = mPathFinder.getZAngleToNext(pos.pos[0], pos.pos[1]);
                    else
                        mMovement.mRotation[2] = getZAngleToDir(vDirToTarget, distToTarget);
                }
            }

            mMovement.mPosition[1] = 1;
            if (mReadyToAttack)
            {
                // to stop possible sideway moving after target moved out of attack range
                mCombatMove = true;
                mTimerCombatMove = 0;
            }
            mReadyToAttack = false;
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
                    mMinMaxAttackDuration[ESM::Weapon::AT_Thrust][0] + 
                    (mMinMaxAttackDuration[ESM::Weapon::AT_Thrust][1] - mMinMaxAttackDuration[ESM::Weapon::AT_Thrust][0]) * static_cast<float>(rand()) / RAND_MAX;

                if (t + s2/speed1 <= t_swing)
                {
                    mReadyToAttack = true;
                    if(mTimerAttack <= -attacksPeriod)
                    {
                        mTimerAttack = t_swing;
                        mAttack = true;
                    }
                }
            }
        }

        // NOTE: This section gets updated every tReaction, which is currently hard
        //       coded at 250ms or 1/4 second
        //
        // TODO: Add a parameter to vary DURATION_SAME_SPOT?
        if((distToTarget > rangeAttack || mFollowTarget) &&
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

            if(mFollowTarget)
                mFollowTarget = false;
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
            Ogre::Vector3 currPathTarget(lastPt.mX, lastPt.mY, lastPt.mZ);
            dist = (newPathTarget - currPathTarget).length();
        }
        else dist = 1e+38F; // necessarily construct a new path

        float targetPosThreshold = (actor.getCell()->getCell()->isExterior())? 300 : 100;

        //construct new path only if target has moved away more than on [targetPosThreshold]
        if(dist > targetPosThreshold)
        {
            ESM::Position pos = actor.getRefData().getPosition();

            ESM::Pathgrid::Point start;
            start.mX = pos.pos[0];
            start.mY = pos.pos[1];
            start.mZ = pos.pos[2];

            ESM::Pathgrid::Point dest;
            dest.mX = newPathTarget.x;
            dest.mY = newPathTarget.y;
            dest.mZ = newPathTarget.z;

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
        float roll = static_cast<float>(rand())/RAND_MAX;
        if(roll <= 0.333f)  //side punch
        {
            movement.mPosition[0] = (static_cast<float>(rand())/RAND_MAX < 0.5f)? 1: -1;
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

        float total = slash + chop + thrust;

        float roll = static_cast<float>(rand())/RAND_MAX;
        if(roll <= static_cast<float>(slash)/total)
        {
            movement.mPosition[0] = (static_cast<float>(rand())/RAND_MAX < 0.5f)? 1: -1;
            movement.mPosition[1] = 0;
            attackType = ESM::Weapon::AT_Slash;
        }
        else if(roll <= (static_cast<float>(slash) + static_cast<float>(thrust))/total)
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
    MWMechanics::WeaponType weaptype;

    MWWorld::ContainerStoreIterator weaponSlot =
        MWMechanics::getActiveWeapon(actor.getClass().getCreatureStats(actor), actor.getClass().getInventoryStore(actor), &weaptype);

    float weapSpeed;
    if (weaptype != MWMechanics::WeapType_HandToHand) 
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
