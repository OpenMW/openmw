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

#include <iostream>

#include <osg/PositionAttitudeTransform>

#include "movement.hpp"
#include "npcstats.hpp"
#include "creaturestats.hpp"
#include "security.hpp"
#include "actorutil.hpp"

#include <components/misc/rng.hpp>

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
#include "../mwworld/player.hpp"

namespace
{

// Wraps a value to (-PI, PI]
void wrap(float& rad)
{
    if (rad>0)
        rad = std::fmod(rad+osg::PI, 2.0f*osg::PI)-osg::PI;
    else
        rad = std::fmod(rad-osg::PI, 2.0f*osg::PI)+osg::PI;
}

std::string toString(int num)
{
    std::ostringstream stream;
    stream << num;
    return stream.str();
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
    while (mAnimation->hasAnimation(prefix + toString(numAnims+1)))
        ++numAnims;

    int roll = Misc::Rng::rollDice(numAnims) + 1; // [1, numAnims]
    if (num)
        *num = roll;
    return prefix + toString(roll);
}

void CharacterController::refreshCurrentAnims(CharacterState idle, CharacterState movement, JumpingState jump, bool force)
{
    // hit recoils/knockdown animations handling
    if(mPtr.getClass().isActor())
    {
        bool recovery = mPtr.getClass().getCreatureStats(mPtr).getHitRecovery();
        bool knockdown = mPtr.getClass().getCreatureStats(mPtr).getKnockedDown();
        bool block = mPtr.getClass().getCreatureStats(mPtr).getBlock();
        if(mHitState == CharState_None)
        {
            if ((mPtr.getClass().getCreatureStats(mPtr).getFatigue().getCurrent() < 0
                    || mPtr.getClass().getCreatureStats(mPtr).getFatigue().getBase() == 0)
                    && mAnimation->hasAnimation("knockout"))
            {
                mHitState = CharState_KnockOut;
                mCurrentHit = "knockout";
                mAnimation->play(mCurrentHit, Priority_Knockdown, MWRender::Animation::BlendMask_All, false, 1, "start", "stop", 0.0f, ~0ul);
                mPtr.getClass().getCreatureStats(mPtr).setKnockedDown(true);
            }
            else if(knockdown && mAnimation->hasAnimation("knockdown"))
            {
                mHitState = CharState_KnockDown;
                mCurrentHit = "knockdown";
                mAnimation->play(mCurrentHit, Priority_Knockdown, MWRender::Animation::BlendMask_All, true, 1, "start", "stop", 0.0f, 0);
            }
            else if (recovery)
            {
                std::string anim = chooseRandomGroup("hit");
                if (mAnimation->hasAnimation(anim))
                {
                    mHitState = CharState_Hit;
                    mCurrentHit = anim;
                    mAnimation->play(mCurrentHit, Priority_Hit, MWRender::Animation::BlendMask_All, true, 1, "start", "stop", 0.0f, 0);
                }
            }
            else if (block && mAnimation->hasAnimation("shield"))
            {
                mHitState = CharState_Block;
                mCurrentHit = "shield";
                MWRender::Animation::AnimPriority priorityBlock (Priority_Hit);
                priorityBlock[MWRender::Animation::BoneGroup_LeftArm] = Priority_Block;
                mAnimation->play(mCurrentHit, priorityBlock, MWRender::Animation::BlendMask_All, true, 1, "block start", "block stop", 0.0f, 0);
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
            mAnimation->play(mCurrentHit, Priority_Knockdown, MWRender::Animation::BlendMask_All, true, 1, "loop stop", "stop", 0.0f, 0);
        }
    }

    const WeaponInfo *weap = std::find_if(sWeaponTypeList, sWeaponTypeListEnd, FindWeaponType(mWeaponType));
    if (!mPtr.getClass().isBipedal(mPtr))
        weap = sWeaponTypeListEnd;

    if(force || jump != mJumpState)
    {
        bool startAtLoop = (jump == mJumpState);
        mJumpState = jump;

        std::string jumpAnimName;
        MWRender::Animation::BlendMask jumpmask = MWRender::Animation::BlendMask_All;
        if(mJumpState != JumpState_None)
        {
            jumpAnimName = "jump";
            if(weap != sWeaponTypeListEnd)
            {
                jumpAnimName += weap->shortgroup;
                if(!mAnimation->hasAnimation(jumpAnimName))
                {
                    jumpmask = MWRender::Animation::BlendMask_LowerBody;
                    jumpAnimName = "jump";
                }
            }
        }

        if(mJumpState == JumpState_InAir)
        {
            mAnimation->disable(mCurrentJump);
            mCurrentJump = jumpAnimName;
            if (mAnimation->hasAnimation("jump"))
                mAnimation->play(mCurrentJump, Priority_Jump, jumpmask, false,
                             1.0f, (startAtLoop?"loop start":"start"), "stop", 0.0f, ~0ul);
        }
        else
        {
            mAnimation->disable(mCurrentJump);
            mCurrentJump.clear();
            if (mAnimation->hasAnimation("jump"))
                mAnimation->play(jumpAnimName, Priority_Jump, jumpmask, true,
                             1.0f, "loop stop", "stop", 0.0f, 0);
        }
    }

    if(force || movement != mMovementState)
    {
        mMovementState = movement;

        std::string movementAnimName;
        MWRender::Animation::BlendMask movemask = MWRender::Animation::BlendMask_All;
        const StateInfo *movestate = std::find_if(sMovementList, sMovementListEnd, FindCharState(mMovementState));
        if(movestate != sMovementListEnd)
        {
            movementAnimName = movestate->groupname;
            if(weap != sWeaponTypeListEnd && movementAnimName.find("swim") == std::string::npos)
            {
                movementAnimName += weap->shortgroup;
                if(!mAnimation->hasAnimation(movementAnimName))
                {
                    movemask = MWRender::Animation::BlendMask_LowerBody;
                    movementAnimName = movestate->groupname;
                }
            }

            if(!mAnimation->hasAnimation(movementAnimName))
            {
                std::string::size_type swimpos = movementAnimName.find("swim");
                if(swimpos == std::string::npos)
                {
                    std::string::size_type runpos = movementAnimName.find("run");
                    if (runpos != std::string::npos)
                    {
                        movementAnimName.replace(runpos, runpos+3, "walk");
                        if (!mAnimation->hasAnimation(movementAnimName))
                            movementAnimName.clear();
                    }
                    else
                        movementAnimName.clear();
                }
                else
                {
                    if (weap != sWeaponTypeListEnd)
                        movemask = MWRender::Animation::BlendMask_LowerBody;
                    movementAnimName.erase(swimpos, 4);
                    if(!mAnimation->hasAnimation(movementAnimName))
                        movementAnimName.clear();
                }
            }
        }

        /* If we're playing the same animation, restart from the loop start instead of the
         * beginning. */
        int mode = ((movementAnimName == mCurrentMovement) ? 2 : 1);

        mMovementAnimationControlled = true;

        mAnimation->disable(mCurrentMovement);
        mCurrentMovement = movementAnimName;
        if(!mCurrentMovement.empty())
        {
            bool isrunning = mPtr.getClass().getCreatureStats(mPtr).getStance(MWMechanics::CreatureStats::Stance_Run)
                    && !MWBase::Environment::get().getWorld()->isFlying(mPtr);

            // For non-flying creatures, MW uses the Walk animation to calculate the animation velocity
            // even if we are running. This must be replicated, otherwise the observed speed would differ drastically.
            std::string anim = mCurrentMovement;
            mAdjustMovementAnimSpeed = true;
            if (mPtr.getClass().getTypeName() == typeid(ESM::Creature).name()
                    && !(mPtr.get<ESM::Creature>()->mBase->mFlags & ESM::Creature::Flies))
            {
                CharacterState walkState = runStateToWalkState(mMovementState);
                const StateInfo *stateinfo = std::find_if(sMovementList, sMovementListEnd, FindCharState(walkState));
                anim = stateinfo->groupname;

                mMovementAnimSpeed = mAnimation->getVelocity(anim);
                if (mMovementAnimSpeed <= 1.0f)
                {
                    // Another bug: when using a fallback animation (e.g. RunForward as fallback to SwimRunForward),
                    // then the equivalent Walk animation will not use a fallback, and if that animation doesn't exist
                    // we will play without any scaling.
                    // Makes the speed attribute of most water creatures totally useless.
                    // And again, this can not be fixed without patching game data.
                    mAdjustMovementAnimSpeed = false;
                    mMovementAnimSpeed = 1.f;
                }
            }
            else
            {
                mMovementAnimSpeed = mAnimation->getVelocity(anim);

                if (mMovementAnimSpeed <= 1.0f)
                {
                    // The first person anims don't have any velocity to calculate a speed multiplier from.
                    // We use the third person velocities instead.
                    // FIXME: should be pulled from the actual animation, but it is not presently loaded.
                    mMovementAnimSpeed = (isrunning ? 222.857f : 154.064f);
                    mMovementAnimationControlled = false;
                }
            }

            mAnimation->play(mCurrentMovement, Priority_Movement, movemask, false,
                             1.f, ((mode!=2)?"start":"loop start"), "stop", 0.0f, ~0ul);
        }
    }

    // idle handled last as it can depend on the other states
    // FIXME: if one of the below states is close to their last animation frame (i.e. will be disabled in the coming update),
    // the idle animation should be displayed
    if ((mUpperBodyState != UpperCharState_Nothing
            || (mMovementState != CharState_None && mMovementState != CharState_TurnLeft && mMovementState != CharState_TurnRight)
            || mHitState != CharState_None)
            && !mPtr.getClass().isBipedal(mPtr))
        idle = CharState_None;

    if(force || idle != mIdleState)
    {
        mIdleState = idle;

        std::string idle;
        MWRender::Animation::AnimPriority idlePriority (Priority_Default);
        // Only play "idleswim" or "idlesneak" if they exist. Otherwise, fallback to
        // "idle"+weapon or "idle".
        if(mIdleState == CharState_IdleSwim && mAnimation->hasAnimation("idleswim"))
        {
            idle = "idleswim";
            idlePriority = Priority_SwimIdle;
        }
        else if(mIdleState == CharState_IdleSneak && mAnimation->hasAnimation("idlesneak"))
        {
            idle = "idlesneak";
            idlePriority[MWRender::Animation::BoneGroup_LowerBody] = Priority_SneakIdleLowerBody;
        }
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
            mAnimation->play(mCurrentIdle, idlePriority, MWRender::Animation::BlendMask_All, false,
                             1.0f, "start", "stop", 0.0f, ~0ul, true);
    }
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
        mCurrentDeath = "death" + toString(death - CharState_Death1 + 1);
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

    mAnimation->play(mCurrentDeath, Priority_Death, MWRender::Animation::BlendMask_All,
                    false, 1.0f, "start", "stop", startpoint, 0);
}

void CharacterController::playRandomDeath(float startpoint)
{
    if (mPtr == getPlayer())
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
    , mAdjustMovementAnimSpeed(false)
    , mHasMovedInXY(false)
    , mMovementAnimationControlled(true)
    , mDeathState(CharState_None)
    , mHitState(CharState_None)
    , mUpperBodyState(UpperCharState_Nothing)
    , mJumpState(JumpState_None)
    , mWeaponType(WeapType_None)
    , mAttackStrength(0.f)
    , mSkipAnim(false)
    , mSecondsOfSwimming(0)
    , mSecondsOfRunning(0)
    , mTurnAnimationThreshold(0)
    , mAttackingOrSpell(false)
{
    if(!mAnimation)
        return;

    mAnimation->setTextKeyListener(this);

    const MWWorld::Class &cls = mPtr.getClass();
    if(cls.isActor())
    {
        /* Accumulate along X/Y only for now, until we can figure out how we should
         * handle knockout and death which moves the character down. */
        mAnimation->setAccumulation(osg::Vec3f(1.0f, 1.0f, 0.0f));

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
            // Set the death state, but don't play it yet
            // We will play it in the first frame, but only if no script set the skipAnim flag
            mDeathState = static_cast<CharacterState>(CharState_Death1 + mPtr.getClass().getCreatureStats(mPtr).getDeathAnimation());
        }
    }
    else
    {
        /* Don't accumulate with non-actors. */
        mAnimation->setAccumulation(osg::Vec3f(0.f, 0.f, 0.f));

        mIdleState = CharState_Idle;
    }


    if(mDeathState == CharState_None)
        refreshCurrentAnims(mIdleState, mMovementState, mJumpState, true);

    mAnimation->runAnimation(0.f);
}

CharacterController::~CharacterController()
{
    if (mAnimation)
        mAnimation->setTextKeyListener(NULL);
}

void split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

void CharacterController::handleTextKey(const std::string &groupname, const std::multimap<float, std::string>::const_iterator &key, const std::multimap<float, std::string> &map)
{
    const std::string &evt = key->second;

    if(evt.compare(0, 7, "sound: ") == 0)
    {
        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        sndMgr->playSound3D(mPtr, evt.substr(7), 1.0f, 1.0f);
        return;
    }
    if(evt.compare(0, 10, "soundgen: ") == 0)
    {
        std::string soundgen = evt.substr(10);

        // The event can optionally contain volume and pitch modifiers
        float volume=1.f, pitch=1.f;
        if (soundgen.find(" ") != std::string::npos)
        {
            std::vector<std::string> tokens;
            split(soundgen, ' ', tokens);
            soundgen = tokens[0];
            if (tokens.size() >= 2)
            {
                std::stringstream stream;
                stream << tokens[1];
                stream >> volume;
            }
            if (tokens.size() >= 3)
            {
                std::stringstream stream;
                stream << tokens[2];
                stream >> pitch;
            }
        }

        std::string sound = mPtr.getClass().getSoundIdFromSndGen(mPtr, soundgen);
        if(!sound.empty())
        {
            MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
            MWBase::SoundManager::PlayType type = MWBase::SoundManager::Play_TypeSfx;
            if(evt.compare(10, evt.size()-10, "left") == 0 || evt.compare(10, evt.size()-10, "right") == 0 || evt.compare(10, evt.size()-10, "land") == 0)
                type = MWBase::SoundManager::Play_TypeFoot;
            sndMgr->playSound3D(mPtr, sound, volume, pitch, type);
        }
        return;
    }

    if(evt.compare(0, groupname.size(), groupname) != 0 ||
       evt.compare(groupname.size(), 2, ": ") != 0)
    {
        // Not ours, skip it
        return;
    }
    size_t off = groupname.size()+2;
    size_t len = evt.size() - off;

    if(evt.compare(off, len, "equip attach") == 0)
        mAnimation->showWeapons(true);
    else if(evt.compare(off, len, "unequip detach") == 0)
        mAnimation->showWeapons(false);
    else if(evt.compare(off, len, "chop hit") == 0)
        mPtr.getClass().hit(mPtr, mAttackStrength, ESM::Weapon::AT_Chop);
    else if(evt.compare(off, len, "slash hit") == 0)
        mPtr.getClass().hit(mPtr, mAttackStrength, ESM::Weapon::AT_Slash);
    else if(evt.compare(off, len, "thrust hit") == 0)
        mPtr.getClass().hit(mPtr, mAttackStrength, ESM::Weapon::AT_Thrust);
    else if(evt.compare(off, len, "hit") == 0)
    {
        if (groupname == "attack1")
            mPtr.getClass().hit(mPtr, mAttackStrength, ESM::Weapon::AT_Chop);
        else if (groupname == "attack2")
            mPtr.getClass().hit(mPtr, mAttackStrength, ESM::Weapon::AT_Slash);
        else if (groupname == "attack3")
            mPtr.getClass().hit(mPtr, mAttackStrength, ESM::Weapon::AT_Thrust);
        else
            mPtr.getClass().hit(mPtr, mAttackStrength);
    }
    else if (!groupname.empty() && groupname.compare(0, groupname.size()-1, "attack") == 0
             && evt.compare(off, len, "start") == 0)
    {
        std::multimap<float, std::string>::const_iterator hitKey = key;

        // Not all animations have a hit key defined. If there is none, the hit happens with the start key.
        bool hasHitKey = false;
        while (hitKey != map.end())
        {
            if (hitKey->second == groupname + ": hit")
            {
                hasHitKey = true;
                break;
            }
            if (hitKey->second == groupname + ": stop")
                break;
            ++hitKey;
        }
        if (!hasHitKey)
        {
            if (groupname == "attack1")
                mPtr.getClass().hit(mPtr, mAttackStrength, ESM::Weapon::AT_Chop);
            else if (groupname == "attack2")
                mPtr.getClass().hit(mPtr, mAttackStrength, ESM::Weapon::AT_Slash);
            else if (groupname == "attack3")
                mPtr.getClass().hit(mPtr, mAttackStrength, ESM::Weapon::AT_Thrust);
        }
    }
    else if (evt.compare(off, len, "shoot attach") == 0)
        mAnimation->attachArrow();
    else if (evt.compare(off, len, "shoot release") == 0)
        mAnimation->releaseArrow(mAttackStrength);
    else if (evt.compare(off, len, "shoot follow attach") == 0)
        mAnimation->attachArrow();

    else if (groupname == "spellcast" && evt.substr(evt.size()-7, 7) == "release"
             // Make sure this key is actually for the RangeType we are casting. The flame atronach has
             // the same animation for all range types, so there are 3 "release" keys on the same time, one for each range type.
             && evt.compare(off, len, mAttackType + " release") == 0)
    {
        MWBase::Environment::get().getWorld()->castSpell(mPtr);
    }

    else if (groupname == "shield" && evt.compare(off, len, "block hit") == 0)
        mPtr.getClass().block(mPtr);
}

void CharacterController::updatePtr(const MWWorld::Ptr &ptr)
{
    mPtr = ptr;
}

void CharacterController::updateIdleStormState(bool inwater)
{
    bool inStormDirection = false;
    if (MWBase::Environment::get().getWorld()->isInStorm())
    {
        osg::Vec3f stormDirection = MWBase::Environment::get().getWorld()->getStormDirection();
        osg::Vec3f characterDirection = mPtr.getRefData().getBaseNode()->getAttitude() * osg::Vec3f(0,1,0);
        inStormDirection = std::acos(stormDirection * characterDirection / (stormDirection.length() * characterDirection.length()))
                > osg::DegreesToRadians(120.f);
    }
    if (inStormDirection && !inwater && mUpperBodyState == UpperCharState_Nothing && mAnimation->hasAnimation("idlestorm"))
    {
        float complete = 0;
        mAnimation->getInfo("idlestorm", &complete);

        if (complete == 0)
            mAnimation->play("idlestorm", Priority_Storm, MWRender::Animation::BlendMask_RightArm, false,
                             1.0f, "start", "loop start", 0.0f, 0);
        else if (complete == 1)
            mAnimation->play("idlestorm", Priority_Storm, MWRender::Animation::BlendMask_RightArm, false,
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
                    mAnimation->play("idlestorm", Priority_Storm, MWRender::Animation::BlendMask_RightArm, true,
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

    if(mAttackingOrSpell)
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
                int roll = Misc::Rng::rollDice(3); // [0, 2]
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
                                 MWRender::Animation::BlendMask_All, true,
                                 1, startKey, stopKey,
                                 0.0f, 0);
                mUpperBodyState = UpperCharState_StartToMinAttack;

                mAttackStrength = std::min(1.f, 0.1f + Misc::Rng::rollClosedProbability());
            }
        }

        mAttackingOrSpell = false;
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

    MWRender::Animation::AnimPriority priorityWeapon(Priority_Weapon);
    priorityWeapon[MWRender::Animation::BoneGroup_LowerBody] = Priority_WeaponLowerBody;

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
            mAnimation->play(weapgroup, priorityWeapon,
                             MWRender::Animation::BlendMask_All, true,
                             1.0f, "unequip start", "unequip stop", 0.0f, 0);
            mUpperBodyState = UpperCharState_UnEquipingWeap;
        }
        else
        {
            getWeaponGroup(weaptype, weapgroup);
            mAnimation->showWeapons(false);
            mAnimation->setWeaponGroup(weapgroup);

            mAnimation->play(weapgroup, priorityWeapon,
                             MWRender::Animation::BlendMask_All, true,
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
    if(mAttackingOrSpell)
    {
        if(mUpperBodyState == UpperCharState_WeapEquiped && (mHitState == CharState_None || mHitState == CharState_Block))
        {
            MWBase::Environment::get().getWorld()->breakInvisibility(mPtr);
            mAttackType.clear();
            if(mWeaponType == WeapType_Spell)
            {
                // Unset casting flag, otherwise pressing the mouse button down would
                // continue casting every frame if there is no animation
                mAttackingOrSpell = false;
                if (mPtr == getPlayer())
                {
                    MWBase::Environment::get().getWorld()->getPlayer().setAttackingOrSpell(false);
                }

                const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();

                // For the player, set the spell we want to cast
                // This has to be done at the start of the casting animation,
                // *not* when selecting a spell in the GUI (otherwise you could change the spell mid-animation)
                if (mPtr == getPlayer())
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
                    if (mAnimation->getNode("Bip01 L Hand"))
                        mAnimation->addEffect("meshes\\" + castStatic->mModel, -1, false, "Bip01 L Hand", effect->mParticle);

                    if (mAnimation->getNode("Bip01 R Hand"))
                        mAnimation->addEffect("meshes\\" + castStatic->mModel, -1, false, "Bip01 R Hand", effect->mParticle);

                    switch(effectentry.mRange)
                    {
                        case 0: mAttackType = "self"; break;
                        case 1: mAttackType = "touch"; break;
                        case 2: mAttackType = "target"; break;
                    }

                    mAnimation->play(mCurrentWeapon, priorityWeapon,
                                     MWRender::Animation::BlendMask_All, true,
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
                mAnimation->play(mCurrentWeapon, priorityWeapon,
                                 MWRender::Animation::BlendMask_All, true,
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
                    if(isWeapon && mPtr == getPlayer() &&
                            Settings::Manager::getBool("best attack", "Game"))
                    {
                        MWWorld::ContainerStoreIterator weapon = mPtr.getClass().getInventoryStore(mPtr).getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
                        mAttackType = getBestAttack(weapon->get<ESM::Weapon>()->mBase);
                    }
                    else
                        determineAttackType();
                }

                mAnimation->play(mCurrentWeapon, priorityWeapon,
                                 MWRender::Animation::BlendMask_All, false,
                                 weapSpeed, mAttackType+" start", mAttackType+" min attack",
                                 0.0f, 0);
                mUpperBodyState = UpperCharState_StartToMinAttack;
            }
        }

        animPlaying = mAnimation->getInfo(mCurrentWeapon, &complete);
        if(mUpperBodyState == UpperCharState_MinAttackToMaxAttack && mHitState != CharState_KnockDown)
            mAttackStrength = complete;
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
                attackStrength = std::min(1.f, 0.1f + Misc::Rng::rollClosedProbability());
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
            mAttackStrength = attackStrength;

            mAnimation->disable(mCurrentWeapon);
            mAnimation->play(mCurrentWeapon, priorityWeapon,
                             MWRender::Animation::BlendMask_All, false,
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
                    mAnimation->play(mCurrentWeapon, priorityWeapon,
                        MWRender::Animation::BlendMask_All, false,
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
                    float str = mAttackStrength;
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
                mAnimation->play(mCurrentWeapon, priorityWeapon,
                                 MWRender::Animation::BlendMask_All, true,
                                 weapSpeed, start, stop, 0.0f, 0);
            else
                mAnimation->play(mCurrentWeapon, priorityWeapon,
                                 MWRender::Animation::BlendMask_All, false,
                                 weapSpeed, start, stop, 0.0f, 0);
        }
    }

    if (mPtr.getClass().hasInventoryStore(mPtr))
    {
        MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        MWWorld::ContainerStoreIterator torch = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        if(torch != inv.end() && torch->getTypeName() == typeid(ESM::Light).name()
                && updateCarriedLeftVisible(mWeaponType))

        {
            mAnimation->play("torch", Priority_Torch, MWRender::Animation::BlendMask_LeftArm,
                false, 1.0f, "start", "stop", 0.0f, (~(size_t)0), true);
        }
        else if (mAnimation->isPlaying("torch"))
        {
            mAnimation->disable("torch");
        }
    }

    mAnimation->setAccurateAiming(mUpperBodyState > UpperCharState_WeapEquiped);

    return forcestateupdate;
}

void CharacterController::update(float duration)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    const MWWorld::Class &cls = mPtr.getClass();
    osg::Vec3f movement(0.f, 0.f, 0.f);
    float speed = 0.f;

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
                                 MWRender::Animation::BlendMask_All, false,
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

        osg::Vec3f vec(cls.getMovementSettings(mPtr).asVec3());
        vec.normalize();

        if(mHitState != CharState_None && mJumpState == JumpState_None)
            vec = osg::Vec3f(0.f, 0.f, 0.f);
        osg::Vec3f rot = cls.getRotationVector(mPtr);

        speed = cls.getSpeed(mPtr);

        vec.x() *= speed;
        vec.y() *= speed;

        CharacterState movestate = CharState_None;
        CharacterState idlestate = CharState_SpecialIdle;
        JumpingState jumpstate = JumpState_None;

        bool forcestateupdate = false;

        mHasMovedInXY = std::abs(vec.x())+std::abs(vec.y()) > 0.0f;
        isrunning = isrunning && mHasMovedInXY;


        // advance athletics
        if(mHasMovedInXY && mPtr == getPlayer())
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
            vec.z() = 0.0f;

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
            jumpstate = JumpState_InAir;

            static const float fJumpMoveBase = gmst.find("fJumpMoveBase")->getFloat();
            static const float fJumpMoveMult = gmst.find("fJumpMoveMult")->getFloat();
            float factor = fJumpMoveBase + fJumpMoveMult * mPtr.getClass().getSkill(mPtr, ESM::Skill::Acrobatics)/100.f;
            factor = std::min(1.f, factor);
            vec.x() *= factor;
            vec.y() *= factor;
            vec.z()  = 0.0f;
        }
        else if(vec.z() > 0.0f && mJumpState == JumpState_None)
        {
            // Started a jump.
            float z = cls.getJump(mPtr);
            if (z > 0)
            {
                if(vec.x() == 0 && vec.y() == 0)
                    vec = osg::Vec3f(0.0f, 0.0f, z);
                else
                {
                    osg::Vec3f lat (vec.x(), vec.y(), 0.0f);
                    lat.normalize();
                    vec = osg::Vec3f(lat.x(), lat.y(), 1.0f) * z * 0.707f;
                }

                // advance acrobatics
                if (mPtr == getPlayer())
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
            jumpstate = JumpState_Landing;
            vec.z() = 0.0f;

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
                    if (mPtr == getPlayer())
                        cls.skillUsageSucceeded(mPtr, ESM::Skill::Acrobatics, 1);
                }
            }
        }
        else
        {
            jumpstate = JumpState_None;
            vec.z() = 0.0f;

            inJump = false;

            if(std::abs(vec.x()/2.0f) > std::abs(vec.y()))
            {
                if(vec.x() > 0.0f)
                    movestate = (inwater ? (isrunning ? CharState_SwimRunRight : CharState_SwimWalkRight)
                                         : (sneak ? CharState_SneakRight
                                                  : (isrunning ? CharState_RunRight : CharState_WalkRight)));
                else if(vec.x() < 0.0f)
                    movestate = (inwater ? (isrunning ? CharState_SwimRunLeft : CharState_SwimWalkLeft)
                                         : (sneak ? CharState_SneakLeft
                                                  : (isrunning ? CharState_RunLeft : CharState_WalkLeft)));
            }
            else if(vec.y() != 0.0f)
            {
                if(vec.y() > 0.0f)
                    movestate = (inwater ? (isrunning ? CharState_SwimRunForward : CharState_SwimWalkForward)
                                         : (sneak ? CharState_SneakForward
                                                  : (isrunning ? CharState_RunForward : CharState_WalkForward)));
                else if(vec.y() < 0.0f)
                    movestate = (inwater ? (isrunning ? CharState_SwimRunBack : CharState_SwimWalkBack)
                                         : (sneak ? CharState_SneakBack
                                                  : (isrunning ? CharState_RunBack : CharState_WalkBack)));
            }
            else if(rot.z() != 0.0f && !inwater && !sneak && !MWBase::Environment::get().getWorld()->isFirstPerson())
            {
                if(rot.z() > 0.0f)
                    movestate = CharState_TurnRight;
                else if(rot.z() < 0.0f)
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

        if(mAnimQueue.empty() || inwater || sneak)
        {
            idlestate = (inwater ? CharState_IdleSwim : (sneak && !inJump ? CharState_IdleSneak : CharState_Idle));
        }
        else if(mAnimQueue.size() > 1)
        {
            if(mAnimation->isPlaying(mAnimQueue.front().first) == false)
            {
                mAnimation->disable(mAnimQueue.front().first);
                mAnimQueue.pop_front();

                mAnimation->play(mAnimQueue.front().first, Priority_Default,
                                 MWRender::Animation::BlendMask_All, false,
                                 1.0f, "start", "stop", 0.0f, mAnimQueue.front().second);
            }
        }

        if (!mSkipAnim)
        {
            // bipedal means hand-to-hand could be used (which is handled in updateWeaponState). an existing InventoryStore means an actual weapon could be used.
            if(cls.isBipedal(mPtr) || cls.hasInventoryStore(mPtr))
                forcestateupdate = updateWeaponState() || forcestateupdate;
            else
                forcestateupdate = updateCreatureState() || forcestateupdate;

            refreshCurrentAnims(idlestate, movestate, jumpstate, forcestateupdate);
            updateIdleStormState(inwater);
        }

        if (inJump)
            mMovementAnimationControlled = false;

        if (mMovementState == CharState_TurnLeft || mMovementState == CharState_TurnRight)
        {
            if (duration > 0)
                mAnimation->adjustSpeedMult(mCurrentMovement, std::min(1.5f, std::abs(rot.z()) / duration / static_cast<float>(osg::PI)));
        }
        else if (mMovementState != CharState_None && mAdjustMovementAnimSpeed)
        {
            float speedmult = speed / mMovementAnimSpeed;
            mAnimation->adjustSpeedMult(mCurrentMovement, speedmult);
        }

        if (!mSkipAnim)
        {
            if(mHitState != CharState_KnockDown && mHitState != CharState_KnockOut)
            {
                world->rotateObject(mPtr, rot.x(), rot.y(), rot.z(), true);
            }
            else //avoid z-rotating for knockdown
                world->rotateObject(mPtr, rot.x(), rot.y(), 0.0f, true);

            if (!mMovementAnimationControlled)
                world->queueMovement(mPtr, vec);
        }
        else
            // We must always queue movement, even if there is none, to apply gravity.
            world->queueMovement(mPtr, osg::Vec3f(0.f, 0.f, 0.f));

        movement = vec;
        cls.getMovementSettings(mPtr).mPosition[0] = cls.getMovementSettings(mPtr).mPosition[1] = 0;
        // Can't reset jump state (mPosition[2]) here; we don't know for sure whether the PhysicSystem will actually handle it in this frame
        // due to the fixed minimum timestep used for the physics update. It will be reset in PhysicSystem::move once the jump is handled.

        if (!mSkipAnim)
            updateHeadTracking(duration);
    }
    else if(cls.getCreatureStats(mPtr).isDead())
    {
        // initial start of death animation for actors that started the game as dead
        // not done in constructor since we need to give scripts a chance to set the mSkipAnim flag
        if (!mSkipAnim && mDeathState != CharState_None && mCurrentDeath.empty())
        {
            playDeath(1.f, mDeathState);
        }

        world->queueMovement(mPtr, osg::Vec3f(0.f, 0.f, 0.f));
    }

    osg::Vec3f moved = mAnimation->runAnimation(mSkipAnim ? 0.f : duration);
    if(duration > 0.0f)
        moved /= duration;
    else
        moved = osg::Vec3f(0.f, 0.f, 0.f);

    // Ensure we're moving in generally the right direction...
    if(speed > 0.f)
    {
        float l = moved.length();

        if((movement.x() < 0.0f && movement.x() < moved.x()*2.0f) ||
           (movement.x() > 0.0f && movement.x() > moved.x()*2.0f))
            moved.x() = movement.x();
        if((movement.y() < 0.0f && movement.y() < moved.y()*2.0f) ||
           (movement.y() > 0.0f && movement.y() > moved.y()*2.0f))
            moved.y() = movement.y();
        if((movement.z() < 0.0f && movement.z() < moved.z()*2.0f) ||
           (movement.z() > 0.0f && movement.z() > moved.z()*2.0f))
            moved.z() = movement.z();
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


bool CharacterController::playGroup(const std::string &groupname, int mode, int count)
{
    if(!mAnimation || !mAnimation->hasAnimation(groupname))
    {
        std::cerr<< "Animation "<<groupname<<" not found for " << mPtr.getCellRef().getRefId() << std::endl;
        return false;
    }
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
                             MWRender::Animation::BlendMask_All, false, 1.0f,
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
    return true;
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

    refreshCurrentAnims(mIdleState, mMovementState, mJumpState, true);
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
        if( mPtr == getPlayer() && !isAnimPlaying(mCurrentDeath) )
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
    if(mPtr == getPlayer())
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
        if (mPtr == getPlayer())
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

bool CharacterController::isSneaking() const
{
    return mIdleState == CharState_IdleSneak ||
            mMovementState == CharState_SneakForward ||
            mMovementState == CharState_SneakBack ||
            mMovementState == CharState_SneakLeft ||
            mMovementState == CharState_SneakRight;
}

void CharacterController::setAttackingOrSpell(bool attackingOrSpell)
{
    mAttackingOrSpell = attackingOrSpell;
}

bool CharacterController::readyToPrepareAttack() const
{
    return (mHitState == CharState_None || mHitState == CharState_Block)
            && mUpperBodyState  <= UpperCharState_WeapEquiped;
}

bool CharacterController::readyToStartAttack() const
{
    if (mHitState != CharState_None && mHitState != CharState_Block)
        return false;

    if (mPtr.getClass().hasInventoryStore(mPtr) || mPtr.getClass().isBipedal(mPtr))
        return mUpperBodyState == UpperCharState_WeapEquiped;
    else
        return mUpperBodyState == UpperCharState_Nothing;
}

float CharacterController::getAttackStrength() const
{
    return mAttackStrength;
}

void CharacterController::setActive(bool active)
{
    mAnimation->setActive(active);
}

void CharacterController::setHeadTrackTarget(const MWWorld::Ptr &target)
{
    mHeadTrackTarget = target;
}

void CharacterController::updateHeadTracking(float duration)
{
    const osg::Node* head = mAnimation->getNode("Bip01 Head");
    if (!head)
        return;

    float zAngleRadians = 0.f;
    float xAngleRadians = 0.f;

    if (!mHeadTrackTarget.isEmpty())
    {
        osg::MatrixList mats = head->getWorldMatrices();
        if (mats.empty())
            return;
        osg::Matrixf mat = mats[0];
        osg::Vec3f headPos = mat.getTrans();

        osg::Vec3f direction;
        if (MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(mHeadTrackTarget))
        {
            const osg::Node* node = anim->getNode("Head");
            if (node == NULL)
                node = anim->getNode("Bip01 Head");
            if (node != NULL)
            {
                osg::MatrixList mats = node->getWorldMatrices();
                if (mats.size())
                    direction = mats[0].getTrans() - headPos;
            }
            else
                // no head node to look at, fall back to look at center of collision box
                direction = MWBase::Environment::get().getWorld()->aimToTarget(mPtr, mHeadTrackTarget);
        }
        direction.normalize();

        if (!mPtr.getRefData().getBaseNode())
            return;
        const osg::Vec3f actorDirection = mPtr.getRefData().getBaseNode()->getAttitude() * osg::Vec3f(0,1,0);

        zAngleRadians = std::atan2(direction.x(), direction.y()) - std::atan2(actorDirection.x(), actorDirection.y());
        xAngleRadians = -std::asin(direction.z());

        wrap(zAngleRadians);
        wrap(xAngleRadians);

        xAngleRadians = std::min(xAngleRadians, osg::DegreesToRadians(40.f));
        xAngleRadians = std::max(xAngleRadians, osg::DegreesToRadians(-40.f));
        zAngleRadians = std::min(zAngleRadians, osg::DegreesToRadians(30.f));
        zAngleRadians = std::max(zAngleRadians, osg::DegreesToRadians(-30.f));
    }

    float factor = duration*5;
    factor = std::min(factor, 1.f);
    xAngleRadians = (1.f-factor) * mAnimation->getHeadPitch() + factor * (-xAngleRadians);
    zAngleRadians = (1.f-factor) * mAnimation->getHeadYaw() + factor * (-zAngleRadians);

    mAnimation->setHeadPitch(xAngleRadians);
    mAnimation->setHeadYaw(zAngleRadians);
}

}
