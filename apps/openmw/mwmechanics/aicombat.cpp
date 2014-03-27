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
        mRotate(false),
        mMovement(),
        mTargetAngle(0)
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

        if (mRotate)
        {
            if (zTurn(actor, Ogre::Degree(mTargetAngle)))
                mRotate = false;
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

        const MWWorld::Class &cls = actor.getClass();
        const ESM::Weapon *weapon = NULL;
        MWMechanics::WeaponType weaptype;
        float weapRange, weapSpeed = 1.0f;

        actor.getClass().getCreatureStats(actor).setMovementFlag(CreatureStats::Flag_Run, true);

        if (actor.getClass().hasInventoryStore(actor))
        {
            MWMechanics::DrawState_ state = actor.getClass().getCreatureStats(actor).getDrawState();
            if (state == MWMechanics::DrawState_Spell || state == MWMechanics::DrawState_Nothing)
                actor.getClass().getCreatureStats(actor).setDrawState(MWMechanics::DrawState_Weapon);

            //Get weapon speed and range
            MWWorld::ContainerStoreIterator weaponSlot =
                MWMechanics::getActiveWeapon(cls.getCreatureStats(actor), cls.getInventoryStore(actor), &weaptype);

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

        ESM::Position pos = actor.getRefData().getPosition();

        float rangeMelee;
        float rangeCloseUp;
        bool distantCombat = false;
        if (weaptype==WeapType_BowAndArrow || weaptype==WeapType_Crossbow || weaptype==WeapType_Thrown)
        {
            rangeMelee = 1000; // TODO: should depend on archer skill
            rangeCloseUp = 0; //doesn't needed when attacking from distance
            distantCombat = true;
        }
        else
        {
            rangeMelee = weapRange;
            rangeCloseUp = 300;
        }

        Ogre::Vector3 vStart(pos.pos[0], pos.pos[1], pos.pos[2]);
        ESM::Position targetPos = mTarget.getRefData().getPosition();
        Ogre::Vector3 vDest(targetPos.pos[0], targetPos.pos[1], targetPos.pos[2]);
        Ogre::Vector3 vDir = vDest - vStart;
        float distBetween = vDir.length();

        if(distBetween < rangeMelee || (distBetween <= rangeCloseUp && mFollowTarget) )
        {
            //Melee and Close-up combat
            vDir.z = 0;
            float dirLen = vDir.length();
            mTargetAngle = Ogre::Radian( Ogre::Math::ACos(vDir.y / dirLen) * sgn(Ogre::Math::ASin(vDir.x / dirLen)) ).valueDegrees();
            mRotate = true;

            //bool LOS = MWBase::Environment::get().getWorld()->getLOS(actor, mTarget);
            if (mFollowTarget && distBetween > rangeMelee)
            {
                //Close-up combat: just run up on target
                mMovement.mPosition[1] = 1;
            }
            else
            {
                //Melee: stop running and attack
                mMovement.mPosition[1] = 0;

                // When attacking with a weapon, choose between slash, thrust or chop
                if (actor.getClass().hasInventoryStore(actor))
                    chooseBestAttack(weapon, mMovement);

                if(mMovement.mPosition[0] || mMovement.mPosition[1])
                {
                    mTimerCombatMove = 0.1f + 0.1f * static_cast<float>(rand())/RAND_MAX;
                    mCombatMove = true;
                }
                else if(actor.getClass().isNpc() && (!distantCombat || (distantCombat && rangeMelee/5)))
                {
                    //apply sideway movement (kind of dodging) with some probability
                    if(static_cast<float>(rand())/RAND_MAX < 0.25)
                    {
                        mMovement.mPosition[0] = static_cast<float>(rand())/RAND_MAX < 0.5? 1: -1;
                        mTimerCombatMove = 0.05f + 0.15f * static_cast<float>(rand())/RAND_MAX;
                        mCombatMove = true;
                    }
                }

                if(distantCombat && distBetween < rangeMelee/4)
                {
                    mMovement.mPosition[1] = -1;
                }

                mReadyToAttack = true;
                //only once got in melee combat, actor is allowed to use close-up shortcutting
                mFollowTarget = true;
            }
        }
        else
        {
            //target is at far distance: build path to target OR follow target (if previously actor had reached it once)
            mFollowTarget = false;

            buildNewPath(actor); //may fail to build a path, check before use

            //delete visited path node
            mPathFinder.checkPathCompleted(pos.pos[0],pos.pos[1],pos.pos[2]);

            //if no new path leave mTargetAngle unchanged
            if(!mPathFinder.getPath().empty())
            {
                //try shortcut
                if(vDir.length() < mPathFinder.getDistToNext(pos.pos[0],pos.pos[1],pos.pos[2]) && MWBase::Environment::get().getWorld()->getLOS(actor, mTarget))
                    mTargetAngle = Ogre::Radian( Ogre::Math::ACos(vDir.y / vDir.length()) * sgn(Ogre::Math::ASin(vDir.x / vDir.length())) ).valueDegrees();
                else
                    mTargetAngle = mPathFinder.getZAngleToNext(pos.pos[0], pos.pos[1]);
                mRotate = true;
            }

            mMovement.mPosition[1] = 1;
            mReadyToAttack = false;
        }

        if(distBetween > rangeMelee)
        {
            //special run attack; it shouldn't affect melee combat tactics
            if(actor.getClass().getMovementSettings(actor).mPosition[1] == 1)
            {
                //check if actor can overcome the distance = distToTarget - attackerWeapRange
                //less than in time of playing weapon anim from 'start' to 'hit' tags (t_swing)
                //then start attacking
                float speed1 = cls.getSpeed(actor);
                float speed2 = mTarget.getClass().getSpeed(mTarget);
                if(mTarget.getClass().getMovementSettings(mTarget).mPosition[0] == 0
                        && mTarget.getClass().getMovementSettings(mTarget).mPosition[1] == 0)
                    speed2 = 0;

                float s1 = distBetween - weapRange;
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

        actor.getClass().getMovementSettings(actor) = mMovement;

        return false;
    }

    void AiCombat::buildNewPath(const MWWorld::Ptr& actor)
    {
        //Construct path to target
        ESM::Pathgrid::Point dest;
        dest.mX = mTarget.getRefData().getPosition().pos[0];
        dest.mY = mTarget.getRefData().getPosition().pos[1];
        dest.mZ = mTarget.getRefData().getPosition().pos[2];
        Ogre::Vector3 newPathTarget = Ogre::Vector3(dest.mX, dest.mY, dest.mZ);

        float dist = -1; //hack to indicate first time, to construct a new path
        if(!mPathFinder.getPath().empty())
        {
            ESM::Pathgrid::Point lastPt = mPathFinder.getPath().back();
            Ogre::Vector3 currPathTarget(lastPt.mX, lastPt.mY, lastPt.mZ);
            dist = Ogre::Math::Abs((newPathTarget - currPathTarget).length());
        }

        float targetPosThreshold;
        bool isOutside = actor.getCell()->getCell()->isExterior();
        if (isOutside)
            targetPosThreshold = 300;
        else
            targetPosThreshold = 100;

        if((dist < 0) || (dist > targetPosThreshold))
        {
            //construct new path only if target has moved away more than on <targetPosThreshold>
            ESM::Position pos = actor.getRefData().getPosition();

            ESM::Pathgrid::Point start;
            start.mX = pos.pos[0];
            start.mY = pos.pos[1];
            start.mZ = pos.pos[2];

            if(!mPathFinder.isPathConstructed())
                mPathFinder.buildPath(start, dest, actor.getCell(), isOutside);
            else
            {
                PathFinder newPathFinder;
                newPathFinder.buildPath(start, dest, actor.getCell(), isOutside);

                //TO EXPLORE:
                //maybe here is a mistake (?): PathFinder::getPathSize() returns number of grid points in the path,
                //not the actual path length. Here we should know if the new path is actually more effective.
                //if(pathFinder2.getPathSize() < mPathFinder.getPathSize())
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
