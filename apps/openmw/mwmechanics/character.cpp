/*
 * OpenMW - The completely unofficial reimplementation of Morrowind
 *
 * This file (character.cpp) is part of the OpenMW package.
 *
 * OpenMW is distributed as free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * version 3, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program. If not, see
 * http://www.gnu.org/licenses/ .
 */

#include "character.hpp"

#include <OgreStringConverter.h>
#include <OgreSceneNode.h>

#include "movement.hpp"
#include "npcstats.hpp"
#include "creaturestats.hpp"
#include "security.hpp"

#include <openengine/misc/rng.hpp>

#include <components/settings/settings.hpp>

#include "../mwrender/animation.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/statemanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/esmstore.hpp"

namespace
{

// Wraps a value to (-PI, PI]
void wrap(Ogre::Radian& rad)
{
    if (rad.valueRadians()>0)
        rad = Ogre::Radian(std::fmod(rad.valueRadians()+Ogre::Math::PI, 2.0f*Ogre::Math::PI)-Ogre::Math::PI);
    else
        rad = Ogre::Radian(std::fmod(rad.valueRadians()-Ogre::Math::PI, 2.0f*Ogre::Math::PI)+Ogre::Math::PI);
}

std::string getBestAttack (const ESM::Weapon* weapon)
{
    int slash = (weapon->mData.mSlash[0] + weapon->mData.mSlash[1])/2;
    int chop = (weapon->mData.mChop[0] + weapon->mData.mChop[1])/2;
    int thrust = (weapon->mData.mThrust[0] + weapon->mData.mThrust[1])/2;
    if (slash >= chop && slash >= thrust)
        return "slash";
    else if (chop >= slash && chop >= thrust)
        return "chop";
    else
        return "thrust";
}

// Converts a movement Run state to its equivalent Walk state.
MWMechanics::CharacterState runStateToWalkState (MWMechanics::CharacterState state)
{
    using namespace MWMechanics;
    CharacterState ret = state;
    switch (state)
    {
        case CharState_RunForward:
            ret = CharState_WalkForward;
            break;
        case CharState_RunBack:
            ret = CharState_WalkBack;
            break;
        case CharState_RunLeft:
            ret = CharState_WalkLeft;
            break;
        case CharState_RunRight:
            ret = CharState_WalkRight;
            break;
        case CharState_SwimRunForward:
            ret = CharState_SwimWalkForward;
            break;
        case CharState_SwimRunBack:
            ret = CharState_SwimWalkBack;
            break;
        case CharState_SwimRunLeft:
            ret = CharState_SwimWalkLeft;
            break;
        case CharState_SwimRunRight:
            ret = CharState_SwimWalkRight;
            break;
        default:
            break;
    }
    return ret;
}

float getFallDamage(const MWWorld::Ptr& ptr, float fallHeight)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    const MWWorld::Store<ESM::GameSetting> &store = world->getStore().get<ESM::GameSetting>();

    const float fallDistanceMin = store.find("fFallDamageDistanceMin")->getFloat();

    if (fallHeight >= fallDistanceMin)
    {
        const float acrobaticsSkill = static_cast<float>(ptr.getClass().getSkill(ptr, ESM::Skill::Acrobatics));
        const float jumpSpellBonus = ptr.getClass().getCreatureStats(ptr).getMagicEffects().get(ESM::MagicEffect::Jump).getMagnitude();
        const float fallAcroBase = store.find("fFallAcroBase")->getFloat();
        const float fallAcroMult = store.find("fFallAcroMult")->getFloat();
        const float fallDistanceBase = store.find("fFallDistanceBase")->getFloat();
        const float fallDistanceMult = store.find("fFallDistanceMult")->getFloat();

        float x = fallHeight - fallDistanceMin;
        x -= (1.5f * acrobaticsSkill) + jumpSpellBonus;
        x = std::max(0.0f, x);

        float a = fallAcroBase + fallAcroMult * (100 - acrobaticsSkill);
        x = fallDistanceBase + fallDistanceMult * x;
        x *= a;

        return x;
    }
    return 0.f;
}

}

namespace MWMechanics
{

struct StateInfo {
    CharacterState state;
    const char groupname[32];
};

static const StateInfo sMovementList[] = {
    { CharState_WalkForward, "walkforward" },
    { CharState_WalkBack, "walkback" },
    { CharState_WalkLeft, "walkleft" },
    { CharState_WalkRight, "walkright" },

    { CharState_SwimWalkForward, "swimwalkforward" },
    { CharState_SwimWalkBack, "swimwalkback" },
    { CharState_SwimWalkLeft, "swimwalkleft" },
    { CharState_SwimWalkRight, "swimwalkright" },

    { CharState_RunForward, "runforward" },
    { CharState_RunBack, "runback" },
    { CharState_RunLeft, "runleft" },
    { CharState_RunRight, "runright" },

    { CharState_SwimRunForward, "swimrunforward" },
    { CharState_SwimRunBack, "swimrunback" },
    { CharState_SwimRunLeft, "swimrunleft" },
    { CharState_SwimRunRight, "swimrunright" },

    { CharState_SneakForward, "sneakforward" },
    { CharState_SneakBack, "sneakback" },
    { CharState_SneakLeft, "sneakleft" },
    { CharState_SneakRight, "sneakright" },

    { CharState_Jump, "jump" },

    { CharState_TurnLeft, "turnleft" },
    { CharState_TurnRight, "turnright" },
};
static const StateInfo *sMovementListEnd = &sMovementList[sizeof(sMovementList)/sizeof(sMovementList[0])];


class FindCharState {
    CharacterState state;

public:
    FindCharState(CharacterState _state) : state(_state) { }

    bool operator()(const StateInfo &info) const
    { return info.state == state; }
};


static const struct WeaponInfo {
    WeaponType type;
    const char shortgroup[16];
    const char longgroup[16];
} sWeaponTypeList[] = {
    { WeapType_HandToHand, "hh", "handtohand" },
    { WeapType_OneHand, "1h", "weapononehand" },
    { WeapType_TwoHand, "2c", "weapontwohand" },
    { WeapType_TwoWide, "2w", "weapontwowide" },
    { WeapType_BowAndArrow, "1h", "bowandarrow" },
    { WeapType_Crossbow, "crossbow", "crossbow" },
    { WeapType_Thrown, "1h", "throwweapon" },
    { WeapType_PickProbe, "1h", "pickprobe" },
    { WeapType_Spell, "spell", "spellcast" },
};
static const WeaponInfo *sWeaponTypeListEnd = &sWeaponTypeList[sizeof(sWeaponTypeList)/sizeof(sWeaponTypeList[0])];

class FindWeaponType {
    WeaponType type;

public:
    FindWeaponType(WeaponType _type) : type(_type) { }

