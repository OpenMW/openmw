#include "aicombat.hpp"

#include <OgreMath.h>
#include <OgreVector3.h>


#include "../mwworld/class.hpp"
#include "../mwworld/timestamp.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"


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
    void chooseBestAttack(const ESM::Weapon* weapon, MWMechanics::Movement &movement);

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
    bool checkWayIsClear(const Ogre::Vector3& from, const Ogre::Vector3& to, float offset)
    {
        if((to - from).length() >= PATHFIND_CAUTION_DIST || abs(from.z - to.z) <= PATHFIND_Z_REACH)
        {
            Ogre::Vector3 dir = to - from;
            dir.z = 0;
            dir.normalise();
			float verticalOffset = 200; // instead of '200' here we want the height of the actor
            Ogre::Vector3 _from = from + dir*offset + Ogre::Vector3::UNIT_Z * verticalOffset;

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
    static const float MAX_ATTACK_DURATION = 0.35f;
    static const float DOOR_CHECK_INTERVAL = 1.5f; // same as AiWander
    // NOTE: MIN_DIST_TO_DOOR_SQUARED is defined in obstacle.hpp

    AiCombat::AiCombat(const MWWorld::Ptr& actor) :
        mTargetActorId(actor.getClass().getCreatureStats(actor).getActorId()),
        mTimerAttack(0),
        mTimerReact(0),
        mTimerCombatMove(0),
        mFollowTarget(false),
        mReadyToAttack(false),
        mAttack(false),
        mCombatMove(false),
        mMovement(),
        mForceNoShortcut(false),
        mShortcutFailPos(),
        mBackOffDoor(false),
        mCell(NULL),
        mDoorIter(actor.getCell()->get<ESM::Door>().mList.end()),
        mDoors(actor.getCell()->get<ESM::Door>()),
        mDoorCheckDuration(0)
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
    bool AiCombat::execute (const MWWorld::Ptr& actor,float duration)
    {
        //General description
        if(!actor.getClass().getCreatureStats(actor).isHostile()
                || actor.getClass().getCreatureStats(actor).getHealth().getCurrent() <= 0)
            return true;

        MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtrViaActorId(mTargetActorId);

        if(target.getClass().getCreatureStats(target).isDead())
            return true;

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

        actor.getClass().getMovementSettings(actor) = mMovement;
        actor.getClass().getMovementSettings(actor).mRotation[0] = 0;
        actor.getClass().getMovementSettings(actor).mRotation[2] = 0;

        if(mMovement.mRotation[2] != 0)
        {
            if(zTurn(actor, Ogre::Degree(mMovement.mRotation[2]))) mMovement.mRotation[2] = 0;
        }

        if(mMovement.mRotation[0] != 0)
        {
            if(smoothTurn(actor, Ogre::Degree(mMovement.mRotation[0]), 0)) mMovement.mRotation[0] = 0;
        }

        mTimerAttack -= duration;
        actor.getClass().getCreatureStats(actor).setAttackingOrSpell(mAttack);

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

        //actual attacking logic
        //TODO: Some skills affect period of strikes.For berserk-like style period ~ 0.25f
        float attacksPeriod = 1.0f;
        if(mReadyToAttack)
        {
            if(mTimerAttack <= -attacksPeriod)
            {
                //TODO: should depend on time between 'start' to 'min attack'
                //for better controlling of NPCs' attack strength.
                //Also it seems that this time is different for slash/thrust/chop
                mTimerAttack = MAX_ATTACK_DURATION * static_cast<float>(rand())/RAND_MAX;
                mAttack = true;

                //say a provoking combat phrase
                if (actor.getClass().isNpc())
                {
                    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
                    int chance = store.get<ESM::GameSetting>().find("iVoiceAttackOdds")->getInt();
                    int roll = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 100; // [0, 99]
                    if (roll < chance)
                    {
                        MWBase::Environment::get().getDialogueManager()->say(actor, "attack");
                    }
                }
            }
            else if (mTimerAttack <= 0)
                mAttack = false;
        }
        else
        {
            mTimerAttack = -attacksPeriod;
            mAttack = false;
        }

        const MWWorld::Class &actorCls = actor.getClass();
        const ESM::Weapon *weapon = NULL;
        MWMechanics::WeaponType weaptype;
        float weapRange, weapSpeed = 1.0f;

        actorCls.getCreatureStats(actor).setMovementFlag(CreatureStats::Flag_Run, true);

        // Get weapon characteristics
        if (actorCls.hasInventoryStore(actor))
        {
            MWMechanics::DrawState_ state = actorCls.getCreatureStats(actor).getDrawState();
            if (state == MWMechanics::DrawState_Spell || state == MWMechanics::DrawState_Nothing)
                actorCls.getCreatureStats(actor).setDrawState(MWMechanics::DrawState_Weapon);

            // TODO: Check equipped weapon and equip a different one if we can't attack with it
            // (e.g. no ammunition, or wrong type of ammunition equipped, etc. autoEquip is not very smart in this regard))

            //Get weapon speed and range
            MWWorld::ContainerStoreIterator weaponSlot =
                MWMechanics::getActiveWeapon(actorCls.getCreatureStats(actor), actorCls.getInventoryStore(actor), &weaptype);

            if (weaptype == WeapType_HandToHand)
            {
                const MWWorld::Store<ESM::GameSetting> &gmst =
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
                weapRange = gmst.find("fHandToHandReach")->getFloat();
            }
            else if (weaptype != WeapType_PickProbe && weaptype != WeapType_Spell)
            {
                // All other WeapTypes are actually weapons, so get<ESM::Weapon> is safe.
                weapon = weaponSlot->get<ESM::Weapon>()->mBase;
                weapRange = weapon->mData.mReach;
                weapSpeed = weapon->mData.mSpeed;
            }
            weapRange *= 100.0f;
        }
        else //is creature
        {
            weaptype = WeapType_HandToHand; //doesn't matter, should only reflect if it is melee or distant weapon
            weapRange = 150; //TODO: use true attack range (the same problem in Creature::hit)
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

        ESM::Position pos = actor.getRefData().getPosition();
        Ogre::Vector3 vActorPos(pos.pos);
        Ogre::Vector3 vTargetPos(target.getRefData().getPosition().pos);
        Ogre::Vector3 vDirToTarget = vTargetPos - vActorPos;

        bool isStuck = false;
        float speed = 0.0f;
        if(mMovement.mPosition[1] && (Ogre::Vector3(mLastPos.pos) - vActorPos).length() < (speed = actorCls.getSpeed(actor)) / 10.0f)
            isStuck = true;

        mLastPos = pos;

        // check if actor can move along z-axis
        bool canMoveByZ = (actorCls.canSwim(actor) && MWBase::Environment::get().getWorld()->isSwimming(actor))
            || MWBase::Environment::get().getWorld()->isFlying(actor);

        // determine vertical angle to target
        // if actor can move along z-axis it will control movement dir
        // if can't - it will control correct aiming
        mMovement.mRotation[0] = getXAngleToDir(vDirToTarget);

        vDirToTarget.z = 0;
        float distToTarget = vDirToTarget.length();

        // (within strike dist) || (not quite strike dist while following)
        if(distToTarget < rangeAttack || (distToTarget <= rangeFollow && mFollowTarget && !isStuck) )
        {
            //Melee and Close-up combat
            mMovement.mRotation[2] = getZAngleToDir(vDirToTarget, distToTarget);

            // (not quite strike dist while following)
            if (mFollowTarget && distToTarget > rangeAttack)
            {
                //Close-up combat: just run up on target
                mMovement.mPosition[1] = 1;
            }
            else // (within strike dist)
            {
                mMovement.mPosition[1] = 0;

                // set slash/thrust/chop attack
                if (mAttack && !distantCombat) chooseBestAttack(weapon, mMovement);

                if(mMovement.mPosition[0] || mMovement.mPosition[1])
                {
                    mTimerCombatMove = 0.1f + 0.1f * static_cast<float>(rand())/RAND_MAX;
                    mCombatMove = true;
                }
                // only NPCs are smart enough to use dodge movements
                else if(actorCls.isNpc() && (!distantCombat || (distantCombat && distToTarget < rangeAttack/2)))
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
            bool inLOS = MWBase::Environment::get().getWorld()->getLOS(actor, target);

            if(mReadyToAttack) isStuck = false;
			
            // check if shortcut is available
            if(!isStuck
                && (!mForceNoShortcut
                || (Ogre::Vector3(mShortcutFailPos.pos) - vActorPos).length() >= PATHFIND_SHORTCUT_RETRY_DIST)
                && inLOS)
            {
                if(speed == 0.0f) speed = actorCls.getSpeed(actor);
                // maximum dist before pit/obstacle for actor to avoid them depending on his speed
                float maxAvoidDist = tReaction * speed + speed / MAX_VEL_ANGULAR.valueRadians() * 2; // *2 - for reliability
				preferShortcut = checkWayIsClear(vActorPos, vTargetPos, distToTarget > maxAvoidDist*1.5? maxAvoidDist : maxAvoidDist/2);
            }

            // don't use pathgrid when actor can move in 3 dimensions
            if(canMoveByZ) preferShortcut = true;

            if(preferShortcut)
            {
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
            mReadyToAttack = false;
        }

        if(distToTarget > rangeAttack && !distantCombat)
        {
            //special run attack; it shouldn't affect melee combat tactics
            if(actorCls.getMovementSettings(actor).mPosition[1] == 1)
            {
                //check if actor can overcome the distance = distToTarget - attackerWeapRange
                //less than in time of playing weapon anim from 'start' to 'hit' tags (t_swing)
                //then start attacking
                float speed1 = actorCls.getSpeed(actor);
                float speed2 = target.getClass().getSpeed(target);
                if(target.getClass().getMovementSettings(target).mPosition[0] == 0
                        && target.getClass().getMovementSettings(target).mPosition[1] == 0)
                    speed2 = 0;

                float s1 = distToTarget - weapRange;
                float t = s1/speed1;
                float s2 = speed2 * t;
                float t_swing = (MAX_ATTACK_DURATION/2) / weapSpeed;//instead of 0.17 should be the time of playing weapon anim from 'start' to 'hit' tags
                if (t + s2/speed1 <= t_swing)
                {
                    mReadyToAttack = true;
                    if(mTimerAttack <= -attacksPeriod)
                    {
                        mTimerAttack = MAX_ATTACK_DURATION * static_cast<float>(rand())/RAND_MAX;
                        mAttack = true;
                    }
                }
            }
        }

        // NOTE: This section gets updated every tReaction, which is currently hard
        //       coded at 250ms or 1/4 second
        //
        // TODO: Add a parameter to vary DURATION_SAME_SPOT?
        MWWorld::CellStore *cell = actor.getCell();
        if((distToTarget > rangeAttack || mFollowTarget) &&
            mObstacleCheck.check(actor, tReaction)) // check if evasive action needed
        {
            // first check if we're walking into a door
            mDoorCheckDuration += 1.0f; // add time taken for obstacle check
            if(mDoorCheckDuration >= DOOR_CHECK_INTERVAL && !cell->getCell()->isExterior())
            {
                mDoorCheckDuration = 0;
                // Check all the doors in this cell
                mDoors = cell->get<ESM::Door>(); // update
                mDoorIter = mDoors.mList.begin();
                for (; mDoorIter != mDoors.mList.end(); ++mDoorIter)
                {
                    MWWorld::LiveCellRef<ESM::Door>& ref = *mDoorIter;
                    float minSqr = 1.3*1.3*MIN_DIST_TO_DOOR_SQUARED; // for legibility
                    if(vActorPos.squaredDistance(Ogre::Vector3(ref.mRef.mPos.pos)) < minSqr &&
                       ref.mData.getLocalRotation().rot[2] < 0.4f) // even small opening
                    {
                        //std::cout<<"closed door id \""<<ref.mRef.mRefID<<"\""<<std::endl;
                        mBackOffDoor = true;
                        mObstacleCheck.clear();
                        if(mFollowTarget)
                            mFollowTarget = false;
                        break;
                    }
                }
            }
            else // probably walking into another NPC TODO: untested in combat situation
            {
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
        }

        if(!cell->getCell()->isExterior() && !mDoors.mList.empty())
        {
            MWWorld::LiveCellRef<ESM::Door>& ref = *mDoorIter;
            float minSqr = 1.6 * 1.6 * MIN_DIST_TO_DOOR_SQUARED; // for legibility
            // TODO: add reaction to checking open doors
            if(mBackOffDoor &&
               vActorPos.squaredDistance(Ogre::Vector3(ref.mRef.mPos.pos)) < minSqr)
            {
                mMovement.mPosition[1] = -0.2; // back off, but slowly
            }
            else if(mBackOffDoor &&
                    mDoorIter != mDoors.mList.end() &&
                    ref.mData.getLocalRotation().rot[2] >= 1)
            {
                mDoorIter = mDoors.mList.end();
                mBackOffDoor = false;
                //std::cout<<"open door id \""<<ref.mRef.mRefID<<"\""<<std::endl;
                mMovement.mPosition[1] = 1;
            }
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

    std::string AiCombat::getTargetId() const
    {
        MWWorld::Ptr target = MWBase::Environment::get().getWorld()->searchPtrViaActorId(mTargetActorId);
        if (target.isEmpty())
            return "";
        return target.getRefData().getHandle();
    }


    AiCombat *MWMechanics::AiCombat::clone() const
    {
        return new AiCombat(*this);
    }
}


namespace
{

void chooseBestAttack(const ESM::Weapon* weapon, MWMechanics::Movement &movement)
{
    if (weapon == NULL)
    {
        //hand-to-hand deal equal damage for each type
        float roll = static_cast<float>(rand())/RAND_MAX;
        if(roll <= 0.333f)  //side punch
        {
            movement.mPosition[0] = (static_cast<float>(rand())/RAND_MAX < 0.5f)? 1: -1;
            movement.mPosition[1] = 0;
        }
        else if(roll <= 0.666f) //forward punch
            movement.mPosition[1] = 1;
        else
        {
            movement.mPosition[1] = movement.mPosition[0] = 0;
        }

        return;
    }

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
    }
    else if(roll <= (static_cast<float>(slash) + static_cast<float>(thrust))/total)
        movement.mPosition[1] = 1;
    else
        movement.mPosition[1] = movement.mPosition[0] = 0;
}

}
