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
        float len = (dirLen >= 0.0f)? dirLen : dir.length();
        return Ogre::Radian( Ogre::Math::ACos(dir.y / len) * sgn(Ogre::Math::ASin(dir.x / len)) ).valueDegrees();
    }

    float getXAngleToDir(const Ogre::Vector3& dir, float dirLen = 0.0f)
    {
        float len = (dirLen >= 0.0f)? dirLen : dir.length();
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
    AiCombat::AiCombat(const MWWorld::Ptr& actor) :
        mTarget(actor),
        mTimerAttack(0),
        mTimerReact(0),
        mTimerCombatMove(0),
        mFollowTarget(false),
        mReadyToAttack(false),
        mStrike(false),
        mCombatMove(false),
        mMovement(),
        mForceNoShortcut(false),
        mShortcutFailPos()
    {
    }

    bool AiCombat::execute (const MWWorld::Ptr& actor,float duration)
    {
        //General description
        if(!actor.getClass().getCreatureStats(actor).isHostile()
                || actor.getClass().getCreatureStats(actor).getHealth().getCurrent() <= 0)
            return true;

        if(mTarget.getClass().getCreatureStats(mTarget).isDead())
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
        actor.getClass().getCreatureStats(actor).setAttackingOrSpell(mStrike);

        float tReaction = 0.25f;
        if(mTimerReact < tReaction)
        {
            mTimerReact += duration;
            return false;
        }

        //Update with period = tReaction
        
        mTimerReact = 0;

        //actual attacking logic
        //TODO: Some skills affect period of strikes.For berserk-like style period ~ 0.25f
        float attackPeriod = 1.0f;
        if(mReadyToAttack)
        {
            if(mTimerAttack <= -attackPeriod)
            {
                //TODO: should depend on time between 'start' to 'min attack'
                //for better controlling of NPCs' attack strength.
                //Also it seems that this time is different for slash/thrust/chop
                mTimerAttack = 0.35f * static_cast<float>(rand())/RAND_MAX;
                mStrike = true;

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
                mStrike = false;
        }
        else
        {
            mTimerAttack = -attackPeriod;
            mStrike = false;
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

            //Get weapon speed and range
            MWWorld::ContainerStoreIterator weaponSlot =
                MWMechanics::getActiveWeapon(actorCls.getCreatureStats(actor), actorCls.getInventoryStore(actor), &weaptype);

            if (weaptype == WeapType_HandToHand)
            {
                const MWWorld::Store<ESM::GameSetting> &gmst =
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
                weapRange = gmst.find("fHandToHandReach")->getFloat();
            }
            else
            {
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

        float rangeMelee;
        float rangeCloseUp;
        bool distantCombat = false;
        if (weaptype == WeapType_BowAndArrow || weaptype == WeapType_Crossbow || weaptype == WeapType_Thrown)
        {
            rangeMelee = 1000; // TODO: should depend on archer skill
            rangeCloseUp = 0; // not needed in ranged combat
            distantCombat = true;
        }
        else
        {
            rangeMelee = weapRange;
            rangeCloseUp = 300;
        }
        
        ESM::Position pos = actor.getRefData().getPosition();
        Ogre::Vector3 vActorPos(pos.pos); 
        Ogre::Vector3 vTargetPos(mTarget.getRefData().getPosition().pos);
        Ogre::Vector3 vDirToTarget = vTargetPos - vActorPos;
        float distToTarget = vDirToTarget.length();

        bool isStuck = false;
        float speed = 0.0f;
        if(mMovement.mPosition[1] && (Ogre::Vector3(mLastPos.pos) - vActorPos).length() < (speed = actorCls.getSpeed(actor)) / 10.0f) 
            isStuck = true;

        mLastPos = pos;

        // check if can move along z-axis
        bool canMoveByZ;
        if(canMoveByZ = ((actorCls.isNpc() || actorCls.canSwim(actor)) && MWBase::Environment::get().getWorld()->isSwimming(actor))
            || (actorCls.canFly(actor) && MWBase::Environment::get().getWorld()->isFlying(actor)))
        {
            // determine vertical angle to target
            mMovement.mRotation[0] = getXAngleToDir(vDirToTarget, distToTarget);
        }

        if(distToTarget < rangeMelee || (distToTarget <= rangeCloseUp && mFollowTarget && !isStuck) )
        {
            //Melee and Close-up combat
            vDirToTarget.z = 0;
            mMovement.mRotation[2] = getZAngleToDir(vDirToTarget, distToTarget);

            if (mFollowTarget && distToTarget > rangeMelee)
            {
                //Close-up combat: just run up on target
                mMovement.mPosition[1] = 1;
            }
            else
            {
                //Melee: stop running and attack
                mMovement.mPosition[1] = 0;

                // set slash/thrust/chop attack
                if (mStrike) chooseBestAttack(weapon, mMovement);

                if(mMovement.mPosition[0] || mMovement.mPosition[1])
                {
                    mTimerCombatMove = 0.1f + 0.1f * static_cast<float>(rand())/RAND_MAX;
                    mCombatMove = true;
                }
                else if(actorCls.isNpc() && (!distantCombat || (distantCombat && distToTarget < rangeMelee/2)))
                {
                    //apply sideway movement (kind of dodging) with some probability
                    if(static_cast<float>(rand())/RAND_MAX < 0.25)
                    {
                        mMovement.mPosition[0] = static_cast<float>(rand())/RAND_MAX < 0.5? 1: -1;
                        mTimerCombatMove = 0.05f + 0.15f * static_cast<float>(rand())/RAND_MAX;
                        mCombatMove = true;
                    }
                }

                if(distantCombat && distToTarget < rangeMelee/4)
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
            bool inLOS;

            if(mReadyToAttack) isStuck = false;
			
            // check if shortcut is available
            if(!isStuck 
                && (!mForceNoShortcut
                || (Ogre::Vector3(mShortcutFailPos.pos) - vActorPos).length() >= PATHFIND_SHORTCUT_RETRY_DIST)
                && (inLOS = MWBase::Environment::get().getWorld()->getLOS(actor, mTarget)))
            {
                if(speed == 0.0f) speed = actorCls.getSpeed(actor);
                // maximum dist before pit/obstacle for actor to avoid them depending on his speed
                float maxAvoidDist = tReaction * speed + speed / MAX_VEL_ANGULAR.valueRadians() * 2;
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

                buildNewPath(actor); //may fail to build a path, check before use

                //delete visited path node
                mPathFinder.checkWaypoint(pos.pos[0],pos.pos[1],pos.pos[2]);

                // This works on the borders between the path grid and areas with no waypoints.
                if(inLOS && mPathFinder.getPath().size() > 1)
                {
                    // get point just before target
                    std::list<ESM::Pathgrid::Point>::iterator pntIter = --mPathFinder.getPath().end();
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

        if(distToTarget > rangeMelee && !distantCombat)
        {
            //special run attack; it shouldn't affect melee combat tactics
            if(actorCls.getMovementSettings(actor).mPosition[1] == 1)
            {
                //check if actor can overcome the distance = distToTarget - attackerWeapRange
                //less than in time of playing weapon anim from 'start' to 'hit' tags (t_swing)
                //then start attacking
                float speed1 = actorCls.getSpeed(actor);
                float speed2 = mTarget.getClass().getSpeed(mTarget);
                if(mTarget.getClass().getMovementSettings(mTarget).mPosition[0] == 0
                        && mTarget.getClass().getMovementSettings(mTarget).mPosition[1] == 0)
                    speed2 = 0;

                float s1 = distToTarget - weapRange;
                float t = s1/speed1;
                float s2 = speed2 * t;
                float t_swing = 0.17f/weapSpeed;//instead of 0.17 should be the time of playing weapon anim from 'start' to 'hit' tags
                if (t + s2/speed1 <= t_swing)
                {
                    mReadyToAttack = true;
                    if(mTimerAttack <= -attackPeriod)
                    {
                        mTimerAttack = 0.3f*static_cast<float>(rand())/RAND_MAX;
                        mStrike = true;
                    }
                }
            }
        }

        return false;
    }

    void AiCombat::buildNewPath(const MWWorld::Ptr& actor)
    {
        Ogre::Vector3 newPathTarget = Ogre::Vector3(mTarget.getRefData().getPosition().pos);

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

    const std::string &AiCombat::getTargetId() const
    {
        return mTarget.getRefData().getHandle();
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