    bool operator()(const WeaponInfo &weap) const
    { return weap.type == type; }
};

std::string CharacterController::chooseRandomGroup (const std::string& prefix, int* num)
{
    int numAnims=0;
    while (mAnimation->hasAnimation(prefix + Ogre::StringConverter::toString(numAnims+1)))
        ++numAnims;

    int roll = OEngine::Misc::Rng::rollDice(numAnims) + 1; // [1, numAnims]
    if (num)
        *num = roll;
    return prefix + Ogre::StringConverter::toString(roll);
}

void CharacterController::refreshCurrentAnims(CharacterState idle, CharacterState movement, bool force)
{
    // hit recoils/knockdown animations handling
    if(mPtr.getClass().isActor())
    {
        bool recovery = mPtr.getClass().getCreatureStats(mPtr).getHitRecovery();
        bool knockdown = mPtr.getClass().getCreatureStats(mPtr).getKnockedDown();
        bool block = mPtr.getClass().getCreatureStats(mPtr).getBlock();
        if(mHitState == CharState_None)
        {
            if (mPtr.getClass().getCreatureStats(mPtr).getFatigue().getCurrent() < 0
                    || mPtr.getClass().getCreatureStats(mPtr).getFatigue().getBase() == 0)
            {
                mHitState = CharState_KnockOut;
                mCurrentHit = "knockout";
                mAnimation->play(mCurrentHit, Priority_Knockdown, MWRender::Animation::Group_All, false, 1, "start", "stop", 0.0f, ~0ul);
                mPtr.getClass().getCreatureStats(mPtr).setKnockedDown(true);
            }
            else if(knockdown)
            {
                mHitState = CharState_KnockDown;
                mCurrentHit = "knockdown";
                mAnimation->play(mCurrentHit, Priority_Knockdown, MWRender::Animation::Group_All, true, 1, "start", "stop", 0.0f, 0);
            }
            else if (recovery)
            {
                mHitState = CharState_Hit;
                mCurrentHit = chooseRandomGroup("hit");
                mAnimation->play(mCurrentHit, Priority_Hit, MWRender::Animation::Group_All, true, 1, "start", "stop", 0.0f, 0);
            }
            else if (block)
            {
                mHitState = CharState_Block;
                mCurrentHit = "shield";
                mAnimation->play(mCurrentHit, Priority_Hit, MWRender::Animation::Group_All, true, 1, "block start", "block stop", 0.0f, 0);
            }

            // Cancel upper body animations
            if (mHitState == CharState_KnockDown || mHitState == CharState_KnockOut)
            {
                if (mUpperBodyState > UpperCharState_WeapEquiped)
                {
                    mAnimation->disable(mCurrentWeapon);
                    mUpperBodyState = UpperCharState_WeapEquiped;
                }
                else if (mUpperBodyState > UpperCharState_Nothing && mUpperBodyState < UpperCharState_WeapEquiped)
                {
                    mAnimation->disable(mCurrentWeapon);
                    mUpperBodyState = UpperCharState_Nothing;
                }
            }
        }
        else if(!mAnimation->isPlaying(mCurrentHit))
        {
            mCurrentHit.erase();
            if (knockdown)
                mPtr.getClass().getCreatureStats(mPtr).setKnockedDown(false);
            if (recovery)
                mPtr.getClass().getCreatureStats(mPtr).setHitRecovery(false);
            if (block)
                mPtr.getClass().getCreatureStats(mPtr).setBlock(false);
            mHitState = CharState_None;
        }
        else if (mHitState == CharState_KnockOut && mPtr.getClass().getCreatureStats(mPtr).getFatigue().getCurrent() > 0)
        {
            mHitState = CharState_KnockDown;
            mAnimation->disable(mCurrentHit);
            mAnimation->play(mCurrentHit, Priority_Knockdown, MWRender::Animation::Group_All, true, 1, "loop stop", "stop", 0.0f, 0);
        }
    }

    const WeaponInfo *weap = std::find_if(sWeaponTypeList, sWeaponTypeListEnd, FindWeaponType(mWeaponType));
    if (!mPtr.getClass().isBipedal(mPtr))
        weap = sWeaponTypeListEnd;

    if(force && mJumpState != JumpState_None)
    {
        std::string jump;
        MWRender::Animation::Group jumpgroup = MWRender::Animation::Group_All;
        if(mJumpState != JumpState_None)
        {
            jump = "jump";
            if(weap != sWeaponTypeListEnd)
            {
                jump += weap->shortgroup;
                if(!mAnimation->hasAnimation(jump))
                {
                    jumpgroup = MWRender::Animation::Group_LowerBody;
                    jump = "jump";
                }
            }
        }

        if(mJumpState == JumpState_InAir)
        {
            int mode = ((jump == mCurrentJump) ? 2 : 1);

            mAnimation->disable(mCurrentJump);
            mCurrentJump = jump;
            if (mAnimation->hasAnimation("jump"))
                mAnimation->play(mCurrentJump, Priority_Jump, jumpgroup, false,
                             1.0f, ((mode!=2)?"start":"loop start"), "stop", 0.0f, ~0ul);
        }
        else
        {
            mAnimation->disable(mCurrentJump);
            mCurrentJump.clear();
            if (mAnimation->hasAnimation("jump"))
                mAnimation->play(jump, Priority_Jump, jumpgroup, true,
                             1.0f, "loop stop", "stop", 0.0f, 0);
        }
    }

    if(force || movement != mMovementState)
    {
        mMovementState = movement;

        std::string movement;
        MWRender::Animation::Group movegroup = MWRender::Animation::Group_All;
        const StateInfo *movestate = std::find_if(sMovementList, sMovementListEnd, FindCharState(mMovementState));
        if(movestate != sMovementListEnd)
        {
            movement = movestate->groupname;
            if(weap != sWeaponTypeListEnd && movement.find("swim") == std::string::npos)
            {
                movement += weap->shortgroup;
                if(!mAnimation->hasAnimation(movement))
                {
                    movegroup = MWRender::Animation::Group_LowerBody;
                    movement = movestate->groupname;
                }
            }

            if(!mAnimation->hasAnimation(movement))
            {
                std::string::size_type swimpos = movement.find("swim");
                if(swimpos == std::string::npos)
                {
                    std::string::size_type runpos = movement.find("run");
                    if (runpos != std::string::npos)
                    {
                        movement.replace(runpos, runpos+3, "walk");
                        if (!mAnimation->hasAnimation(movement))
                            movement.clear();
                    }
                    else
                        movement.clear();
                }
                else
                {
                    movegroup = MWRender::Animation::Group_LowerBody;
                    movement.erase(swimpos, 4);
                    if(!mAnimation->hasAnimation(movement))
                        movement.clear();
                }
            }
        }

        /* If we're playing the same animation, restart from the loop start instead of the
         * beginning. */
        int mode = ((movement == mCurrentMovement) ? 2 : 1);

        mMovementAnimationControlled = true;

        mAnimation->disable(mCurrentMovement);
        mCurrentMovement = movement;
        if(!mCurrentMovement.empty())
        {
            float vel, speedmult = 1.0f;

            bool isrunning = mPtr.getClass().getCreatureStats(mPtr).getStance(MWMechanics::CreatureStats::Stance_Run)
                    && !MWBase::Environment::get().getWorld()->isFlying(mPtr);

            // For non-flying creatures, MW uses the Walk animation to calculate the animation velocity
            // even if we are running. This must be replicated, otherwise the observed speed would differ drastically.
            std::string anim = mCurrentMovement;
            if (mPtr.getClass().getTypeName() == typeid(ESM::Creature).name()
                    && !(mPtr.get<ESM::Creature>()->mBase->mFlags & ESM::Creature::Flies))
            {
                CharacterState walkState = runStateToWalkState(mMovementState);
                const StateInfo *stateinfo = std::find_if(sMovementList, sMovementListEnd, FindCharState(walkState));
                anim = stateinfo->groupname;

                if (mMovementSpeed > 0.0f && (vel=mAnimation->getVelocity(anim)) > 1.0f)
                    speedmult = mMovementSpeed / vel;
                else
                    // Another bug: when using a fallback animation (e.g. RunForward as fallback to SwimRunForward),
                    // then the equivalent Walk animation will not use a fallback, and if that animation doesn't exist
                    // we will play without any scaling.
                    // Makes the speed attribute of most water creatures totally useless.
                    // And again, this can not be fixed without patching game data.
                    speedmult = 1.f;
            }
            else
            {
                if(mMovementSpeed > 0.0f && (vel=mAnimation->getVelocity(anim)) > 1.0f)
                {
                    speedmult = mMovementSpeed / vel;
                }
                else if (mMovementState == CharState_TurnLeft || mMovementState == CharState_TurnRight)
                    speedmult = 1.f; // adjusted each frame
                else if (mMovementSpeed > 0.0f)
                {
                    // The first person anims don't have any velocity to calculate a speed multiplier from.
                    // We use the third person velocities instead.
                    // FIXME: should be pulled from the actual animation, but it is not presently loaded.
                    speedmult = mMovementSpeed / (isrunning ? 222.857f : 154.064f);
                    mMovementAnimationControlled = false;
                }
            }

            mAnimation->play(mCurrentMovement, Priority_Movement, movegroup, false,
                             speedmult, ((mode!=2)?"start":"loop start"), "stop", 0.0f, ~0ul);
        }
    }

    // idle handled last as it can depend on the other states
    // FIXME: if one of the below states is close to their last animation frame (i.e. will be disabled in the coming update),
    // the idle animation should be displayed
    if ((mUpperBodyState != UpperCharState_Nothing
            || mMovementState != CharState_None
            || mHitState != CharState_None)
            && !mPtr.getClass().isBipedal(mPtr))
        idle = CharState_None;

    if(force || idle != mIdleState)
    {
        mIdleState = idle;

        std::string idle;
        // Only play "idleswim" or "idlesneak" if they exist. Otherwise, fallback to
        // "idle"+weapon or "idle".
        if(mIdleState == CharState_IdleSwim && mAnimation->hasAnimation("idleswim"))
            idle = "idleswim";
        else if(mIdleState == CharState_IdleSneak && mAnimation->hasAnimation("idlesneak"))
            idle = "idlesneak";
        else if(mIdleState != CharState_None)
        {
            idle = "idle";
            if(weap != sWeaponTypeListEnd)
            {
                idle += weap->shortgroup;
                if(!mAnimation->hasAnimation(idle))
                    idle = "idle";
            }
        }

        mAnimation->disable(mCurrentIdle);
        mCurrentIdle = idle;
        if(!mCurrentIdle.empty())
            mAnimation->play(mCurrentIdle, Priority_Default, MWRender::Animation::Group_All, false,
                             1.0f, "start", "stop", 0.0f, ~0ul, true);
    }

    updateIdleStormState();
}


void getWeaponGroup(WeaponType weaptype, std::string &group)
{
    const WeaponInfo *info = std::find_if(sWeaponTypeList, sWeaponTypeListEnd, FindWeaponType(weaptype));
    if(info != sWeaponTypeListEnd)
        group = info->longgroup;
}


MWWorld::ContainerStoreIterator getActiveWeapon(CreatureStats &stats, MWWorld::InventoryStore &inv, WeaponType *weaptype)
{
    if(stats.getDrawState() == DrawState_Spell)
    {
        *weaptype = WeapType_Spell;
        return inv.end();
    }

    if(stats.getDrawState() == MWMechanics::DrawState_Weapon)
    {
        MWWorld::ContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        if(weapon == inv.end())
            *weaptype = WeapType_HandToHand;
        else
        {
            const std::string &type = weapon->getTypeName();
            if(type == typeid(ESM::Lockpick).name() || type == typeid(ESM::Probe).name())
                *weaptype = WeapType_PickProbe;
            else if(type == typeid(ESM::Weapon).name())
            {
                MWWorld::LiveCellRef<ESM::Weapon> *ref = weapon->get<ESM::Weapon>();
                ESM::Weapon::Type type = (ESM::Weapon::Type)ref->mBase->mData.mType;
                switch(type)
                {
                    case ESM::Weapon::ShortBladeOneHand:
                    case ESM::Weapon::LongBladeOneHand:
                    case ESM::Weapon::BluntOneHand:
                    case ESM::Weapon::AxeOneHand:
                    case ESM::Weapon::Arrow:
                    case ESM::Weapon::Bolt:
                        *weaptype = WeapType_OneHand;
                        break;
                    case ESM::Weapon::LongBladeTwoHand:
                    case ESM::Weapon::BluntTwoClose:
                    case ESM::Weapon::AxeTwoHand:
                        *weaptype = WeapType_TwoHand;
                        break;
                    case ESM::Weapon::BluntTwoWide:
                    case ESM::Weapon::SpearTwoWide:
                        *weaptype = WeapType_TwoWide;
                        break;
                    case ESM::Weapon::MarksmanBow:
                        *weaptype = WeapType_BowAndArrow;
                        break;
                    case ESM::Weapon::MarksmanCrossbow:
                        *weaptype = WeapType_Crossbow;
                        break;
                    case ESM::Weapon::MarksmanThrown:
                        *weaptype = WeapType_Thrown;
                        break;
                }
            }
        }

        return weapon;
    }

    return inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
}

void CharacterController::playDeath(float startpoint, CharacterState death)
{
    switch (death)
    {
    case CharState_SwimDeath:
        mCurrentDeath = "swimdeath";
        break;
    case CharState_DeathKnockDown:
        mCurrentDeath = "deathknockdown";
        break;
    case CharState_DeathKnockOut:
        mCurrentDeath = "deathknockout";
        break;
    default:
        mCurrentDeath = "death" + Ogre::StringConverter::toString(death - CharState_Death1 + 1);
    }
    mDeathState = death;

    mPtr.getClass().getCreatureStats(mPtr).setDeathAnimation(mDeathState - CharState_Death1);

    // For dead actors, refreshCurrentAnims is no longer called, so we need to disable the movement state manually.
    // Note that these animations wouldn't actually be visible (due to the Death animation's priority being higher).
    // However, they could still trigger text keys, such as Hit events, or sounds.
    mMovementState = CharState_None;
    mAnimation->disable(mCurrentMovement);
    mCurrentMovement = "";
    mUpperBodyState = UpperCharState_Nothing;
    mAnimation->disable(mCurrentWeapon);
    mCurrentWeapon = "";
    mHitState = CharState_None;
    mAnimation->disable(mCurrentHit);
    mCurrentHit = "";
    mIdleState = CharState_None;
    mAnimation->disable(mCurrentIdle);
    mCurrentIdle = "";
    mJumpState = JumpState_None;
    mAnimation->disable(mCurrentJump);
    mCurrentJump = "";
    mMovementAnimationControlled = true;

    mAnimation->play(mCurrentDeath, Priority_Death, MWRender::Animation::Group_All,
                    false, 1.0f, "start", "stop", startpoint, 0);
}

void CharacterController::playRandomDeath(float startpoint)
{
    if (mPtr == MWBase::Environment::get().getWorld()->getPlayerPtr())
    {
        // The first-person animations do not include death, so we need to
        // force-switch to third person before playing the death animation.
        MWBase::Environment::get().getWorld()->useDeathCamera();
    }

    if(MWBase::Environment::get().getWorld()->isSwimming(mPtr) && mAnimation->hasAnimation("swimdeath"))
    {
        mDeathState = CharState_SwimDeath;
    }
    else if (mHitState == CharState_KnockDown && mAnimation->hasAnimation("deathknockdown"))
    {
        mDeathState = CharState_DeathKnockDown;
    }
    else if (mHitState == CharState_KnockOut && mAnimation->hasAnimation("deathknockout"))
    {
        mDeathState = CharState_DeathKnockOut;
    }
    else
    {
        int selected=0;
        chooseRandomGroup("death", &selected);
        mDeathState = static_cast<CharacterState>(CharState_Death1 + (selected-1));
    }
    playDeath(startpoint, mDeathState);
}

CharacterController::CharacterController(const MWWorld::Ptr &ptr, MWRender::Animation *anim)
    : mPtr(ptr)
    , mAnimation(anim)
    , mIdleState(CharState_None)
    , mMovementState(CharState_None)
    , mMovementSpeed(0.0f)
    , mHasMovedInXY(false)
    , mMovementAnimationControlled(true)
    , mDeathState(CharState_None)
    , mHitState(CharState_None)
    , mUpperBodyState(UpperCharState_Nothing)
    , mJumpState(JumpState_None)
    , mWeaponType(WeapType_None)
    , mSkipAnim(false)
    , mSecondsOfRunning(0)
    , mSecondsOfSwimming(0)
    , mTurnAnimationThreshold(0)
{
    if(!mAnimation)
        return;

    const MWWorld::Class &cls = mPtr.getClass();
    if(cls.isActor())
    {
        /* Accumulate along X/Y only for now, until we can figure out how we should
         * handle knockout and death which moves the character down. */
        mAnimation->setAccumulation(Ogre::Vector3(1.0f, 1.0f, 0.0f));

        if (cls.hasInventoryStore(mPtr))
        {
            getActiveWeapon(cls.getCreatureStats(mPtr), cls.getInventoryStore(mPtr), &mWeaponType);
            if (mWeaponType != WeapType_None)
            {
                mUpperBodyState = UpperCharState_WeapEquiped;
                getWeaponGroup(mWeaponType, mCurrentWeapon);
            }

            if(mWeaponType != WeapType_None && mWeaponType != WeapType_Spell && mWeaponType != WeapType_HandToHand)
            {
                mAnimation->showWeapons(true);
                mAnimation->setWeaponGroup(mCurrentWeapon);
            }

            mAnimation->showCarriedLeft(updateCarriedLeftVisible(mWeaponType));
        }

        if(!cls.getCreatureStats(mPtr).isDead())
            mIdleState = CharState_Idle;
        else
        {
            int deathindex = mPtr.getClass().getCreatureStats(mPtr).getDeathAnimation();
            playDeath(1.0f, CharacterState(CharState_Death1 + deathindex));
        }
    }
    else
    {
        /* Don't accumulate with non-actors. */
        mAnimation->setAccumulation(Ogre::Vector3(0.0f));

        mIdleState = CharState_Idle;
    }


    if(mDeathState == CharState_None)
        refreshCurrentAnims(mIdleState, mMovementState, true);

    mAnimation->runAnimation(0.f);
}

CharacterController::~CharacterController()
{
}


void CharacterController::updatePtr(const MWWorld::Ptr &ptr)
{
    mPtr = ptr;
}

void CharacterController::updateIdleStormState()
{
    bool inStormDirection = false;
    if (MWBase::Environment::get().getWorld()->isInStorm())
    {
        Ogre::Vector3 stormDirection = MWBase::Environment::get().getWorld()->getStormDirection();
        Ogre::Vector3 characterDirection = mPtr.getRefData().getBaseNode()->getOrientation().yAxis();
        inStormDirection = stormDirection.angleBetween(characterDirection) > Ogre::Degree(120);
    }
    if (inStormDirection && mUpperBodyState == UpperCharState_Nothing && mAnimation->hasAnimation("idlestorm"))
    {
        float complete = 0;
        mAnimation->getInfo("idlestorm", &complete);

        if (complete == 0)
            mAnimation->play("idlestorm", Priority_Storm, MWRender::Animation::Group_RightArm, false,
                             1.0f, "start", "loop start", 0.0f, 0);
        else if (complete == 1)
            mAnimation->play("idlestorm", Priority_Storm, MWRender::Animation::Group_RightArm, false,
                             1.0f, "loop start", "loop stop", 0.0f, ~0ul);
    }
    else
    {
        if (mUpperBodyState == UpperCharState_Nothing)
        {
            if (mAnimation->isPlaying("idlestorm"))
            {
                if (mAnimation->getCurrentTime("idlestorm") < mAnimation->getTextKeyTime("idlestorm: loop stop"))
                {
                    mAnimation->play("idlestorm", Priority_Storm, MWRender::Animation::Group_RightArm, true,
                                     1.0f, "loop stop", "stop", 0.0f, 0);
                }
            }
        }
        else
            mAnimation->disable("idlestorm");
    }
}

void CharacterController::castSpell(const std::string &spellid)
{
    static const std::string schools[] = {
        "alteration", "conjuration", "destruction", "illusion", "mysticism", "restoration"
    };

    const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
    const ESM::Spell *spell = store.get<ESM::Spell>().find(spellid);
    const ESM::ENAMstruct &effectentry = spell->mEffects.mList.at(0);

    const ESM::MagicEffect *effect;
    effect = store.get<ESM::MagicEffect>().find(effectentry.mEffectID);

    const ESM::Static* castStatic;
    if (!effect->mCasting.empty())
        castStatic = store.get<ESM::Static>().find (effect->mCasting);
    else
        castStatic = store.get<ESM::Static>().find ("VFX_DefaultCast");

    mAnimation->addEffect("meshes\\" + castStatic->mModel, effect->mIndex);

    MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
    if(!effect->mCastSound.empty())
        sndMgr->playSound3D(mPtr, effect->mCastSound, 1.0f, 1.0f);
    else
        sndMgr->playSound3D(mPtr, schools[effect->mData.mSchool]+" cast", 1.0f, 1.0f);
}

bool CharacterController::updateCreatureState()
{
    const MWWorld::Class &cls = mPtr.getClass();
    CreatureStats &stats = cls.getCreatureStats(mPtr);

    WeaponType weapType = WeapType_None;
    if(stats.getDrawState() == DrawState_Weapon)
        weapType = WeapType_HandToHand;
    else if (stats.getDrawState() == DrawState_Spell)
        weapType = WeapType_Spell;

    if (weapType != mWeaponType)
    {
        mWeaponType = weapType;
        if (mAnimation->isPlaying(mCurrentWeapon))
            mAnimation->disable(mCurrentWeapon);
    }

    if(stats.getAttackingOrSpell())
    {
        if(mUpperBodyState == UpperCharState_Nothing && mHitState == CharState_None)
        {
            MWBase::Environment::get().getWorld()->breakInvisibility(mPtr);

            std::string startKey = "start";
            std::string stopKey = "stop";
            if (weapType == WeapType_Spell)
            {
                const std::string spellid = stats.getSpells().getSelectedSpell();
                if (!spellid.empty() && MWBase::Environment::get().getWorld()->startSpellCast(mPtr))
                {
                    castSpell(spellid);

                    if (!mAnimation->hasAnimation("spellcast"))
                        MWBase::Environment::get().getWorld()->castSpell(mPtr); // No "release" text key to use, so cast immediately
                    else
                    {
                        const ESM::Spell *spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellid);
                        const ESM::ENAMstruct &effectentry = spell->mEffects.mList.at(0);

                        switch(effectentry.mRange)
                        {
                            case 0: mAttackType = "self"; break;
                            case 1: mAttackType = "touch"; break;
                            case 2: mAttackType = "target"; break;
                        }

                        startKey = mAttackType + " " + startKey;
                        stopKey = mAttackType + " " + stopKey;
                        mCurrentWeapon = "spellcast";
                    }
                }
                else
                    mCurrentWeapon = "";
            }
            if (weapType != WeapType_Spell || !mAnimation->hasAnimation("spellcast")) // Not all creatures have a dedicated spellcast animation
            {
                int roll = OEngine::Misc::Rng::rollDice(3); // [0, 2]
                if (roll == 0)
                    mCurrentWeapon = "attack1";
                else if (roll == 1)
                    mCurrentWeapon = "attack2";
                else
                    mCurrentWeapon = "attack3";
            }

            if (!mCurrentWeapon.empty())
            {
                mAnimation->play(mCurrentWeapon, Priority_Weapon,
                                 MWRender::Animation::Group_All, true,
                                 1, startKey, stopKey,
                                 0.0f, 0);
                mUpperBodyState = UpperCharState_StartToMinAttack;
            }
        }

        stats.setAttackingOrSpell(false);
    }

    bool animPlaying = mAnimation->getInfo(mCurrentWeapon);
    if (!animPlaying)
        mUpperBodyState = UpperCharState_Nothing;
    return false;
}

bool CharacterController::updateCarriedLeftVisible(WeaponType weaptype) const
{
    // Shields/torches shouldn't be visible during any operation involving two hands
    // There seems to be no text keys for this purpose, except maybe for "[un]equip start/stop",
    // but they are also present in weapon drawing animation.
    switch (weaptype)
    {
    case WeapType_Spell:
    case WeapType_BowAndArrow:
    case WeapType_Crossbow:
    case WeapType_HandToHand:
    case WeapType_TwoHand:
    case WeapType_TwoWide:
        return false;
    default:
        return true;
    }
}

bool CharacterController::updateWeaponState()
{
    const MWWorld::Class &cls = mPtr.getClass();
    CreatureStats &stats = cls.getCreatureStats(mPtr);
    WeaponType weaptype = WeapType_None;
    if(stats.getDrawState() == DrawState_Weapon)
        weaptype = WeapType_HandToHand;
    else if (stats.getDrawState() == DrawState_Spell)
        weaptype = WeapType_Spell;

    const bool isWerewolf = cls.isNpc() && cls.getNpcStats(mPtr).isWerewolf();

    std::string soundid;
    if (mPtr.getClass().hasInventoryStore(mPtr))
    {
        MWWorld::InventoryStore &inv = cls.getInventoryStore(mPtr);
        MWWorld::ContainerStoreIterator weapon = getActiveWeapon(stats, inv, &weaptype);
        if(weapon != inv.end() && !(weaptype == WeapType_None && mWeaponType == WeapType_Spell))
        {
            soundid = (weaptype == WeapType_None) ?
                                   weapon->getClass().getDownSoundId(*weapon) :
                                   weapon->getClass().getUpSoundId(*weapon);
        }
    }

    bool forcestateupdate = false;
    if(weaptype != mWeaponType && mHitState != CharState_KnockDown && mHitState != CharState_KnockOut
                                && mHitState != CharState_Hit)
    {
        forcestateupdate = true;

        mAnimation->showCarriedLeft(updateCarriedLeftVisible(weaptype));

        std::string weapgroup;
        if(weaptype == WeapType_None)
        {
            getWeaponGroup(mWeaponType, weapgroup);
            mAnimation->play(weapgroup, Priority_Weapon,
                             MWRender::Animation::Group_UpperBody, true,
                             1.0f, "unequip start", "unequip stop", 0.0f, 0);
            mUpperBodyState = UpperCharState_UnEquipingWeap;
        }
        else
        {
            getWeaponGroup(weaptype, weapgroup);
            mAnimation->showWeapons(false);
            mAnimation->setWeaponGroup(weapgroup);

            mAnimation->play(weapgroup, Priority_Weapon,
                             MWRender::Animation::Group_UpperBody, true,
                             1.0f, "equip start", "equip stop", 0.0f, 0);
            mUpperBodyState = UpperCharState_EquipingWeap;

            if(isWerewolf)
            {
                const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
                const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfEquip");
                if(sound)
                {
                    MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
                    sndMgr->playSound3D(mPtr, sound->mId, 1.0f, 1.0f);
                }
            }
        }

        if(!soundid.empty())
        {
            MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
            sndMgr->playSound3D(mPtr, soundid, 1.0f, 1.0f);
        }

        mWeaponType = weaptype;
        getWeaponGroup(mWeaponType, mCurrentWeapon);
    }

    if(isWerewolf)
    {
        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        if(cls.getCreatureStats(mPtr).getStance(MWMechanics::CreatureStats::Stance_Run)
            && mHasMovedInXY
            && !MWBase::Environment::get().getWorld()->isSwimming(mPtr)
            && mWeaponType == WeapType_None)
        {
            if(!sndMgr->getSoundPlaying(mPtr, "WolfRun"))
                sndMgr->playSound3D(mPtr, "WolfRun", 1.0f, 1.0f, MWBase::SoundManager::Play_TypeSfx,
                                    MWBase::SoundManager::Play_Loop);
        }
        else
            sndMgr->stopSound3D(mPtr, "WolfRun");
    }

    // Cancel attack if we no longer have ammunition
    bool ammunition = true;
    bool isWeapon = false;
    float weapSpeed = 1.f;
    if (mPtr.getClass().hasInventoryStore(mPtr))
    {
        MWWorld::InventoryStore &inv = cls.getInventoryStore(mPtr);
        MWWorld::ContainerStoreIterator weapon = getActiveWeapon(stats, inv, &weaptype);
        isWeapon = (weapon != inv.end() && weapon->getTypeName() == typeid(ESM::Weapon).name());
        if(isWeapon)
            weapSpeed = weapon->get<ESM::Weapon>()->mBase->mData.mSpeed;

        MWWorld::ContainerStoreIterator ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);
        if (mWeaponType == WeapType_Crossbow)
            ammunition = (ammo != inv.end() && ammo->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::Bolt);
        else if (mWeaponType == WeapType_BowAndArrow)
            ammunition = (ammo != inv.end() && ammo->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::Arrow);
        if (!ammunition && mUpperBodyState > UpperCharState_WeapEquiped)
        {
            mAnimation->disable(mCurrentWeapon);
            mUpperBodyState = UpperCharState_WeapEquiped;
        }
    }

    float complete;
    bool animPlaying;
    if(stats.getAttackingOrSpell())
    {
        if(mUpperBodyState == UpperCharState_WeapEquiped && mHitState == CharState_None)
        {
            MWBase::Environment::get().getWorld()->breakInvisibility(mPtr);
            mAttackType.clear();
            if(mWeaponType == WeapType_Spell)
            {
                // Unset casting flag, otherwise pressing the mouse button down would
                // continue casting every frame if there is no animation
                stats.setAttackingOrSpell(false);

                const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();

                // For the player, set the spell we want to cast
                // This has to be done at the start of the casting animation,
                // *not* when selecting a spell in the GUI (otherwise you could change the spell mid-animation)
                if (mPtr == MWBase::Environment::get().getWorld()->getPlayerPtr())
                {
                    std::string selectedSpell = MWBase::Environment::get().getWindowManager()->getSelectedSpell();
                    stats.getSpells().setSelectedSpell(selectedSpell);
                }
                std::string spellid = stats.getSpells().getSelectedSpell();

                if(!spellid.empty() && MWBase::Environment::get().getWorld()->startSpellCast(mPtr))
                {
                    castSpell(spellid);

                    const ESM::Spell *spell = store.get<ESM::Spell>().find(spellid);
                    const ESM::ENAMstruct &effectentry = spell->mEffects.mList.at(0);

                    const ESM::MagicEffect *effect;
                    effect = store.get<ESM::MagicEffect>().find(effectentry.mEffectID);

                    const ESM::Static* castStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find ("VFX_Hands");
                    if (mAnimation->getNode("Left Hand"))
                        mAnimation->addEffect("meshes\\" + castStatic->mModel, -1, false, "Left Hand", effect->mParticle);
                    else
                        mAnimation->addEffect("meshes\\" + castStatic->mModel, -1, false, "Bip01 L Hand", effect->mParticle);

                    if (mAnimation->getNode("Right Hand"))
                        mAnimation->addEffect("meshes\\" + castStatic->mModel, -1, false, "Right Hand", effect->mParticle);
                    else
                        mAnimation->addEffect("meshes\\" + castStatic->mModel, -1, false, "Bip01 R Hand", effect->mParticle);

                    switch(effectentry.mRange)
                    {
                        case 0: mAttackType = "self"; break;
                        case 1: mAttackType = "touch"; break;
                        case 2: mAttackType = "target"; break;
                    }

                    mAnimation->play(mCurrentWeapon, Priority_Weapon,
                                     MWRender::Animation::Group_UpperBody, true,
                                     weapSpeed, mAttackType+" start", mAttackType+" stop",
                                     0.0f, 0);
                    mUpperBodyState = UpperCharState_CastingSpell;
                }
                if (mPtr.getClass().hasInventoryStore(mPtr))
                {
                    MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
                    if (inv.getSelectedEnchantItem() != inv.end())
                    {
                        // Enchanted items cast immediately (no animation)
                        MWBase::Environment::get().getWorld()->castSpell(mPtr);
                    }
                }

            }
            else if(mWeaponType == WeapType_PickProbe)
            {
                MWWorld::ContainerStoreIterator weapon = mPtr.getClass().getInventoryStore(mPtr).getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
                MWWorld::Ptr item = *weapon;
                // TODO: this will only work for the player, and needs to be fixed if NPCs should ever use lockpicks/probes.
                MWWorld::Ptr target = MWBase::Environment::get().getWorld()->getFacedObject();
                std::string resultMessage, resultSound;

                if(!target.isEmpty())
                {
                    if(item.getTypeName() == typeid(ESM::Lockpick).name())
                        Security(mPtr).pickLock(target, item, resultMessage, resultSound);
                    else if(item.getTypeName() == typeid(ESM::Probe).name())
                        Security(mPtr).probeTrap(target, item, resultMessage, resultSound);
                }
                mAnimation->play(mCurrentWeapon, Priority_Weapon,
                                 MWRender::Animation::Group_UpperBody, true,
                                 1.0f, "start", "stop", 0.0, 0);
                mUpperBodyState = UpperCharState_FollowStartToFollowStop;

                if(!resultMessage.empty())
                    MWBase::Environment::get().getWindowManager()->messageBox(resultMessage);
                if(!resultSound.empty())
                    MWBase::Environment::get().getSoundManager()->playSound(resultSound, 1.0f, 1.0f);
            }
            else if (ammunition)
            {
                if(mWeaponType == WeapType_Crossbow || mWeaponType == WeapType_BowAndArrow ||
                   mWeaponType == WeapType_Thrown)
                    mAttackType = "shoot";
                else
                {
                    if(isWeapon && mPtr == MWBase::Environment::get().getWorld()->getPlayerPtr() &&
                            Settings::Manager::getBool("best attack", "Game"))
                    {
                        MWWorld::ContainerStoreIterator weapon = mPtr.getClass().getInventoryStore(mPtr).getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
                        mAttackType = getBestAttack(weapon->get<ESM::Weapon>()->mBase);
                    }
                    else
                        determineAttackType();
                }

                mAnimation->play(mCurrentWeapon, Priority_Weapon,
                                 MWRender::Animation::Group_UpperBody, false,
                                 weapSpeed, mAttackType+" start", mAttackType+" min attack",
                                 0.0f, 0);
                mUpperBodyState = UpperCharState_StartToMinAttack;
            }
        }

        animPlaying = mAnimation->getInfo(mCurrentWeapon, &complete);
    }
    else
    {
        animPlaying = mAnimation->getInfo(mCurrentWeapon, &complete);
        if(mUpperBodyState == UpperCharState_MinAttackToMaxAttack && mHitState != CharState_KnockDown)
        {
            float attackStrength = complete;
            if (!mPtr.getClass().isNpc())
            {
                // most creatures don't actually have an attack wind-up animation, so use a uniform random value
                // (even some creatures that can use weapons don't have a wind-up animation either, e.g. Rieklings)
                // Note: vanilla MW uses a random value for *all* non-player actors, but we probably don't need to go that far.
                attackStrength = std::min(1.f, 0.1f + OEngine::Misc::Rng::rollClosedProbability());
            }

            if(mWeaponType != WeapType_Crossbow && mWeaponType != WeapType_BowAndArrow)
            {
                MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();

                if(isWerewolf)
                {
                    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
                    const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfSwing");
                    if(sound)
                        sndMgr->playSound3D(mPtr, sound->mId, 1.0f, 1.0f);
                }
                else
                {
                    std::string sound = "SwishM";
                    if(attackStrength < 0.5f)
                        sndMgr->playSound3D(mPtr, sound, 1.0f, 0.8f); //Weak attack
                    else if(attackStrength < 1.0f)
                        sndMgr->playSound3D(mPtr, sound, 1.0f, 1.0f); //Medium attack
                    else
                        sndMgr->playSound3D(mPtr, sound, 1.0f, 1.2f); //Strong attack
                }
            }
            stats.setAttackStrength(attackStrength);

            mAnimation->disable(mCurrentWeapon);
            mAnimation->play(mCurrentWeapon, Priority_Weapon,
                             MWRender::Animation::Group_UpperBody, false,
                             weapSpeed, mAttackType+" max attack", mAttackType+" min hit",
                             1.0f-complete, 0);

            complete = 0.f;
            mUpperBodyState = UpperCharState_MaxAttackToMinHit;
        }
        else if (mHitState == CharState_KnockDown)
        {
            if (mUpperBodyState > UpperCharState_WeapEquiped)
                mUpperBodyState = UpperCharState_WeapEquiped;
            mAnimation->disable(mCurrentWeapon);
        }
    }

    mAnimation->setPitchFactor(0.f);
    if (mWeaponType == WeapType_BowAndArrow || mWeaponType == WeapType_Thrown)
    {
        switch (mUpperBodyState)
        {
        case UpperCharState_StartToMinAttack:
            mAnimation->setPitchFactor(complete);
            break;
        case UpperCharState_MinAttackToMaxAttack:
        case UpperCharState_MaxAttackToMinHit:
        case UpperCharState_MinHitToHit:
            mAnimation->setPitchFactor(1.f);
            break;
        case UpperCharState_FollowStartToFollowStop:
            if (animPlaying)
                mAnimation->setPitchFactor(1.f-complete);
            break;
        default:
            break;
        }
    }
    else if (mWeaponType == WeapType_Crossbow)
    {
        switch (mUpperBodyState)
        {
        case UpperCharState_EquipingWeap:
            mAnimation->setPitchFactor(complete);
            break;
        case UpperCharState_UnEquipingWeap:
            mAnimation->setPitchFactor(1.f-complete);
            break;
        case UpperCharState_WeapEquiped:
        case UpperCharState_StartToMinAttack:
        case UpperCharState_MinAttackToMaxAttack:
        case UpperCharState_MaxAttackToMinHit:
        case UpperCharState_MinHitToHit:
        case UpperCharState_FollowStartToFollowStop:
            mAnimation->setPitchFactor(1.f);
            break;
        default:
            break;
        }
    }

    if(!animPlaying)
    {
        if(mUpperBodyState == UpperCharState_EquipingWeap ||
           mUpperBodyState == UpperCharState_FollowStartToFollowStop ||
           mUpperBodyState == UpperCharState_CastingSpell)
        {
            if (ammunition && mWeaponType == WeapType_Crossbow)
                mAnimation->attachArrow();

            mUpperBodyState = UpperCharState_WeapEquiped;
            //don't allow to continue playing hit animation on UpperBody after actor had attacked during it
            if(mHitState == CharState_Hit)
            {
                mAnimation->changeGroups(mCurrentHit, MWRender::Animation::Group_LowerBody);
                //commenting out following 2 lines will give a bit different combat dynamics(slower)
                mHitState = CharState_None;
                mCurrentHit.clear();
                mPtr.getClass().getCreatureStats(mPtr).setHitRecovery(false);
            }
        }
        else if(mUpperBodyState == UpperCharState_UnEquipingWeap)
            mUpperBodyState = UpperCharState_Nothing;
    }
    else if(complete >= 1.0f)
    {
        std::string start, stop;
        switch(mUpperBodyState)
        {
            case UpperCharState_StartToMinAttack:
                start = mAttackType+" min attack";
                stop = mAttackType+" max attack";
                mUpperBodyState = UpperCharState_MinAttackToMaxAttack;
                break;
            case UpperCharState_MinAttackToMaxAttack:
                //hack to avoid body pos desync when jumping/sneaking in 'max attack' state
                if(!mAnimation->isPlaying(mCurrentWeapon))
                    mAnimation->play(mCurrentWeapon, Priority_Weapon,
                        MWRender::Animation::Group_UpperBody, false,
                        0, mAttackType+" min attack", mAttackType+" max attack", 0.999f, 0);
                break;
            case UpperCharState_MaxAttackToMinHit:
                if(mAttackType == "shoot")
                {
                    start = mAttackType+" min hit";
                    stop = mAttackType+" release";
                }
                else
                {
                    start = mAttackType+" min hit";
                    stop = mAttackType+" hit";
                }
                mUpperBodyState = UpperCharState_MinHitToHit;
                break;
            case UpperCharState_MinHitToHit:
                if(mAttackType == "shoot")
                {
                    start = mAttackType+" follow start";
                    stop = mAttackType+" follow stop";
                }
                else
                {
                    float str = stats.getAttackStrength();
                    start = mAttackType+((str < 0.5f) ? " small follow start"
                                                                  : (str < 1.0f) ? " medium follow start"
                                                                                 : " large follow start");
                    stop = mAttackType+((str < 0.5f) ? " small follow stop"
                                                                 : (str < 1.0f) ? " medium follow stop"
                                                                                : " large follow stop");
                }
                mUpperBodyState = UpperCharState_FollowStartToFollowStop;
                break;
            default:
                break;
        }

        if(!start.empty())
        {
            mAnimation->disable(mCurrentWeapon);
            if (mUpperBodyState == UpperCharState_FollowStartToFollowStop)
                mAnimation->play(mCurrentWeapon, Priority_Weapon,
                                 MWRender::Animation::Group_UpperBody, true,
                                 weapSpeed, start, stop, 0.0f, 0);
            else
                mAnimation->play(mCurrentWeapon, Priority_Weapon,
                                 MWRender::Animation::Group_UpperBody, false,
                                 weapSpeed, start, stop, 0.0f, 0);
        }
    }

     //if playing combat animation and lowerbody is not busy switch to whole body animation
    if((weaptype != WeapType_None || mUpperBodyState == UpperCharState_UnEquipingWeap) && animPlaying)
    {
        if( mMovementState != CharState_None ||
             mJumpState != JumpState_None ||
             mHitState != CharState_None ||
             MWBase::Environment::get().getWorld()->isSwimming(mPtr) ||
             cls.getCreatureStats(mPtr).getMovementFlag(CreatureStats::Flag_Sneak))
        {
            mAnimation->changeGroups(mCurrentWeapon, MWRender::Animation::Group_UpperBody);
        }
        else
        {
            mAnimation->changeGroups(mCurrentWeapon, MWRender::Animation::Group_All);
        }
    }

    if (mPtr.getClass().hasInventoryStore(mPtr))
    {
        MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        MWWorld::ContainerStoreIterator torch = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        if(torch != inv.end() && torch->getTypeName() == typeid(ESM::Light).name()
                && updateCarriedLeftVisible(mWeaponType))

        {
            mAnimation->play("torch", Priority_Torch, MWRender::Animation::Group_LeftArm,
                false, 1.0f, "start", "stop", 0.0f, (~(size_t)0), true);
        }
        else if (mAnimation->isPlaying("torch"))
        {
            mAnimation->disable("torch");
        }
    }

    return forcestateupdate;
}

void CharacterController::update(float duration)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    const MWWorld::Class &cls = mPtr.getClass();
    Ogre::Vector3 movement(0.0f);

    updateMagicEffects();

    if(!cls.isActor())
    {
        if(mAnimQueue.size() > 1)
        {
            if(mAnimation->isPlaying(mAnimQueue.front().first) == false)
            {
                mAnimation->disable(mAnimQueue.front().first);
                mAnimQueue.pop_front();

                mAnimation->play(mAnimQueue.front().first, Priority_Default,
                                 MWRender::Animation::Group_All, false,
                                 1.0f, "start", "stop", 0.0f, mAnimQueue.front().second);
            }
        }
    }
    else if(!cls.getCreatureStats(mPtr).isDead())
    {
        bool onground = world->isOnGround(mPtr);
        bool inwater = world->isSwimming(mPtr);
        bool sneak = cls.getCreatureStats(mPtr).getStance(MWMechanics::CreatureStats::Stance_Sneak);
        bool flying = world->isFlying(mPtr);
        // Can't run while flying (see speed formula in Npc/Creature::getSpeed)
        bool isrunning = cls.getCreatureStats(mPtr).getStance(MWMechanics::CreatureStats::Stance_Run) && !flying;
        CreatureStats &stats = cls.getCreatureStats(mPtr);

        //Force Jump Logic

        bool isMoving = (std::abs(cls.getMovementSettings(mPtr).mPosition[0]) > .5 || std::abs(cls.getMovementSettings(mPtr).mPosition[1]) > .5);
        if(!inwater && !flying)
        {
            //Force Jump
            if(stats.getMovementFlag(MWMechanics::CreatureStats::Flag_ForceJump))
            {
                if(onground)
                {
                    cls.getMovementSettings(mPtr).mPosition[2] = 1;
                }
                else
                    cls.getMovementSettings(mPtr).mPosition[2] = 0;
            }
            //Force Move Jump, only jump if they're otherwise moving
            if(stats.getMovementFlag(MWMechanics::CreatureStats::Flag_ForceMoveJump) && isMoving)
            {

                if(onground)
                {
                    cls.getMovementSettings(mPtr).mPosition[2] = 1;
                }
                else
                    cls.getMovementSettings(mPtr).mPosition[2] = 0;
            }
        }

        Ogre::Vector3 vec(cls.getMovementSettings(mPtr).mPosition);
        vec.normalise();

        if(mHitState != CharState_None && mJumpState == JumpState_None)
            vec = Ogre::Vector3(0.0f);
        Ogre::Vector3 rot = cls.getRotationVector(mPtr);

        mMovementSpeed = cls.getSpeed(mPtr);

        vec.x *= mMovementSpeed;
        vec.y *= mMovementSpeed;

        CharacterState movestate = CharState_None;
        CharacterState idlestate = CharState_SpecialIdle;
        bool forcestateupdate = false;

        mHasMovedInXY = std::abs(vec[0])+std::abs(vec[1]) > 0.0f;
        isrunning = isrunning && mHasMovedInXY;


        // advance athletics
        if(mHasMovedInXY && mPtr == MWBase::Environment::get().getWorld()->getPlayerPtr())
        {
            if(inwater)
            {
                mSecondsOfSwimming += duration;
                while(mSecondsOfSwimming > 1)
                {
                    cls.skillUsageSucceeded(mPtr, ESM::Skill::Athletics, 1);
                    mSecondsOfSwimming -= 1;
                }
            }
            else if(isrunning)
            {
                mSecondsOfRunning += duration;
                while(mSecondsOfRunning > 1)
                {
                    cls.skillUsageSucceeded(mPtr, ESM::Skill::Athletics, 0);
                    mSecondsOfRunning -= 1;
                }
            }
        }

        // reduce fatigue
        const MWWorld::Store<ESM::GameSetting> &gmst = world->getStore().get<ESM::GameSetting>();
        float fatigueLoss = 0;
        static const float fFatigueRunBase = gmst.find("fFatigueRunBase")->getFloat();
        static const float fFatigueRunMult = gmst.find("fFatigueRunMult")->getFloat();
        static const float fFatigueSwimWalkBase = gmst.find("fFatigueSwimWalkBase")->getFloat();
        static const float fFatigueSwimRunBase = gmst.find("fFatigueSwimRunBase")->getFloat();
        static const float fFatigueSwimWalkMult = gmst.find("fFatigueSwimWalkMult")->getFloat();
        static const float fFatigueSwimRunMult = gmst.find("fFatigueSwimRunMult")->getFloat();
        static const float fFatigueSneakBase = gmst.find("fFatigueSneakBase")->getFloat();
        static const float fFatigueSneakMult = gmst.find("fFatigueSneakMult")->getFloat();

        const float encumbrance = cls.getEncumbrance(mPtr) / cls.getCapacity(mPtr);
        if (encumbrance < 1)
        {
            if (sneak)
                fatigueLoss = fFatigueSneakBase + encumbrance * fFatigueSneakMult;
            else
            {
                if (inwater)
                {
                    if (!isrunning)
                        fatigueLoss = fFatigueSwimWalkBase + encumbrance * fFatigueSwimWalkMult;
                    else
                        fatigueLoss = fFatigueSwimRunBase + encumbrance * fFatigueSwimRunMult;
                }
                if (isrunning)
                    fatigueLoss = fFatigueRunBase + encumbrance * fFatigueRunMult;
            }
        }
        fatigueLoss *= duration;
        DynamicStat<float> fatigue = cls.getCreatureStats(mPtr).getFatigue();
        fatigue.setCurrent(fatigue.getCurrent() - fatigueLoss, fatigue.getCurrent() < 0);
        cls.getCreatureStats(mPtr).setFatigue(fatigue);

        if(sneak || inwater || flying)
            vec.z = 0.0f;

        if (inwater || flying)
            cls.getCreatureStats(mPtr).land();

        bool inJump = true;
        if(!onground && !flying && !inwater)
        {
            // In the air (either getting up —ascending part of jump— or falling).

            if (world->isSlowFalling(mPtr))
            {
                // SlowFalling spell effect is active, do not keep previous fall height
                cls.getCreatureStats(mPtr).land();
            }

            forcestateupdate = (mJumpState != JumpState_InAir);
            mJumpState = JumpState_InAir;

            static const float fJumpMoveBase = gmst.find("fJumpMoveBase")->getFloat();
            static const float fJumpMoveMult = gmst.find("fJumpMoveMult")->getFloat();
            float factor = fJumpMoveBase + fJumpMoveMult * mPtr.getClass().getSkill(mPtr, ESM::Skill::Acrobatics)/100.f;
            factor = std::min(1.f, factor);
            vec.x *= factor;
            vec.y *= factor;
            vec.z  = 0.0f;
        }
        else if(vec.z > 0.0f && mJumpState == JumpState_None)
        {
            // Started a jump.
            float z = cls.getJump(mPtr);
            if (z > 0)
            {
                if(vec.x == 0 && vec.y == 0)
                    vec = Ogre::Vector3(0.0f, 0.0f, z);
                else
                {
                    Ogre::Vector3 lat = Ogre::Vector3(vec.x, vec.y, 0.0f).normalisedCopy();
                    vec = Ogre::Vector3(lat.x, lat.y, 1.0f) * z * 0.707f;
                }

                // advance acrobatics
                if (mPtr == MWBase::Environment::get().getWorld()->getPlayerPtr())
                    cls.skillUsageSucceeded(mPtr, ESM::Skill::Acrobatics, 0);

                // decrease fatigue
                const MWWorld::Store<ESM::GameSetting> &gmst = world->getStore().get<ESM::GameSetting>();
                const float fatigueJumpBase = gmst.find("fFatigueJumpBase")->getFloat();
                const float fatigueJumpMult = gmst.find("fFatigueJumpMult")->getFloat();
                float normalizedEncumbrance = mPtr.getClass().getNormalizedEncumbrance(mPtr);
                if (normalizedEncumbrance > 1)
                    normalizedEncumbrance = 1;
                const float fatigueDecrease = fatigueJumpBase + (1 - normalizedEncumbrance) * fatigueJumpMult;
                DynamicStat<float> fatigue = cls.getCreatureStats(mPtr).getFatigue();
                fatigue.setCurrent(fatigue.getCurrent() - fatigueDecrease);
                cls.getCreatureStats(mPtr).setFatigue(fatigue);
            }
        }
        else if(mJumpState == JumpState_InAir)
        {
            forcestateupdate = true;
            mJumpState = JumpState_Landing;
            vec.z = 0.0f;

            float height = cls.getCreatureStats(mPtr).land();
            float healthLost = getFallDamage(mPtr, height);
            if (healthLost > 0.0f)
            {
                const float fatigueTerm = cls.getCreatureStats(mPtr).getFatigueTerm();

                // inflict fall damages
                DynamicStat<float> health = cls.getCreatureStats(mPtr).getHealth();
                float realHealthLost = static_cast<float>(healthLost * (1.0f - 0.25f * fatigueTerm));
                health.setCurrent(health.getCurrent() - realHealthLost);
                cls.getCreatureStats(mPtr).setHealth(health);
                cls.onHit(mPtr, realHealthLost, true, MWWorld::Ptr(), MWWorld::Ptr(), true);

                const int acrobaticsSkill = cls.getSkill(mPtr, ESM::Skill::Acrobatics);
                if (healthLost > (acrobaticsSkill * fatigueTerm))
                {
                    cls.getCreatureStats(mPtr).setKnockedDown(true);
                }
                else
                {
                    // report acrobatics progression
                    if (mPtr == MWBase::Environment::get().getWorld()->getPlayerPtr())
                        cls.skillUsageSucceeded(mPtr, ESM::Skill::Acrobatics, 1);
                }
            }
        }
        else
        {
            mJumpState = JumpState_None;
            vec.z = 0.0f;

            inJump = false;

            if(std::abs(vec.x/2.0f) > std::abs(vec.y))
            {
                if(vec.x > 0.0f)
                    movestate = (inwater ? (isrunning ? CharState_SwimRunRight : CharState_SwimWalkRight)
                                         : (sneak ? CharState_SneakRight
                                                  : (isrunning ? CharState_RunRight : CharState_WalkRight)));
                else if(vec.x < 0.0f)
                    movestate = (inwater ? (isrunning ? CharState_SwimRunLeft : CharState_SwimWalkLeft)
                                         : (sneak ? CharState_SneakLeft
                                                  : (isrunning ? CharState_RunLeft : CharState_WalkLeft)));
            }
            else if(vec.y != 0.0f)
            {
                if(vec.y > 0.0f)
                    movestate = (inwater ? (isrunning ? CharState_SwimRunForward : CharState_SwimWalkForward)
                                         : (sneak ? CharState_SneakForward
                                                  : (isrunning ? CharState_RunForward : CharState_WalkForward)));
                else if(vec.y < 0.0f)
                    movestate = (inwater ? (isrunning ? CharState_SwimRunBack : CharState_SwimWalkBack)
                                         : (sneak ? CharState_SneakBack
                                                  : (isrunning ? CharState_RunBack : CharState_WalkBack)));
            }
            // Don't play turning animations during attack. It would break positioning of the arrow bone when releasing a shot.
            // Actually, in vanilla the turning animation is not even played when merely having equipped the weapon,
            // but I don't think we need to go as far as that.
            else if(rot.z != 0.0f && !inwater && !sneak && mUpperBodyState < UpperCharState_StartToMinAttack)
            {
                if(rot.z > 0.0f)
                    movestate = CharState_TurnRight;
                else if(rot.z < 0.0f)
                    movestate = CharState_TurnLeft;
            }
        }

        mTurnAnimationThreshold -= duration;
        if (movestate == CharState_TurnRight || movestate == CharState_TurnLeft)
            mTurnAnimationThreshold = 0.05f;
        else if (movestate == CharState_None && (mMovementState == CharState_TurnRight || mMovementState == CharState_TurnLeft)
                 && mTurnAnimationThreshold > 0)
        {
            movestate = mMovementState;
        }

        if (onground)
            cls.getCreatureStats(mPtr).land();

        if(movestate != CharState_None)
            clearAnimQueue();

        if(mAnimQueue.empty())
        {
            idlestate = (inwater ? CharState_IdleSwim : (sneak ? CharState_IdleSneak : CharState_Idle));
        }
        else if(mAnimQueue.size() > 1)
        {
            if(mAnimation->isPlaying(mAnimQueue.front().first) == false)
            {
                mAnimation->disable(mAnimQueue.front().first);
                mAnimQueue.pop_front();

                mAnimation->play(mAnimQueue.front().first, Priority_Default,
                                 MWRender::Animation::Group_All, false,
                                 1.0f, "start", "stop", 0.0f, mAnimQueue.front().second);
            }
        }

        if(cls.isBipedal(mPtr))
            forcestateupdate = updateWeaponState() || forcestateupdate;
        else
            forcestateupdate = updateCreatureState() || forcestateupdate;

        if (!mSkipAnim)
            refreshCurrentAnims(idlestate, movestate, forcestateupdate);
        if (inJump)
            mMovementAnimationControlled = false;

        if (mMovementState == CharState_TurnLeft || mMovementState == CharState_TurnRight)
        {
            if (duration > 0)
                mAnimation->adjustSpeedMult(mCurrentMovement, std::min(1.5f, std::abs(rot.z) / duration / Ogre::Math::PI));
        }

        if (!mSkipAnim)
        {
            rot *= Ogre::Math::RadiansToDegrees(1.0f);
            if(mHitState != CharState_KnockDown && mHitState != CharState_KnockOut)
            {
                world->rotateObject(mPtr, rot.x, rot.y, rot.z, true);
            }
            else //avoid z-rotating for knockdown
                world->rotateObject(mPtr, rot.x, rot.y, 0.0f, true);

            if (!mMovementAnimationControlled)
                world->queueMovement(mPtr, vec);
        }
        else
            // We must always queue movement, even if there is none, to apply gravity.
            world->queueMovement(mPtr, Ogre::Vector3(0.0f));

        movement = vec;
        cls.getMovementSettings(mPtr).mPosition[0] = cls.getMovementSettings(mPtr).mPosition[1] = 0;
        // Can't reset jump state (mPosition[2]) here; we don't know for sure whether the PhysicSystem will actually handle it in this frame
        // due to the fixed minimum timestep used for the physics update. It will be reset in PhysicSystem::move once the jump is handled.

        if (!mSkipAnim)
            updateHeadTracking(duration);
    }
    else if(cls.getCreatureStats(mPtr).isDead())
    {
        world->queueMovement(mPtr, Ogre::Vector3(0.0f));
    }

    Ogre::Vector3 moved = mAnimation->runAnimation(mSkipAnim ? 0.f : duration);
    if(duration > 0.0f)
        moved /= duration;
    else
        moved = Ogre::Vector3(0.0f);

    // Ensure we're moving in generally the right direction...
    if(mMovementSpeed > 0.f)
    {
        float l = moved.length();

        if((movement.x < 0.0f && movement.x < moved.x*2.0f) ||
           (movement.x > 0.0f && movement.x > moved.x*2.0f))
            moved.x = movement.x;
        if((movement.y < 0.0f && movement.y < moved.y*2.0f) ||
           (movement.y > 0.0f && movement.y > moved.y*2.0f))
            moved.y = movement.y;
        if((movement.z < 0.0f && movement.z < moved.z*2.0f) ||
           (movement.z > 0.0f && movement.z > moved.z*2.0f))
            moved.z = movement.z;
        // but keep the original speed
        float newLength = moved.length();
        if (newLength > 0)
            moved *= (l / newLength);
    }

    if (mSkipAnim)
        mAnimation->updateEffects(duration);

    // Update movement
    if(mMovementAnimationControlled && mPtr.getClass().isActor())
        world->queueMovement(mPtr, moved);

    mSkipAnim = false;

    mAnimation->enableHeadAnimation(cls.isActor() && !cls.getCreatureStats(mPtr).isDead());
}


void CharacterController::playGroup(const std::string &groupname, int mode, int count)
{
    if(!mAnimation || !mAnimation->hasAnimation(groupname))
        std::cerr<< "Animation "<<groupname<<" not found" <<std::endl;
    else
    {
        count = std::max(count, 1);
        if(mode != 0 || mAnimQueue.empty() || !isAnimPlaying(mAnimQueue.front().first))
        {
            clearAnimQueue();
            mAnimQueue.push_back(std::make_pair(groupname, count-1));

            mAnimation->disable(mCurrentIdle);
            mCurrentIdle.clear();

            mIdleState = CharState_SpecialIdle;
            mAnimation->play(groupname, Priority_Default,
                             MWRender::Animation::Group_All, false, 1.0f,
                             ((mode==2) ? "loop start" : "start"), "stop", 0.0f, count-1);
        }
        else if(mode == 0)
        {
            if (!mAnimQueue.empty())
                mAnimation->stopLooping(mAnimQueue.front().first);
            mAnimQueue.resize(1);
            mAnimQueue.push_back(std::make_pair(groupname, count-1));
        }
    }
}

void CharacterController::skipAnim()
{
    mSkipAnim = true;
}

bool CharacterController::isAnimPlaying(const std::string &groupName)
{
    if(mAnimation == NULL)
        return false;
    return mAnimation->isPlaying(groupName);
}


void CharacterController::clearAnimQueue()
{
    if(!mAnimQueue.empty())
        mAnimation->disable(mAnimQueue.front().first);
    mAnimQueue.clear();
}


void CharacterController::forceStateUpdate()
{
    if(!mAnimation)
        return;
    clearAnimQueue();

    refreshCurrentAnims(mIdleState, mMovementState, true);
    if(mDeathState != CharState_None)
    {
        playRandomDeath();
    }

    mAnimation->runAnimation(0.f);
}

bool CharacterController::kill()
{
    if( isDead() )
    {
        if( mPtr == MWBase::Environment::get().getWorld()->getPlayerPtr() && !isAnimPlaying(mCurrentDeath) )
        {
            //player's death animation is over
            MWBase::Environment::get().getStateManager()->askLoadRecent();
        }
        return false;
    }

    playRandomDeath();

    mAnimation->disable(mCurrentIdle);

    mIdleState = CharState_None;
    mCurrentIdle.clear();

    // Play Death Music if it was the player dying
    if(mPtr == MWBase::Environment::get().getWorld()->getPlayerPtr())
        MWBase::Environment::get().getSoundManager()->streamMusic("Special/MW_Death.mp3");

    return true;
}

void CharacterController::resurrect()
{
    if(mDeathState == CharState_None)
        return;

    if(mAnimation)
        mAnimation->disable(mCurrentDeath);
    mCurrentDeath.clear();
    mDeathState = CharState_None;
}

void CharacterController::updateContinuousVfx()
{
    // Keeping track of when to stop a continuous VFX seems to be very difficult to do inside the spells code,
    // as it's extremely spread out (ActiveSpells, Spells, InventoryStore effects, etc...) so we do it here.

    // Stop any effects that are no longer active
    std::vector<int> effects;
    mAnimation->getLoopingEffects(effects);

    for (std::vector<int>::iterator it = effects.begin(); it != effects.end(); ++it)
    {
        if (mPtr.getClass().getCreatureStats(mPtr).isDead()
            || mPtr.getClass().getCreatureStats(mPtr).getMagicEffects().get(MWMechanics::EffectKey(*it)).getMagnitude() <= 0)
            mAnimation->removeEffect(*it);
    }
}

void CharacterController::updateMagicEffects()
{
    if (!mPtr.getClass().isActor())
        return;
    float alpha = 1.f;
    if (mPtr.getClass().getCreatureStats(mPtr).getMagicEffects().get(ESM::MagicEffect::Invisibility).getMagnitude())
    {
        if (mPtr == MWBase::Environment::get().getWorld()->getPlayerPtr())
            alpha = 0.4f;
        else
            alpha = 0.f;
    }
    float chameleon = mPtr.getClass().getCreatureStats(mPtr).getMagicEffects().get(ESM::MagicEffect::Chameleon).getMagnitude();
    if (chameleon)
    {
        alpha *= std::max(0.2f, (100.f - chameleon)/100.f);
    }
    mAnimation->setAlpha(alpha);

    bool vampire = mPtr.getClass().getCreatureStats(mPtr).getMagicEffects().get(ESM::MagicEffect::Vampirism).getMagnitude() > 0.0f;
    mAnimation->setVampire(vampire);

    float light = mPtr.getClass().getCreatureStats(mPtr).getMagicEffects().get(ESM::MagicEffect::Light).getMagnitude();
    mAnimation->setLightEffect(light);
}

void CharacterController::determineAttackType()
{
    float *move = mPtr.getClass().getMovementSettings(mPtr).mPosition;

    if (move[1] && !move[0]) // forward-backward
        mAttackType = "thrust";
    else if (move[0] && !move[1]) //sideway
        mAttackType = "slash";
    else
        mAttackType = "chop";
}

bool CharacterController::isReadyToBlock() const
{
    return updateCarriedLeftVisible(mWeaponType);
}

bool CharacterController::isKnockedOut() const
{
    return mHitState == CharState_KnockOut;
}

void CharacterController::setHeadTrackTarget(const MWWorld::Ptr &target)
{
    mHeadTrackTarget = target;
}

void CharacterController::updateHeadTracking(float duration)
{
    Ogre::Node* head = mAnimation->getNode("Bip01 Head");
    if (!head)
        return;
    Ogre::Radian zAngle (0.f);
    Ogre::Radian xAngle (0.f);
    if (!mHeadTrackTarget.isEmpty())
    {
        Ogre::Vector3 headPos = mPtr.getRefData().getBaseNode()->convertLocalToWorldPosition(head->_getDerivedPosition());
        Ogre::Vector3 targetPos (mHeadTrackTarget.getRefData().getPosition().pos);
        if (MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(mHeadTrackTarget))
        {
            Ogre::Node* targetHead = anim->getNode("Head");
            if (!targetHead)
                targetHead = anim->getNode("Bip01 Head");
            if (targetHead)
                targetPos = mHeadTrackTarget.getRefData().getBaseNode()->convertLocalToWorldPosition(
                        targetHead->_getDerivedPosition());
        }

        Ogre::Vector3 direction = targetPos - headPos;
        direction.normalise();

        const Ogre::Vector3 actorDirection = mPtr.getRefData().getBaseNode()->getOrientation().yAxis();

        zAngle = Ogre::Math::ATan2(direction.x,direction.y) -
                Ogre::Math::ATan2(actorDirection.x, actorDirection.y);
        xAngle = -Ogre::Math::ASin(direction.z);
        wrap(zAngle);
        wrap(xAngle);
        xAngle = Ogre::Degree(std::min(xAngle.valueDegrees(), 40.f));
        xAngle = Ogre::Degree(std::max(xAngle.valueDegrees(), -40.f));
        zAngle = Ogre::Degree(std::min(zAngle.valueDegrees(), 30.f));
        zAngle = Ogre::Degree(std::max(zAngle.valueDegrees(), -30.f));

    }
    float factor = duration*5;
    factor = std::min(factor, 1.f);
    xAngle = (1.f-factor) * mAnimation->getHeadPitch() + factor * (-xAngle);
    zAngle = (1.f-factor) * mAnimation->getHeadYaw() + factor * (-zAngle);

    mAnimation->setHeadPitch(xAngle);
    mAnimation->setHeadYaw(zAngle);
}

}
