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
 * https://www.gnu.org/licenses/ .
 */

#include "character.hpp"

#include <iostream>

#include <components/misc/rng.hpp>

#include <components/settings/settings.hpp>

#include <components/sceneutil/positionattitudetransform.hpp>

#include "../mwrender/animation.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/player.hpp"

#include "aicombataction.hpp"
#include "movement.hpp"
#include "npcstats.hpp"
#include "creaturestats.hpp"
#include "security.hpp"
#include "actorutil.hpp"
#include "spellcasting.hpp"

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

std::string getBestAttack (const ESM::Weapon* weapon)
{
    int slash = (weapon->mData.mSlash[0] + weapon->mData.mSlash[1])/2;
    int chop = (weapon->mData.mChop[0] + weapon->mData.mChop[1])/2;
    int thrust = (weapon->mData.mThrust[0] + weapon->mData.mThrust[1])/2;
    if (slash == chop && slash == thrust)
        return "slash";
    else if (thrust >= chop && thrust >= slash)
        return "thrust";
    else if (slash >= chop && slash >= thrust)
        return "slash";
    else
        return "chop";
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

    const float fallDistanceMin = store.find("fFallDamageDistanceMin")->mValue.getFloat();

    if (fallHeight >= fallDistanceMin)
    {
        const float acrobaticsSkill = static_cast<float>(ptr.getClass().getSkill(ptr, ESM::Skill::Acrobatics));
        const float jumpSpellBonus = ptr.getClass().getCreatureStats(ptr).getMagicEffects().get(ESM::MagicEffect::Jump).getMagnitude();
        const float fallAcroBase = store.find("fFallAcroBase")->mValue.getFloat();
        const float fallAcroMult = store.find("fFallAcroMult")->mValue.getFloat();
        const float fallDistanceBase = store.find("fFallDistanceBase")->mValue.getFloat();
        const float fallDistanceMult = store.find("fFallDistanceMult")->mValue.getFloat();

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
    { CharState_SwimTurnLeft, "swimturnleft" },
    { CharState_SwimTurnRight, "swimturnright" },
};
static const StateInfo *sMovementListEnd = &sMovementList[sizeof(sMovementList)/sizeof(sMovementList[0])];


class FindCharState {
    CharacterState state;

public:
    FindCharState(CharacterState _state) : state(_state) { }

    bool operator()(const StateInfo &info) const
    { return info.state == state; }
};


std::string CharacterController::chooseRandomGroup (const std::string& prefix, int* num) const
{
    int numAnims=0;
    while (mAnimation->hasAnimation(prefix + std::to_string(numAnims+1)))
        ++numAnims;

    int roll = Misc::Rng::rollDice(numAnims) + 1; // [1, numAnims]
    if (num)
        *num = roll;
    return prefix + std::to_string(roll);
}

void CharacterController::refreshHitRecoilAnims(CharacterState& idle)
{
    bool recovery = mPtr.getClass().getCreatureStats(mPtr).getHitRecovery();
    bool knockdown = mPtr.getClass().getCreatureStats(mPtr).getKnockedDown();
    bool block = mPtr.getClass().getCreatureStats(mPtr).getBlock();
    bool isSwimming = MWBase::Environment::get().getWorld()->isSwimming(mPtr);
    if(mHitState == CharState_None)
    {
        if ((mPtr.getClass().getCreatureStats(mPtr).getFatigue().getCurrent() < 0
                || mPtr.getClass().getCreatureStats(mPtr).getFatigue().getBase() == 0)
                && mAnimation->hasAnimation("knockout"))
        {
            mTimeUntilWake = Misc::Rng::rollClosedProbability() * 2 + 1; // Wake up after 1 to 3 seconds
            if (isSwimming && mAnimation->hasAnimation("swimknockout"))
            {
                mHitState = CharState_SwimKnockOut;
                mCurrentHit = "swimknockout";
            }
            else
            {
                mHitState = CharState_KnockOut;
                mCurrentHit = "knockout";
            }

            mAnimation->play(mCurrentHit, Priority_Knockdown, MWRender::Animation::BlendMask_All, false, 1, "start", "stop", 0.0f, ~0ul);
            mPtr.getClass().getCreatureStats(mPtr).setKnockedDown(true);
        }
        else if(knockdown && mAnimation->hasAnimation("knockdown"))
        {
            if (isSwimming && mAnimation->hasAnimation("swimknockdown"))
            {
                mHitState = CharState_SwimKnockDown;
                mCurrentHit = "swimknockdown";
            }
            else
            {
                mHitState = CharState_KnockDown;
                mCurrentHit = "knockdown";
            }

            mAnimation->play(mCurrentHit, Priority_Knockdown, MWRender::Animation::BlendMask_All, true, 1, "start", "stop", 0.0f, 0);
        }
        else if (recovery)
        {
            std::string anim = chooseRandomGroup("swimhit");
            if (isSwimming && mAnimation->hasAnimation(anim))
            {
                mHitState = CharState_SwimHit;
                mCurrentHit = anim;
                mAnimation->play(mCurrentHit, Priority_Hit, MWRender::Animation::BlendMask_All, true, 1, "start", "stop", 0.0f, 0);
            }
            else
            {
                anim = chooseRandomGroup("hit");
                if (mAnimation->hasAnimation(anim))
                {
                    mHitState = CharState_Hit;
                    mCurrentHit = anim;
                    mAnimation->play(mCurrentHit, Priority_Hit, MWRender::Animation::BlendMask_All, true, 1, "start", "stop", 0.0f, 0);
                }
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
        if (isKnockedOut() || isKnockedDown())
        {
            if (mUpperBodyState > UpperCharState_WeapEquiped)
            {
                mAnimation->disable(mCurrentWeapon);
                mUpperBodyState = UpperCharState_WeapEquiped;
                if (mWeaponType > ESM::Weapon::None)
                    mAnimation->showWeapons(true);
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
    else if (isKnockedOut() && mPtr.getClass().getCreatureStats(mPtr).getFatigue().getCurrent() > 0 
            && mTimeUntilWake <= 0)
    {
        mHitState = isSwimming ? CharState_SwimKnockDown : CharState_KnockDown;
        mAnimation->disable(mCurrentHit);
        mAnimation->play(mCurrentHit, Priority_Knockdown, MWRender::Animation::BlendMask_All, true, 1, "loop stop", "stop", 0.0f, 0);
    }
    if (mHitState != CharState_None)
        idle = CharState_None;
}

void CharacterController::refreshJumpAnims(const std::string& weapShortGroup, JumpingState jump, CharacterState& idle, bool force)
{
    if (!force && jump == mJumpState && idle == CharState_None)
        return;

    std::string jumpAnimName;
    MWRender::Animation::BlendMask jumpmask = MWRender::Animation::BlendMask_All;
    if (jump != JumpState_None)
    {
        jumpAnimName = "jump";
        if(!weapShortGroup.empty())
        {
            jumpAnimName += weapShortGroup;
            if(!mAnimation->hasAnimation(jumpAnimName))
            {
                jumpAnimName = fallbackShortWeaponGroup("jump", &jumpmask);

                // If we apply jump only for lower body, do not reset idle animations.
                // For upper body there will be idle animation.
                if (jumpmask == MWRender::Animation::BlendMask_LowerBody && idle == CharState_None)
                    idle = CharState_Idle;
            }
        }
    }

    if (!force && jump == mJumpState)
        return;

    bool startAtLoop = (jump == mJumpState);
    mJumpState = jump;

    if (!mCurrentJump.empty())
    {
        mAnimation->disable(mCurrentJump);
        mCurrentJump.clear();
    }

    if(mJumpState == JumpState_InAir)
    {
        if (mAnimation->hasAnimation(jumpAnimName))
        {
            mAnimation->play(jumpAnimName, Priority_Jump, jumpmask, false,
                         1.0f, startAtLoop ? "loop start" : "start", "stop", 0.f, ~0ul);
            mCurrentJump = jumpAnimName;
        }
    }
    else if (mJumpState == JumpState_Landing)
    {
        if (mAnimation->hasAnimation(jumpAnimName))
        {
            mAnimation->play(jumpAnimName, Priority_Jump, jumpmask, true,
                         1.0f, "loop stop", "stop", 0.0f, 0);
            mCurrentJump = jumpAnimName;
        }
    }
}

bool CharacterController::onOpen()
{
    if (mPtr.getTypeName() == typeid(ESM::Container).name())
    {
        if (!mAnimation->hasAnimation("containeropen"))
            return true;

        if (mAnimation->isPlaying("containeropen"))
            return false;

        if (mAnimation->isPlaying("containerclose"))
            return false;

        mAnimation->play("containeropen", Priority_Persistent, MWRender::Animation::BlendMask_All, false, 1.0f, "start", "stop", 0.f, 0);
        if (mAnimation->isPlaying("containeropen"))
            return false;
    }

    return true;
}

void CharacterController::onClose()
{
    if (mPtr.getTypeName() == typeid(ESM::Container).name())
    {
        if (!mAnimation->hasAnimation("containerclose"))
            return;

        float complete, startPoint = 0.f;
        bool animPlaying = mAnimation->getInfo("containeropen", &complete);
        if (animPlaying)
            startPoint = 1.f - complete;

        mAnimation->play("containerclose", Priority_Persistent, MWRender::Animation::BlendMask_All, false, 1.0f, "start", "stop", startPoint, 0);
    }
}

std::string CharacterController::getWeaponAnimation(int weaponType) const
{
    std::string weaponGroup = getWeaponType(weaponType)->mLongGroup;
    bool isRealWeapon = weaponType != ESM::Weapon::HandToHand && weaponType != ESM::Weapon::Spell && weaponType != ESM::Weapon::None;
    if (isRealWeapon && !mAnimation->hasAnimation(weaponGroup))
    {
        static const std::string oneHandFallback = getWeaponType(ESM::Weapon::LongBladeOneHand)->mLongGroup;
        static const std::string twoHandFallback = getWeaponType(ESM::Weapon::LongBladeTwoHand)->mLongGroup;

        const ESM::WeaponType* weapInfo = getWeaponType(weaponType);

        // For real two-handed melee weapons use 2h swords animations as fallback, otherwise use the 1h ones
        if (weapInfo->mFlags & ESM::WeaponType::TwoHanded && weapInfo->mWeaponClass == ESM::WeaponType::Melee)
            weaponGroup = twoHandFallback;
        else if (isRealWeapon)
            weaponGroup = oneHandFallback;
    }

    return weaponGroup;
}

std::string CharacterController::fallbackShortWeaponGroup(const std::string& baseGroupName, MWRender::Animation::BlendMask* blendMask)
{
    bool isRealWeapon = mWeaponType != ESM::Weapon::HandToHand && mWeaponType != ESM::Weapon::Spell && mWeaponType != ESM::Weapon::None;
    if (!isRealWeapon)
    {
        if (blendMask != nullptr)
            *blendMask = MWRender::Animation::BlendMask_LowerBody;

        return baseGroupName;
    }

    static const std::string oneHandFallback = getWeaponType(ESM::Weapon::LongBladeOneHand)->mShortGroup;
    static const std::string twoHandFallback = getWeaponType(ESM::Weapon::LongBladeTwoHand)->mShortGroup;

    std::string groupName = baseGroupName;
    const ESM::WeaponType* weapInfo = getWeaponType(mWeaponType);

    // For real two-handed melee weapons use 2h swords animations as fallback, otherwise use the 1h ones
    if (isRealWeapon && weapInfo->mFlags & ESM::WeaponType::TwoHanded && weapInfo->mWeaponClass == ESM::WeaponType::Melee)
        groupName += twoHandFallback;
    else if (isRealWeapon)
        groupName += oneHandFallback;

    // Special case for crossbows - we shouls apply 1h animations a fallback only for lower body
    if (mWeaponType == ESM::Weapon::MarksmanCrossbow && blendMask != nullptr)
        *blendMask = MWRender::Animation::BlendMask_LowerBody;

    if (!mAnimation->hasAnimation(groupName))
    {
        groupName = baseGroupName;
        if (blendMask != nullptr)
            *blendMask = MWRender::Animation::BlendMask_LowerBody;
    }

    return groupName;
}

void CharacterController::refreshMovementAnims(const std::string& weapShortGroup, CharacterState movement, CharacterState& idle, bool force)
{
    if (movement == mMovementState && idle == mIdleState && !force)
        return;

    // Reset idle if we actually play movement animations excepts of these cases:
    // 1. When we play turning animations
    // 2. When we use a fallback animation for lower body since movement animation for given weapon is missing (e.g. for crossbows and spellcasting)
    bool resetIdle = (movement != CharState_None && !isTurning());

    std::string movementAnimName;
    MWRender::Animation::BlendMask movemask;
    const StateInfo *movestate;

    movemask = MWRender::Animation::BlendMask_All;
    movestate = std::find_if(sMovementList, sMovementListEnd, FindCharState(movement));
    if(movestate != sMovementListEnd)
    {
        movementAnimName = movestate->groupname;
        if(!weapShortGroup.empty())
        {
            std::string::size_type swimpos = movementAnimName.find("swim");
            if (swimpos == std::string::npos)
            {
                if (mWeaponType == ESM::Weapon::Spell && (movement == CharState_TurnLeft || movement == CharState_TurnRight)) // Spellcasting stance turning is a special case
                    movementAnimName = weapShortGroup + movementAnimName;
                else
                    movementAnimName += weapShortGroup;
            }

            if(!mAnimation->hasAnimation(movementAnimName))
            {
                movementAnimName = movestate->groupname;
                if (swimpos == std::string::npos)
                {
                    movementAnimName = fallbackShortWeaponGroup(movementAnimName, &movemask);

                    // If we apply movement only for lower body, do not reset idle animations.
                    // For upper body there will be idle animation.
                    if (movemask == MWRender::Animation::BlendMask_LowerBody && idle == CharState_None)
                        idle = CharState_Idle;

                    if (movemask == MWRender::Animation::BlendMask_LowerBody)
                        resetIdle = false;
                }
            }
        }
    }

    if(force || movement != mMovementState)
    {
        mMovementState = movement;
        if(movestate != sMovementListEnd)
        {
            if(!mAnimation->hasAnimation(movementAnimName))
            {
                std::string::size_type swimpos = movementAnimName.find("swim");
                if (swimpos != std::string::npos)
                {
                    movementAnimName.erase(swimpos, 4);
                    if (!weapShortGroup.empty())
                    {
                        std::string weapMovementAnimName = movementAnimName + weapShortGroup;
                        if(mAnimation->hasAnimation(weapMovementAnimName))
                            movementAnimName = weapMovementAnimName;
                        else
                        {
                            movementAnimName = fallbackShortWeaponGroup(movementAnimName, &movemask);
                            if (movemask == MWRender::Animation::BlendMask_LowerBody)
                                resetIdle = false;
                        }
                    }
                }

                if (swimpos == std::string::npos || !mAnimation->hasAnimation(movementAnimName))
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
            }
        }

        // If we're playing the same animation, start it from the point it ended
        float startpoint = 0.f;
        if (!mCurrentMovement.empty() && movementAnimName == mCurrentMovement)
            mAnimation->getInfo(mCurrentMovement, &startpoint);

        mMovementAnimationControlled = true;

        mAnimation->disable(mCurrentMovement);

        if (!mAnimation->hasAnimation(movementAnimName))
            movementAnimName.clear();

        mCurrentMovement = movementAnimName;
        if(!mCurrentMovement.empty())
        {
            if (resetIdle)
            {
                mAnimation->disable(mCurrentIdle);
                mIdleState = CharState_None;
                idle = CharState_None;
            }

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
                    bool sneaking = mMovementState == CharState_SneakForward || mMovementState == CharState_SneakBack
                                 || mMovementState == CharState_SneakLeft    || mMovementState == CharState_SneakRight;
                    mMovementAnimSpeed = (sneaking ? 33.5452f : (isRunning() ? 222.857f : 154.064f));
                    mMovementAnimationControlled = false;
                }
            }

            mAnimation->play(mCurrentMovement, Priority_Movement, movemask, false,
                             1.f, "start", "stop", startpoint, ~0ul, true);
        }
        else
            mMovementState = CharState_None;
    }
}

void CharacterController::refreshIdleAnims(const std::string& weapShortGroup, CharacterState idle, bool force)
{
    // FIXME: if one of the below states is close to their last animation frame (i.e. will be disabled in the coming update),
    // the idle animation should be displayed
    if (((mUpperBodyState != UpperCharState_Nothing && mUpperBodyState != UpperCharState_WeapEquiped)
            || (mMovementState != CharState_None && !isTurning())
            || mHitState != CharState_None)
            && !mPtr.getClass().isBipedal(mPtr))
        idle = CharState_None;

    if(force || idle != mIdleState || (!mAnimation->isPlaying(mCurrentIdle) && mAnimQueue.empty()))
    {
        mIdleState = idle;
        size_t numLoops = ~0ul;

        std::string idleGroup;
        MWRender::Animation::AnimPriority idlePriority (Priority_Default);
        // Only play "idleswim" or "idlesneak" if they exist. Otherwise, fallback to
        // "idle"+weapon or "idle".
        if(mIdleState == CharState_IdleSwim && mAnimation->hasAnimation("idleswim"))
        {
            idleGroup = "idleswim";
            idlePriority = Priority_SwimIdle;
        }
        else if(mIdleState == CharState_IdleSneak && mAnimation->hasAnimation("idlesneak"))
        {
            idleGroup = "idlesneak";
            idlePriority[MWRender::Animation::BoneGroup_LowerBody] = Priority_SneakIdleLowerBody;
        }
        else if(mIdleState != CharState_None)
        {
            idleGroup = "idle";
            if(!weapShortGroup.empty())
            {
                idleGroup += weapShortGroup;
                if(!mAnimation->hasAnimation(idleGroup))
                {
                    idleGroup = fallbackShortWeaponGroup("idle");
                }

                // play until the Loop Stop key 2 to 5 times, then play until the Stop key
                // this replicates original engine behavior for the "Idle1h" 1st-person animation
                numLoops = 1 + Misc::Rng::rollDice(4); 
            }
        }

        // There is no need to restart anim if the new and old anims are the same.
        // Just update a number of loops.
        float startPoint = 0;
        if (!mCurrentIdle.empty() && mCurrentIdle == idleGroup)
        {
            mAnimation->getInfo(mCurrentIdle, &startPoint);
        }

        if(!mCurrentIdle.empty())
            mAnimation->disable(mCurrentIdle);

        mCurrentIdle = idleGroup;
        if(!mCurrentIdle.empty())
            mAnimation->play(mCurrentIdle, idlePriority, MWRender::Animation::BlendMask_All, false,
                             1.0f, "start", "stop", startPoint, numLoops, true);
    }
}

void CharacterController::refreshCurrentAnims(CharacterState idle, CharacterState movement, JumpingState jump, bool force)
{
    // If the current animation is persistent, do not touch it
    if (isPersistentAnimPlaying())
        return;

    if (mPtr.getClass().isActor())
        refreshHitRecoilAnims(idle);

    std::string weap;
    if (mPtr.getClass().hasInventoryStore(mPtr))
        weap = getWeaponType(mWeaponType)->mShortGroup;

    refreshJumpAnims(weap, jump, idle, force);
    refreshMovementAnims(weap, movement, idle, force);

    // idle handled last as it can depend on the other states
    refreshIdleAnims(weap, idle, force);
}

void CharacterController::playDeath(float startpoint, CharacterState death)
{
    // Make sure the character was swimming upon death for forward-compatibility
    const bool wasSwimming = MWBase::Environment::get().getWorld()->isSwimming(mPtr);

    switch (death)
    {
    case CharState_SwimDeath:
        mCurrentDeath = "swimdeath";
        break;
    case CharState_SwimDeathKnockDown:
        mCurrentDeath = (wasSwimming ? "swimdeathknockdown" : "deathknockdown");
        break;
    case CharState_SwimDeathKnockOut:
        mCurrentDeath = (wasSwimming ? "swimdeathknockout" : "deathknockout");
        break;
    case CharState_DeathKnockDown:
        mCurrentDeath = "deathknockdown";
        break;
    case CharState_DeathKnockOut:
        mCurrentDeath = "deathknockout";
        break;
    default:
        mCurrentDeath = "death" + std::to_string(death - CharState_Death1 + 1);
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

CharacterState CharacterController::chooseRandomDeathState() const
{
    int selected=0;
    chooseRandomGroup("death", &selected);
    return static_cast<CharacterState>(CharState_Death1 + (selected-1));
}

void CharacterController::playRandomDeath(float startpoint)
{
    if (mPtr == getPlayer())
    {
        // The first-person animations do not include death, so we need to
        // force-switch to third person before playing the death animation.
        MWBase::Environment::get().getWorld()->useDeathCamera();
    }

    if(mHitState == CharState_SwimKnockDown && mAnimation->hasAnimation("swimdeathknockdown"))
    {
        mDeathState = CharState_SwimDeathKnockDown;
    }
    else if(mHitState == CharState_SwimKnockOut && mAnimation->hasAnimation("swimdeathknockout"))
    {
        mDeathState = CharState_SwimDeathKnockOut;
    }
    else if(MWBase::Environment::get().getWorld()->isSwimming(mPtr) && mAnimation->hasAnimation("swimdeath"))
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
        mDeathState = chooseRandomDeathState();
    }

    // Do not interrupt scripted animation by death
    if (isPersistentAnimPlaying())
        return;

    playDeath(startpoint, mDeathState);
}

std::string CharacterController::chooseRandomAttackAnimation() const
{
    std::string result;
    bool isSwimming = MWBase::Environment::get().getWorld()->isSwimming(mPtr);

    if (isSwimming)
        result = chooseRandomGroup("swimattack");

    if (!isSwimming || !mAnimation->hasAnimation(result))
        result = chooseRandomGroup("attack");

    return result;
}

CharacterController::CharacterController(const MWWorld::Ptr &ptr, MWRender::Animation *anim)
    : mPtr(ptr)
    , mWeapon(MWWorld::Ptr())
    , mAnimation(anim)
    , mIdleState(CharState_None)
    , mMovementState(CharState_None)
    , mMovementAnimSpeed(0.f)
    , mAdjustMovementAnimSpeed(false)
    , mHasMovedInXY(false)
    , mMovementAnimationControlled(true)
    , mDeathState(CharState_None)
    , mFloatToSurface(true)
    , mHitState(CharState_None)
    , mUpperBodyState(UpperCharState_Nothing)
    , mJumpState(JumpState_None)
    , mWeaponType(ESM::Weapon::None)
    , mAttackStrength(0.f)
    , mSkipAnim(false)
    , mSecondsOfSwimming(0)
    , mSecondsOfRunning(0)
    , mTurnAnimationThreshold(0)
    , mAttackingOrSpell(false)
    , mCastingManualSpell(false)
    , mTimeUntilWake(0.f)
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
            getActiveWeapon(mPtr, &mWeaponType);
            if (mWeaponType != ESM::Weapon::None)
            {
                mUpperBodyState = UpperCharState_WeapEquiped;
                mCurrentWeapon = getWeaponAnimation(mWeaponType);
            }

            if(mWeaponType != ESM::Weapon::None && mWeaponType != ESM::Weapon::Spell && mWeaponType != ESM::Weapon::HandToHand)
            {
                mAnimation->showWeapons(true);
                // Note: controllers for ranged weapon should use time for beginning of animation to play shooting properly,
                // for other weapons they should use absolute time. Some mods rely on this behaviour (to rotate throwing projectiles, for example)
                ESM::WeaponType::Class weaponClass = getWeaponType(mWeaponType)->mWeaponClass;
                bool useRelativeDuration = weaponClass == ESM::WeaponType::Ranged;
                mAnimation->setWeaponGroup(mCurrentWeapon, useRelativeDuration);
            }

            mAnimation->showCarriedLeft(updateCarriedLeftVisible(mWeaponType));
        }

        if(!cls.getCreatureStats(mPtr).isDead())
            mIdleState = CharState_Idle;
        else
        {
            const MWMechanics::CreatureStats& cStats = mPtr.getClass().getCreatureStats(mPtr);
            if (cStats.isDeathAnimationFinished())
            {
                // Set the death state, but don't play it yet
                // We will play it in the first frame, but only if no script set the skipAnim flag
                signed char deathanim = cStats.getDeathAnimation();
                if (deathanim == -1)
                    mDeathState = chooseRandomDeathState();
                else
                    mDeathState = static_cast<CharacterState>(CharState_Death1 + deathanim);

                mFloatToSurface = false;
            }
            // else: nothing to do, will detect death in the next frame and start playing death animation
        }
    }
    else
    {
        /* Don't accumulate with non-actors. */
        mAnimation->setAccumulation(osg::Vec3f(0.f, 0.f, 0.f));

        mIdleState = CharState_Idle;
    }

    // Do not update animation status for dead actors
    if(mDeathState == CharState_None && (!cls.isActor() || !cls.getCreatureStats(mPtr).isDead()))
        refreshCurrentAnims(mIdleState, mMovementState, mJumpState, true);

    mAnimation->runAnimation(0.f);

    unpersistAnimationState();
}

CharacterController::~CharacterController()
{
    if (mAnimation)
    {
        persistAnimationState();
        mAnimation->setTextKeyListener(nullptr);
    }
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
            // NB: landing sound is not played for NPCs here
            if(soundgen == "left" || soundgen == "right" || soundgen == "land")
            {
                sndMgr->playSound3D(mPtr, sound, volume, pitch, MWSound::Type::Foot,
                                    MWSound::PlayMode::NoPlayerLocal);
            }
            else
            {
                sndMgr->playSound3D(mPtr, sound, volume, pitch);
            }
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

    if(groupname == "shield" && evt.compare(off, len, "equip attach") == 0)
        mAnimation->showCarriedLeft(true);
    else if(groupname == "shield" && evt.compare(off, len, "unequip detach") == 0)
        mAnimation->showCarriedLeft(false);
    else if(evt.compare(off, len, "equip attach") == 0)
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
        if (groupname == "attack1" || groupname == "swimattack1")
            mPtr.getClass().hit(mPtr, mAttackStrength, ESM::Weapon::AT_Chop);
        else if (groupname == "attack2" || groupname == "swimattack2")
            mPtr.getClass().hit(mPtr, mAttackStrength, ESM::Weapon::AT_Slash);
        else if (groupname == "attack3" || groupname == "swimattack3")
            mPtr.getClass().hit(mPtr, mAttackStrength, ESM::Weapon::AT_Thrust);
        else
            mPtr.getClass().hit(mPtr, mAttackStrength);
    }
    else if (!groupname.empty()
             && (groupname.compare(0, groupname.size()-1, "attack") == 0 || groupname.compare(0, groupname.size()-1, "swimattack") == 0)
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
            if (groupname == "attack1" || groupname == "swimattack1")
                mPtr.getClass().hit(mPtr, mAttackStrength, ESM::Weapon::AT_Chop);
            else if (groupname == "attack2" || groupname == "swimattack2")
                mPtr.getClass().hit(mPtr, mAttackStrength, ESM::Weapon::AT_Slash);
            else if (groupname == "attack3" || groupname == "swimattack3")
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
        MWBase::Environment::get().getWorld()->castSpell(mPtr, mCastingManualSpell);
        mCastingManualSpell = false;
    }

    else if (groupname == "shield" && evt.compare(off, len, "block hit") == 0)
        mPtr.getClass().block(mPtr);
    else if (groupname == "containeropen" && evt.compare(off, len, "loot") == 0)
        MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Container, mPtr);
}

void CharacterController::updatePtr(const MWWorld::Ptr &ptr)
{
    mPtr = ptr;
}

void CharacterController::updateIdleStormState(bool inwater)
{
    if (!mAnimation->hasAnimation("idlestorm") || mUpperBodyState != UpperCharState_Nothing || inwater)
    {
        mAnimation->disable("idlestorm");
        return;
    }

    if (MWBase::Environment::get().getWorld()->isInStorm())
    {
        osg::Vec3f stormDirection = MWBase::Environment::get().getWorld()->getStormDirection();
        osg::Vec3f characterDirection = mPtr.getRefData().getBaseNode()->getAttitude() * osg::Vec3f(0,1,0);
        stormDirection.normalize();
        characterDirection.normalize();
        if (stormDirection * characterDirection < -0.5f)
        {
            if (!mAnimation->isPlaying("idlestorm"))
            {
                int mask = MWRender::Animation::BlendMask_Torso | MWRender::Animation::BlendMask_RightArm;
                mAnimation->play("idlestorm", Priority_Storm, mask, true, 1.0f, "start", "stop", 0.0f, ~0ul);
            }
            else
            {
                mAnimation->setLoopingEnabled("idlestorm", true);
            }
            return;
        }
    }

    if (mAnimation->isPlaying("idlestorm"))
    {
        mAnimation->setLoopingEnabled("idlestorm", false);
    }
}

bool CharacterController::updateCreatureState()
{
    const MWWorld::Class &cls = mPtr.getClass();
    CreatureStats &stats = cls.getCreatureStats(mPtr);

    int weapType = ESM::Weapon::None;
    if(stats.getDrawState() == DrawState_Weapon)
        weapType = ESM::Weapon::HandToHand;
    else if (stats.getDrawState() == DrawState_Spell)
        weapType = ESM::Weapon::Spell;

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
            if (weapType == ESM::Weapon::Spell)
            {
                const std::string spellid = stats.getSpells().getSelectedSpell();
                bool canCast = mCastingManualSpell || MWBase::Environment::get().getWorld()->startSpellCast(mPtr);

                if (!spellid.empty() && canCast)
                {
                    MWMechanics::CastSpell cast(mPtr, nullptr, false, mCastingManualSpell);
                    cast.playSpellCastingEffects(spellid, false);

                    if (!mAnimation->hasAnimation("spellcast"))
                    {
                        MWBase::Environment::get().getWorld()->castSpell(mPtr, mCastingManualSpell); // No "release" text key to use, so cast immediately
                        mCastingManualSpell = false;
                    }
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

            if (weapType != ESM::Weapon::Spell || !mAnimation->hasAnimation("spellcast")) // Not all creatures have a dedicated spellcast animation
            {
                mCurrentWeapon = chooseRandomAttackAnimation();
            }

            if (!mCurrentWeapon.empty())
            {
                mAnimation->play(mCurrentWeapon, Priority_Weapon,
                                 MWRender::Animation::BlendMask_All, true,
                                 1, startKey, stopKey,
                                 0.0f, 0);
                mUpperBodyState = UpperCharState_StartToMinAttack;

                mAttackStrength = std::min(1.f, 0.1f + Misc::Rng::rollClosedProbability());

                if (weapType == ESM::Weapon::HandToHand)
                    playSwishSound(0.0f);
            }
        }

        mAttackingOrSpell = false;
    }

    bool animPlaying = mAnimation->getInfo(mCurrentWeapon);
    if (!animPlaying)
        mUpperBodyState = UpperCharState_Nothing;
    return false;
}

bool CharacterController::updateCarriedLeftVisible(const int weaptype) const
{
    // Shields/torches shouldn't be visible during any operation involving two hands
    // There seems to be no text keys for this purpose, except maybe for "[un]equip start/stop",
    // but they are also present in weapon drawing animation.
    return mAnimation->updateCarriedLeftVisible(weaptype);
}

bool CharacterController::updateWeaponState(CharacterState& idle)
{
    const MWWorld::Class &cls = mPtr.getClass();
    CreatureStats &stats = cls.getCreatureStats(mPtr);
    int weaptype = ESM::Weapon::None;
    if(stats.getDrawState() == DrawState_Weapon)
        weaptype = ESM::Weapon::HandToHand;
    else if (stats.getDrawState() == DrawState_Spell)
        weaptype = ESM::Weapon::Spell;

    const bool isWerewolf = cls.isNpc() && cls.getNpcStats(mPtr).isWerewolf();

    std::string upSoundId;
    std::string downSoundId;
    bool weaponChanged = false;
    if (mPtr.getClass().hasInventoryStore(mPtr))
    {
        MWWorld::InventoryStore &inv = cls.getInventoryStore(mPtr);
        MWWorld::ContainerStoreIterator weapon = getActiveWeapon(mPtr, &weaptype);
        if(stats.getDrawState() == DrawState_Spell)
            weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);

        if(weapon != inv.end() && mWeaponType != ESM::Weapon::HandToHand && weaptype != ESM::Weapon::HandToHand && weaptype != ESM::Weapon::Spell && weaptype != ESM::Weapon::None)
            upSoundId = weapon->getClass().getUpSoundId(*weapon);

        if(weapon != inv.end() && mWeaponType != ESM::Weapon::HandToHand && mWeaponType != ESM::Weapon::Spell && mWeaponType != ESM::Weapon::None)
            downSoundId = weapon->getClass().getDownSoundId(*weapon);

        // weapon->HtH switch: weapon is empty already, so we need to take sound from previous weapon
        if(weapon == inv.end() && !mWeapon.isEmpty() && weaptype == ESM::Weapon::HandToHand && mWeaponType != ESM::Weapon::Spell)
            downSoundId = mWeapon.getClass().getDownSoundId(mWeapon);

        MWWorld::Ptr newWeapon = weapon != inv.end() ? *weapon : MWWorld::Ptr();

        if (mWeapon != newWeapon)
        {
            mWeapon = newWeapon;
            weaponChanged = true;
        }
    }

    // Use blending only with 3d-person movement animations for bipedal actors
    bool firstPersonPlayer = (mPtr == MWMechanics::getPlayer() && MWBase::Environment::get().getWorld()->isFirstPerson());
    MWRender::Animation::AnimPriority priorityWeapon(Priority_Weapon);
    if (!firstPersonPlayer && mPtr.getClass().isBipedal(mPtr))
        priorityWeapon[MWRender::Animation::BoneGroup_LowerBody] = Priority_WeaponLowerBody;

    bool forcestateupdate = false;

    // We should not play equipping animation and sound during weapon->weapon transition
    bool isStillWeapon = weaptype != ESM::Weapon::HandToHand && weaptype != ESM::Weapon::Spell && weaptype != ESM::Weapon::None &&
                            mWeaponType != ESM::Weapon::HandToHand && mWeaponType != ESM::Weapon::Spell && mWeaponType != ESM::Weapon::None;

    // If the current weapon type was changed in the middle of attack (e.g. by Equip console command or when bound spell expires),
    // we should force actor to the "weapon equipped" state, interrupt attack and update animations.
    if (isStillWeapon && mWeaponType != weaptype && mUpperBodyState > UpperCharState_WeapEquiped)
    {
        forcestateupdate = true;
        mUpperBodyState = UpperCharState_WeapEquiped;
        mAttackingOrSpell = false;
        mAnimation->disable(mCurrentWeapon);
        mAnimation->showWeapons(true);
        if (mPtr == getPlayer())
            MWBase::Environment::get().getWorld()->getPlayer().setAttackingOrSpell(false);
    }

    if(!isKnockedOut() && !isKnockedDown() && !isRecovery())
    {
        std::string weapgroup;
        if ((!isWerewolf || mWeaponType != ESM::Weapon::Spell)
            && weaptype != mWeaponType
            && mUpperBodyState != UpperCharState_UnEquipingWeap
            && !isStillWeapon)
        {
            // We can not play un-equip animation if weapon changed since last update
            if (!weaponChanged)
            {
                // Note: we do not disable unequipping animation automatically to avoid body desync
                weapgroup = getWeaponAnimation(mWeaponType);
                int unequipMask = MWRender::Animation::BlendMask_All;
                bool useShieldAnims = mAnimation->useShieldAnimations();
                if (useShieldAnims && mWeaponType != ESM::Weapon::HandToHand && mWeaponType != ESM::Weapon::Spell && !(mWeaponType == ESM::Weapon::None && weaptype == ESM::Weapon::Spell))
                {
                    unequipMask = unequipMask |~MWRender::Animation::BlendMask_LeftArm;
                    mAnimation->play("shield", Priority_Block,
                                MWRender::Animation::BlendMask_LeftArm, true,
                                1.0f, "unequip start", "unequip stop", 0.0f, 0);
                }
                else if (mWeaponType == ESM::Weapon::HandToHand)
                    mAnimation->showCarriedLeft(false);

                mAnimation->play(weapgroup, priorityWeapon, unequipMask, false,
                                1.0f, "unequip start", "unequip stop", 0.0f, 0);
                mUpperBodyState = UpperCharState_UnEquipingWeap;

                // If we do not have the "unequip detach" key, hide weapon manually.
                if (mAnimation->getTextKeyTime(weapgroup+": unequip detach") < 0)
                    mAnimation->showWeapons(false);
            }

            if(!downSoundId.empty())
            {
                MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
                sndMgr->playSound3D(mPtr, downSoundId, 1.0f, 1.0f);
            }
        }

        float complete;
        bool animPlaying = mAnimation->getInfo(mCurrentWeapon, &complete);
        if (!animPlaying || complete >= 1.0f)
        {
            // Weapon is changed, no current animation (e.g. unequipping or attack).
            // Start equipping animation now.
            if (weaptype != mWeaponType)
            {
                forcestateupdate = true;
                bool useShieldAnims = mAnimation->useShieldAnimations();
                if (!useShieldAnims)
                    mAnimation->showCarriedLeft(updateCarriedLeftVisible(weaptype));

                weapgroup = getWeaponAnimation(weaptype);

                // Note: controllers for ranged weapon should use time for beginning of animation to play shooting properly,
                // for other weapons they should use absolute time. Some mods rely on this behaviour (to rotate throwing projectiles, for example)
                ESM::WeaponType::Class weaponClass = getWeaponType(weaptype)->mWeaponClass;
                bool useRelativeDuration = weaponClass == ESM::WeaponType::Ranged;
                mAnimation->setWeaponGroup(weapgroup, useRelativeDuration);

                if (!isStillWeapon)
                {
                    mAnimation->disable(mCurrentWeapon);
                    if (weaptype != ESM::Weapon::None)
                    {
                        mAnimation->showWeapons(false);
                        int equipMask = MWRender::Animation::BlendMask_All;
                        if (useShieldAnims && weaptype != ESM::Weapon::Spell)
                        {
                            equipMask = equipMask |~MWRender::Animation::BlendMask_LeftArm;
                            mAnimation->play("shield", Priority_Block,
                                        MWRender::Animation::BlendMask_LeftArm, true,
                                        1.0f, "equip start", "equip stop", 0.0f, 0);
                        }

                        mAnimation->play(weapgroup, priorityWeapon, equipMask, true,
                                        1.0f, "equip start", "equip stop", 0.0f, 0);
                        mUpperBodyState = UpperCharState_EquipingWeap;

                        // If we do not have the "equip attach" key, show weapon manually.
                        if (weaptype != ESM::Weapon::Spell)
                        {
                            if (mAnimation->getTextKeyTime(weapgroup+": equip attach") < 0)
                                mAnimation->showWeapons(true);
                        }
                    }
                }

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

                mWeaponType = weaptype;
                mCurrentWeapon = getWeaponAnimation(mWeaponType);

                if(!upSoundId.empty() && !isStillWeapon)
                {
                    MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
                    sndMgr->playSound3D(mPtr, upSoundId, 1.0f, 1.0f);
                }
            }

            // Make sure that we disabled unequipping animation
            if (mUpperBodyState == UpperCharState_UnEquipingWeap)
            {
                mUpperBodyState = UpperCharState_Nothing;
                mAnimation->disable(mCurrentWeapon);
                mWeaponType = ESM::Weapon::None;
                mCurrentWeapon = getWeaponAnimation(mWeaponType);
            }
        }
    }

    if(isWerewolf)
    {
        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        if(cls.getCreatureStats(mPtr).getStance(MWMechanics::CreatureStats::Stance_Run)
            && mHasMovedInXY
            && !MWBase::Environment::get().getWorld()->isSwimming(mPtr)
            && mWeaponType == ESM::Weapon::None)
        {
            if(!sndMgr->getSoundPlaying(mPtr, "WolfRun"))
                sndMgr->playSound3D(mPtr, "WolfRun", 1.0f, 1.0f, MWSound::Type::Sfx,
                                    MWSound::PlayMode::Loop);
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
        MWWorld::ConstContainerStoreIterator weapon = getActiveWeapon(mPtr, &weaptype);
        isWeapon = (weapon != inv.end() && weapon->getTypeName() == typeid(ESM::Weapon).name());
        if (isWeapon)
        {
            weapSpeed = weapon->get<ESM::Weapon>()->mBase->mData.mSpeed;
            MWWorld::ConstContainerStoreIterator ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);
            int ammotype = getWeaponType(weapon->get<ESM::Weapon>()->mBase->mData.mType)->mAmmoType;
            if (ammotype != ESM::Weapon::None && (ammo == inv.end() || ammo->get<ESM::Weapon>()->mBase->mData.mType != ammotype))
                ammunition = false;
        }

        if (!ammunition && mUpperBodyState > UpperCharState_WeapEquiped)
        {
            mAnimation->disable(mCurrentWeapon);
            mUpperBodyState = UpperCharState_WeapEquiped;
        }
    }

    // Combat for actors with persistent animations obviously will be buggy
    if (isPersistentAnimPlaying())
        return forcestateupdate;

    float complete;
    bool animPlaying;
    ESM::WeaponType::Class weapclass = getWeaponType(mWeaponType)->mWeaponClass;
    if(mAttackingOrSpell)
    {
        MWWorld::Ptr player = getPlayer();

        bool resetIdle = ammunition;
        if(mUpperBodyState == UpperCharState_WeapEquiped && (mHitState == CharState_None || mHitState == CharState_Block))
        {
            MWBase::Environment::get().getWorld()->breakInvisibility(mPtr);
            mAttackStrength = 0;

            // Randomize attacks for non-bipedal creatures with Weapon flag
            if (mPtr.getClass().getTypeName() == typeid(ESM::Creature).name() &&
                !mPtr.getClass().isBipedal(mPtr) &&
                (!mAnimation->hasAnimation(mCurrentWeapon) || isRandomAttackAnimation(mCurrentWeapon)))
            {
                mCurrentWeapon = chooseRandomAttackAnimation();
            }

            if(mWeaponType == ESM::Weapon::Spell)
            {
                // Unset casting flag, otherwise pressing the mouse button down would
                // continue casting every frame if there is no animation
                mAttackingOrSpell = false;
                if (mPtr == player)
                {
                    MWBase::Environment::get().getWorld()->getPlayer().setAttackingOrSpell(false);

                    // For the player, set the spell we want to cast
                    // This has to be done at the start of the casting animation,
                    // *not* when selecting a spell in the GUI (otherwise you could change the spell mid-animation)
                    std::string selectedSpell = MWBase::Environment::get().getWindowManager()->getSelectedSpell();
                    stats.getSpells().setSelectedSpell(selectedSpell);
                }
                std::string spellid = stats.getSpells().getSelectedSpell();
                bool isMagicItem = false;
                bool canCast = mCastingManualSpell || MWBase::Environment::get().getWorld()->startSpellCast(mPtr);

                if (spellid.empty())
                {
                    if (mPtr.getClass().hasInventoryStore(mPtr))
                    {
                        MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
                        if (inv.getSelectedEnchantItem() != inv.end())
                        {
                            const MWWorld::Ptr& enchantItem = *inv.getSelectedEnchantItem();
                            spellid = enchantItem.getClass().getEnchantment(enchantItem);
                            isMagicItem = true;
                        }
                    }
                }

                static const bool useCastingAnimations = Settings::Manager::getBool("use magic item animations", "Game");
                if (isMagicItem && !useCastingAnimations)
                {
                    // Enchanted items by default do not use casting animations
                    MWBase::Environment::get().getWorld()->castSpell(mPtr);
                    resetIdle = false;
                }
                else if(!spellid.empty() && canCast)
                {
                    MWMechanics::CastSpell cast(mPtr, nullptr, false, mCastingManualSpell);
                    cast.playSpellCastingEffects(spellid, isMagicItem);

                    std::vector<ESM::ENAMstruct> effects;
                    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
                    if (isMagicItem)
                    {
                        const ESM::Enchantment *enchantment = store.get<ESM::Enchantment>().find(spellid);
                        effects = enchantment->mEffects.mList;
                    }
                    else
                    {
                        const ESM::Spell *spell = store.get<ESM::Spell>().find(spellid);
                        effects = spell->mEffects.mList;
                    }

                    const ESM::MagicEffect *effect = store.get<ESM::MagicEffect>().find(effects.back().mEffectID); // use last effect of list for color of VFX_Hands

                    const ESM::Static* castStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find ("VFX_Hands");

                    for (size_t iter = 0; iter < effects.size(); ++iter) // play hands vfx for each effect
                    {
                        if (mAnimation->getNode("Bip01 L Hand"))
                            mAnimation->addEffect("meshes\\" + castStatic->mModel, -1, false, "Bip01 L Hand", effect->mParticle);

                        if (mAnimation->getNode("Bip01 R Hand"))
                            mAnimation->addEffect("meshes\\" + castStatic->mModel, -1, false, "Bip01 R Hand", effect->mParticle);
                    }

                    const ESM::ENAMstruct &firstEffect = effects.at(0); // first effect used for casting animation

                    std::string startKey;
                    std::string stopKey;
                    if (isRandomAttackAnimation(mCurrentWeapon))
                    {
                        startKey = "start";
                        stopKey = "stop";
                        MWBase::Environment::get().getWorld()->castSpell(mPtr, mCastingManualSpell); // No "release" text key to use, so cast immediately
                        mCastingManualSpell = false;
                    }
                    else
                    {
                        switch(firstEffect.mRange)
                        {
                            case 0: mAttackType = "self"; break;
                            case 1: mAttackType = "touch"; break;
                            case 2: mAttackType = "target"; break;
                        }

                        startKey = mAttackType+" start";
                        stopKey = mAttackType+" stop";
                    }

                    mAnimation->play(mCurrentWeapon, priorityWeapon,
                                     MWRender::Animation::BlendMask_All, true,
                                     1, startKey, stopKey,
                                     0.0f, 0);
                    mUpperBodyState = UpperCharState_CastingSpell;
                }
                else
                {
                    resetIdle = false;
                }
            }
            else if(mWeaponType == ESM::Weapon::PickProbe)
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
                    MWBase::Environment::get().getSoundManager()->playSound3D(target, resultSound,
                                                                              1.0f, 1.0f);
            }
            else if (ammunition)
            {
                std::string startKey;
                std::string stopKey;

                if(weapclass == ESM::WeaponType::Ranged || weapclass == ESM::WeaponType::Thrown)
                {
                    mAttackType = "shoot";
                    startKey = mAttackType+" start";
                    stopKey = mAttackType+" min attack";
                }
                else if (isRandomAttackAnimation(mCurrentWeapon))
                {
                    startKey = "start";
                    stopKey = "stop";
                }
                else
                {
                    if(mPtr == getPlayer())
                    {
                        if (Settings::Manager::getBool("best attack", "Game"))
                        {
                            if (isWeapon)
                            {
                                MWWorld::ConstContainerStoreIterator weapon = mPtr.getClass().getInventoryStore(mPtr).getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
                                mAttackType = getBestAttack(weapon->get<ESM::Weapon>()->mBase);
                            }
                            else
                            {
                                // There is no "best attack" for Hand-to-Hand
                                setAttackTypeRandomly(mAttackType);
                            }
                        }
                        else
                        {
                            setAttackTypeBasedOnMovement();
                        }
                    }
                    // else if (mPtr != getPlayer()) use mAttackType set by AiCombat
                    startKey = mAttackType+" start";
                    stopKey = mAttackType+" min attack";
                }

                mAnimation->play(mCurrentWeapon, priorityWeapon,
                                 MWRender::Animation::BlendMask_All, false,
                                 weapSpeed, startKey, stopKey,
                                 0.0f, 0);
                mUpperBodyState = UpperCharState_StartToMinAttack;
            }
        }

        // We should not break swim and sneak animations
        if (resetIdle &&
            idle != CharState_IdleSneak && idle != CharState_IdleSwim &&
            mIdleState != CharState_IdleSneak && mIdleState != CharState_IdleSwim)
        {
            mAnimation->disable(mCurrentIdle);
            mIdleState = CharState_None;
        }

        animPlaying = mAnimation->getInfo(mCurrentWeapon, &complete);
        if(mUpperBodyState == UpperCharState_MinAttackToMaxAttack && !isKnockedDown())
            mAttackStrength = complete;
    }
    else
    {
        animPlaying = mAnimation->getInfo(mCurrentWeapon, &complete);
        if(mUpperBodyState == UpperCharState_MinAttackToMaxAttack && !isKnockedDown())
        {
            float attackStrength = complete;
            float minAttackTime = mAnimation->getTextKeyTime(mCurrentWeapon+": "+mAttackType+" "+"min attack");
            float maxAttackTime = mAnimation->getTextKeyTime(mCurrentWeapon+": "+mAttackType+" "+"max attack");
            if (minAttackTime == maxAttackTime)
            {
                // most creatures don't actually have an attack wind-up animation, so use a uniform random value
                // (even some creatures that can use weapons don't have a wind-up animation either, e.g. Rieklings)
                // Note: vanilla MW uses a random value for *all* non-player actors, but we probably don't need to go that far.
                attackStrength = std::min(1.f, 0.1f + Misc::Rng::rollClosedProbability());
            }

            if(weapclass != ESM::WeaponType::Ranged && weapclass != ESM::WeaponType::Thrown)
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
                    playSwishSound(attackStrength);
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
        else if (isKnockedDown())
        {
            if (mUpperBodyState > UpperCharState_WeapEquiped)
            {
                mUpperBodyState = UpperCharState_WeapEquiped;
                if (mWeaponType > ESM::Weapon::None)
                    mAnimation->showWeapons(true);
            }
            mAnimation->disable(mCurrentWeapon);
        }
    }

    mAnimation->setPitchFactor(0.f);
    if (weapclass == ESM::WeaponType::Ranged || weapclass == ESM::WeaponType::Thrown)
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
            {
                // technically we do not need a pitch for crossbow reload animation,
                // but we should avoid abrupt repositioning
                if (mWeaponType == ESM::Weapon::MarksmanCrossbow)
                    mAnimation->setPitchFactor(std::max(0.f, 1.f-complete*10.f));
                else
                    mAnimation->setPitchFactor(1.f-complete);
            }
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
            if (ammunition && mWeaponType == ESM::Weapon::MarksmanCrossbow)
                mAnimation->attachArrow();

            mUpperBodyState = UpperCharState_WeapEquiped;
        }
        else if(mUpperBodyState == UpperCharState_UnEquipingWeap)
            mUpperBodyState = UpperCharState_Nothing;
    }
    else if(complete >= 1.0f && !isRandomAttackAnimation(mCurrentWeapon))
    {
        std::string start, stop;
        switch(mUpperBodyState)
        {
            case UpperCharState_MinAttackToMaxAttack:
                //hack to avoid body pos desync when jumping/sneaking in 'max attack' state
                if(!mAnimation->isPlaying(mCurrentWeapon))
                    mAnimation->play(mCurrentWeapon, priorityWeapon,
                        MWRender::Animation::BlendMask_All, false,
                        0, mAttackType+" min attack", mAttackType+" max attack", 0.999f, 0);
                break;
            case UpperCharState_StartToMinAttack:
            case UpperCharState_MaxAttackToMinHit:
            {
                if (mUpperBodyState == UpperCharState_StartToMinAttack)
                {
                    // If actor is already stopped preparing attack, do not play the "min attack -> max attack" part.
                    // Happens if the player did not hold the attack button.
                    // Note: if the "min attack"->"max attack" is a stub, "play" it anyway. Attack strength will be random.
                    float minAttackTime = mAnimation->getTextKeyTime(mCurrentWeapon+": "+mAttackType+" "+"min attack");
                    float maxAttackTime = mAnimation->getTextKeyTime(mCurrentWeapon+": "+mAttackType+" "+"max attack");
                    if (mAttackingOrSpell || minAttackTime == maxAttackTime)
                    {
                        start = mAttackType+" min attack";
                        stop = mAttackType+" max attack";
                        mUpperBodyState = UpperCharState_MinAttackToMaxAttack;
                        break;
                    }
                    playSwishSound(0.0f);
                }

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
            }
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

        // Note: apply crossbow reload animation only for upper body
        // since blending with movement animations can give weird result.
        if(!start.empty())
        {
            int mask = MWRender::Animation::BlendMask_All;
            if (mWeaponType == ESM::Weapon::MarksmanCrossbow)
                mask = MWRender::Animation::BlendMask_UpperBody;

            mAnimation->disable(mCurrentWeapon);
            if (mUpperBodyState == UpperCharState_FollowStartToFollowStop)
                mAnimation->play(mCurrentWeapon, priorityWeapon,
                                 mask, true,
                                 weapSpeed, start, stop, 0.0f, 0);
            else
                mAnimation->play(mCurrentWeapon, priorityWeapon,
                                 mask, false,
                                 weapSpeed, start, stop, 0.0f, 0);
        }
    }
    else if(complete >= 1.0f && isRandomAttackAnimation(mCurrentWeapon))
    {
        mAnimation->disable(mCurrentWeapon);
        mUpperBodyState = UpperCharState_WeapEquiped;
    }

    if (mPtr.getClass().hasInventoryStore(mPtr))
    {
        const MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        MWWorld::ConstContainerStoreIterator torch = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
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

void CharacterController::updateAnimQueue()
{
    if(mAnimQueue.size() > 1)
    {
        if(mAnimation->isPlaying(mAnimQueue.front().mGroup) == false)
        {
            mAnimation->disable(mAnimQueue.front().mGroup);
            mAnimQueue.pop_front();

            bool loopfallback = (mAnimQueue.front().mGroup.compare(0,4,"idle") == 0);
            mAnimation->play(mAnimQueue.front().mGroup, Priority_Default,
                             MWRender::Animation::BlendMask_All, false,
                             1.0f, "start", "stop", 0.0f, mAnimQueue.front().mLoopCount, loopfallback);
        }
    }

    if(!mAnimQueue.empty())
        mAnimation->setLoopingEnabled(mAnimQueue.front().mGroup, mAnimQueue.size() <= 1);
}

void CharacterController::update(float duration, bool animationOnly)
{
    MWBase::World *world = MWBase::Environment::get().getWorld();
    const MWWorld::Class &cls = mPtr.getClass();
    osg::Vec3f movement(0.f, 0.f, 0.f);
    float speed = 0.f;

    updateMagicEffects();

    if (isKnockedOut())
        mTimeUntilWake -= duration;

    bool isPlayer = mPtr == MWMechanics::getPlayer();
    bool godmode = isPlayer && MWBase::Environment::get().getWorld()->getGodModeState();

    float scale = mPtr.getCellRef().getScale();

    static const bool normalizeSpeed = Settings::Manager::getBool("normalise race speed", "Game");
    if (!normalizeSpeed && mPtr.getClass().isNpc())
    {
        const ESM::NPC* npc = mPtr.get<ESM::NPC>()->mBase;
        const ESM::Race* race = world->getStore().get<ESM::Race>().find(npc->mRace);
        float weight = npc->isMale() ? race->mData.mWeight.mMale : race->mData.mWeight.mFemale;
        scale *= weight;
    }

    if(!cls.isActor())
        updateAnimQueue();
    else if(!cls.getCreatureStats(mPtr).isDead())
    {
        bool onground = world->isOnGround(mPtr);
        bool incapacitated = (cls.getCreatureStats(mPtr).isParalyzed() || cls.getCreatureStats(mPtr).getKnockedDown());
        bool inwater = world->isSwimming(mPtr);
        bool flying = world->isFlying(mPtr);
        bool solid = world->isActorCollisionEnabled(mPtr);
        // Can't run and sneak while flying (see speed formula in Npc/Creature::getSpeed)
        bool sneak = cls.getCreatureStats(mPtr).getStance(MWMechanics::CreatureStats::Stance_Sneak) && !flying;
        bool isrunning = cls.getCreatureStats(mPtr).getStance(MWMechanics::CreatureStats::Stance_Run) && !flying;
        CreatureStats &stats = cls.getCreatureStats(mPtr);

        //Force Jump Logic

        bool isMoving = (std::abs(cls.getMovementSettings(mPtr).mPosition[0]) > .5 || std::abs(cls.getMovementSettings(mPtr).mPosition[1]) > .5);
        if(!inwater && !flying && solid)
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
        float analogueMult = 1.f;
        if(isPlayer)
        {
            // Joystick analogue movement.
            float xAxis = std::abs(cls.getMovementSettings(mPtr).mPosition[0]);
            float yAxis = std::abs(cls.getMovementSettings(mPtr).mPosition[1]);
            analogueMult = ((xAxis > yAxis) ? xAxis : yAxis);

            // If Strafing, our max speed is slower so multiply by X axis instead.
            if(std::abs(vec.x()/2.0f) > std::abs(vec.y()))
                analogueMult = xAxis;

            // Due to the half way split between walking/running, we multiply speed by 2 while walking, unless a keyboard was used.
            if(!isrunning && !sneak && !flying && analogueMult <= 0.5f)
                analogueMult *= 2.f;
        }

        speed *= analogueMult;
        vec.x() *= speed;
        vec.y() *= speed;

        CharacterState movestate = CharState_None;
        CharacterState idlestate = CharState_SpecialIdle;
        JumpingState jumpstate = JumpState_None;

        bool forcestateupdate = false;

        mHasMovedInXY = std::abs(vec.x())+std::abs(vec.y()) > 0.0f;
        isrunning = isrunning && mHasMovedInXY;

        // advance athletics
        if(mHasMovedInXY && isPlayer)
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
            else if(isrunning && !sneak)
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
        static const float fFatigueRunBase = gmst.find("fFatigueRunBase")->mValue.getFloat();
        static const float fFatigueRunMult = gmst.find("fFatigueRunMult")->mValue.getFloat();
        static const float fFatigueSwimWalkBase = gmst.find("fFatigueSwimWalkBase")->mValue.getFloat();
        static const float fFatigueSwimRunBase = gmst.find("fFatigueSwimRunBase")->mValue.getFloat();
        static const float fFatigueSwimWalkMult = gmst.find("fFatigueSwimWalkMult")->mValue.getFloat();
        static const float fFatigueSwimRunMult = gmst.find("fFatigueSwimRunMult")->mValue.getFloat();
        static const float fFatigueSneakBase = gmst.find("fFatigueSneakBase")->mValue.getFloat();
        static const float fFatigueSneakMult = gmst.find("fFatigueSneakMult")->mValue.getFloat();

        if (cls.getEncumbrance(mPtr) <= cls.getCapacity(mPtr))
        {
            const float encumbrance = cls.getNormalizedEncumbrance(mPtr);
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
                else if (isrunning)
                    fatigueLoss = fFatigueRunBase + encumbrance * fFatigueRunMult;
            }
        }
        fatigueLoss *= duration;
        fatigueLoss *= analogueMult;
        DynamicStat<float> fatigue = cls.getCreatureStats(mPtr).getFatigue();

        if (!godmode)
        {
            fatigue.setCurrent(fatigue.getCurrent() - fatigueLoss, fatigue.getCurrent() < 0);
            cls.getCreatureStats(mPtr).setFatigue(fatigue);
        }

        float z = cls.getJump(mPtr);
        if(sneak || inwater || flying || incapacitated || !solid || z <= 0)
            vec.z() = 0.0f;

        bool inJump = true;
        bool playLandingSound = false;
        if(!onground && !flying && !inwater && solid)
        {
            // In the air (either getting up —ascending part of jump— or falling).

            forcestateupdate = (mJumpState != JumpState_InAir);
            jumpstate = JumpState_InAir;

            static const float fJumpMoveBase = gmst.find("fJumpMoveBase")->mValue.getFloat();
            static const float fJumpMoveMult = gmst.find("fJumpMoveMult")->mValue.getFloat();
            float factor = fJumpMoveBase + fJumpMoveMult * mPtr.getClass().getSkill(mPtr, ESM::Skill::Acrobatics)/100.f;
            factor = std::min(1.f, factor);
            vec.x() *= factor;
            vec.y() *= factor;
            vec.z()  = 0.0f;
        }
        else if(vec.z() > 0.0f && mJumpState != JumpState_InAir)
        {
            // Started a jump.
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
            }
        }
        else if(mJumpState == JumpState_InAir && !inwater && !flying && solid)
        {
            forcestateupdate = true;
            jumpstate = JumpState_Landing;
            vec.z() = 0.0f;

            // We should reset idle animation during landing
            mAnimation->disable(mCurrentIdle);

            float height = cls.getCreatureStats(mPtr).land(isPlayer);
            float healthLost = getFallDamage(mPtr, height);

            if (healthLost > 0.0f)
            {
                const float fatigueTerm = cls.getCreatureStats(mPtr).getFatigueTerm();

                // inflict fall damages
                if (!godmode)
                {
                    float realHealthLost = static_cast<float>(healthLost * (1.0f - 0.25f * fatigueTerm));
                    cls.onHit(mPtr, realHealthLost, true, MWWorld::Ptr(), MWWorld::Ptr(), osg::Vec3f(), true);
                }

                const int acrobaticsSkill = cls.getSkill(mPtr, ESM::Skill::Acrobatics);
                if (healthLost > (acrobaticsSkill * fatigueTerm))
                {
                    if (!godmode)
                        cls.getCreatureStats(mPtr).setKnockedDown(true);
                }
                else
                {
                    // report acrobatics progression
                    if (isPlayer)
                        cls.skillUsageSucceeded(mPtr, ESM::Skill::Acrobatics, 1);
                }
            }

            if (mPtr.getClass().isNpc())
                playLandingSound = true;
        }
        else
        {
            if(mPtr.getClass().isNpc() && mJumpState == JumpState_InAir && !flying && solid)
                playLandingSound = true;

            jumpstate = mAnimation->isPlaying(mCurrentJump) ? JumpState_Landing : JumpState_None;

            vec.x() *= scale;
            vec.y() *= scale;
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
            else if(rot.z() != 0.0f)
            {
                // Do not play turning animation for player if rotation speed is very slow.
                // Actual threshold should take framerate in account.
                float rotationThreshold = 0.f;
                if (isPlayer)
                    rotationThreshold = 0.015 * 60 * duration;

                // It seems only bipedal actors use turning animations.
                // Also do not use turning animations in the first-person view and when sneaking.
                bool isFirstPlayer = isPlayer && MWBase::Environment::get().getWorld()->isFirstPerson();
                if (!sneak && jumpstate == JumpState_None && !isFirstPlayer && mPtr.getClass().isBipedal(mPtr))
                {
                    if(rot.z() > rotationThreshold)
                        movestate = inwater ? CharState_SwimTurnRight : CharState_TurnRight;
                    else if(rot.z() < -rotationThreshold)
                        movestate = inwater ? CharState_SwimTurnLeft : CharState_TurnLeft;
                }
            }
        }

        if (playLandingSound)
        {
            MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
            std::string sound;
            osg::Vec3f pos(mPtr.getRefData().getPosition().asVec3());
            if (world->isUnderwater(mPtr.getCell(), pos) || world->isWalkingOnWater(mPtr))
                sound = "DefaultLandWater";
            else if (onground)
                sound = "DefaultLand";

            if (!sound.empty())
                sndMgr->playSound3D(mPtr, sound, 1.f, 1.f, MWSound::Type::Foot, MWSound::PlayMode::NoPlayerLocal);
        }

        // Player can not use smooth turning as NPCs, so we play turning animation a bit to avoid jittering
        if (isPlayer)
        {
            float threshold = mCurrentMovement.find("swim") == std::string::npos ? 0.4f : 0.8f;
            float complete;
            bool animPlaying = mAnimation->getInfo(mCurrentMovement, &complete);
            if (movestate == CharState_None && jumpstate == JumpState_None && isTurning())
            {
                if (animPlaying && complete < threshold)
                    movestate = mMovementState;
            }
        }
        else
        {
            if (mPtr.getClass().isBipedal(mPtr))
            {
                if (mTurnAnimationThreshold > 0)
                    mTurnAnimationThreshold -= duration;

                if (movestate == CharState_TurnRight || movestate == CharState_TurnLeft ||
                    movestate == CharState_SwimTurnRight || movestate == CharState_SwimTurnLeft)
                {
                    mTurnAnimationThreshold = 0.05f;
                }
                else if (movestate == CharState_None && isTurning()
                        && mTurnAnimationThreshold > 0)
                {
                    movestate = mMovementState;
                }
            }
        }

        if(movestate != CharState_None && !isTurning())
            clearAnimQueue();

        if(mAnimQueue.empty() || inwater || (sneak && mIdleState != CharState_SpecialIdle))
        {
            if (inwater)
                idlestate = CharState_IdleSwim;
            else if (sneak && !inJump)
                idlestate = CharState_IdleSneak;
            else
                idlestate = CharState_Idle;
        }
        else
            updateAnimQueue();

        if (!mSkipAnim)
        {
            // bipedal means hand-to-hand could be used (which is handled in updateWeaponState). an existing InventoryStore means an actual weapon could be used.
            if(cls.isBipedal(mPtr) || cls.hasInventoryStore(mPtr))
                forcestateupdate = updateWeaponState(idlestate) || forcestateupdate;
            else
                forcestateupdate = updateCreatureState() || forcestateupdate;

            refreshCurrentAnims(idlestate, movestate, jumpstate, forcestateupdate);
            updateIdleStormState(inwater);
        }

        if (inJump)
            mMovementAnimationControlled = false;

        if (isTurning())
        {
            // Adjust animation speed from 1.0 to 1.5 multiplier
            if (duration > 0)
            {
                float turnSpeed = std::min(1.5f, std::abs(rot.z()) / duration / static_cast<float>(osg::PI));
                mAnimation->adjustSpeedMult(mCurrentMovement, std::max(turnSpeed, 1.0f));
            }
        }
        else if (mMovementState != CharState_None && mAdjustMovementAnimSpeed)
        {
            float speedmult = speed / mMovementAnimSpeed;
            mAnimation->adjustSpeedMult(mCurrentMovement, speedmult);
        }

        if (!mSkipAnim)
        {
            if(!isKnockedDown() && !isKnockedOut())
            {
                if (rot != osg::Vec3f())
                    world->rotateObject(mPtr, rot.x(), rot.y(), rot.z(), true);
            }
            else //avoid z-rotating for knockdown
            {
                if (rot.x() != 0 && rot.y() != 0)
                    world->rotateObject(mPtr, rot.x(), rot.y(), 0.0f, true);
            }

            if (!animationOnly && !mMovementAnimationControlled)
                world->queueMovement(mPtr, vec);
        }
        else if (!animationOnly)
            // We must always queue movement, even if there is none, to apply gravity.
            world->queueMovement(mPtr, osg::Vec3f(0.f, 0.f, 0.f));

        movement = vec;
        cls.getMovementSettings(mPtr).mPosition[0] = cls.getMovementSettings(mPtr).mPosition[1] = 0;
        if (movement.z() == 0.f)
            cls.getMovementSettings(mPtr).mPosition[2] = 0;
        // Can't reset jump state (mPosition[2]) here in full; we don't know for sure whether the PhysicSystem will actually handle it in this frame
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
            // Fast-forward death animation to end for persisting corpses or corpses after end of death animation
            if (cls.isPersistent(mPtr) || cls.getCreatureStats(mPtr).isDeathAnimationFinished())
                playDeath(1.f, mDeathState);
        }
        // We must always queue movement, even if there is none, to apply gravity.
        if (!animationOnly)
            world->queueMovement(mPtr, osg::Vec3f(0.f, 0.f, 0.f));
    }

    bool isPersist = isPersistentAnimPlaying();
    osg::Vec3f moved = mAnimation->runAnimation(mSkipAnim && !isPersist ? 0.f : duration);
    if(duration > 0.0f)
        moved /= duration;
    else
        moved = osg::Vec3f(0.f, 0.f, 0.f);

    moved.x() *= scale;
    moved.y() *= scale;

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

    if (mFloatToSurface && cls.isActor() && cls.getCreatureStats(mPtr).isDead() && cls.canSwim(mPtr))
        moved.z() = 1.0;

    // Update movement
    if(!animationOnly && mMovementAnimationControlled && mPtr.getClass().isActor())
        world->queueMovement(mPtr, moved);

    mSkipAnim = false;

    mAnimation->enableHeadAnimation(cls.isActor() && !cls.getCreatureStats(mPtr).isDead());
}

void CharacterController::persistAnimationState()
{
    ESM::AnimationState& state = mPtr.getRefData().getAnimationState();

    state.mScriptedAnims.clear();
    for (AnimationQueue::const_iterator iter = mAnimQueue.begin(); iter != mAnimQueue.end(); ++iter)
    {
        if (!iter->mPersist)
            continue;

        ESM::AnimationState::ScriptedAnimation anim;
        anim.mGroup = iter->mGroup;

        if (iter == mAnimQueue.begin())
        {
            anim.mLoopCount = mAnimation->getCurrentLoopCount(anim.mGroup);
            float complete;
            mAnimation->getInfo(anim.mGroup, &complete, nullptr);
            anim.mTime = complete;
        }
        else
        {
            anim.mLoopCount = iter->mLoopCount;
            anim.mTime = 0.f;
        }

        state.mScriptedAnims.push_back(anim);
    }
}

void CharacterController::unpersistAnimationState()
{
    const ESM::AnimationState& state = mPtr.getRefData().getAnimationState();

    if (!state.mScriptedAnims.empty())
    {
        clearAnimQueue();
        for (ESM::AnimationState::ScriptedAnimations::const_iterator iter = state.mScriptedAnims.begin(); iter != state.mScriptedAnims.end(); ++iter)
        {
            AnimationQueueEntry entry;
            entry.mGroup = iter->mGroup;
            entry.mLoopCount = iter->mLoopCount;
            entry.mPersist = true;

            mAnimQueue.push_back(entry);
        }

        const ESM::AnimationState::ScriptedAnimation& anim = state.mScriptedAnims.front();
        float complete = anim.mTime;
        if (anim.mAbsolute)
        {
            float start = mAnimation->getTextKeyTime(anim.mGroup+": start");
            float stop = mAnimation->getTextKeyTime(anim.mGroup+": stop");
            float time = std::max(start, std::min(stop, anim.mTime));
            complete = (time - start) / (stop - start);
        }

        mAnimation->disable(mCurrentIdle);
        mCurrentIdle.clear();
        mIdleState = CharState_SpecialIdle;

        bool loopfallback = (mAnimQueue.front().mGroup.compare(0,4,"idle") == 0);
        mAnimation->play(anim.mGroup,
                         Priority_Persistent, MWRender::Animation::BlendMask_All, false, 1.0f,
                         "start", "stop", complete, anim.mLoopCount, loopfallback);
    }
}

bool CharacterController::playGroup(const std::string &groupname, int mode, int count, bool persist)
{
    if(!mAnimation || !mAnimation->hasAnimation(groupname))
        return false;

    // We should not interrupt persistent animations by non-persistent ones
    if (isPersistentAnimPlaying() && !persist)
        return false;

    // If this animation is a looped animation (has a "loop start" key) that is already playing
    // and has not yet reached the end of the loop, allow it to continue animating with its existing loop count
    // and remove any other animations that were queued.
    // This emulates observed behavior from the original allows the script "OutsideBanner" to animate banners correctly.
    if (!mAnimQueue.empty() && mAnimQueue.front().mGroup == groupname &&
        mAnimation->getTextKeyTime(mAnimQueue.front().mGroup + ": loop start") >= 0 &&
        mAnimation->isPlaying(groupname))
    {
        float endOfLoop = mAnimation->getTextKeyTime(mAnimQueue.front().mGroup+": loop stop");

        if (endOfLoop < 0) // if no Loop Stop key was found, use the Stop key
            endOfLoop = mAnimation->getTextKeyTime(mAnimQueue.front().mGroup+": stop");

        if (endOfLoop > 0 && (mAnimation->getCurrentTime(mAnimQueue.front().mGroup) < endOfLoop))
        {
            mAnimQueue.resize(1);
            return true;
        }
    }

    count = std::max(count, 1);

    AnimationQueueEntry entry;
    entry.mGroup = groupname;
    entry.mLoopCount = count-1;
    entry.mPersist = persist;

    if(mode != 0 || mAnimQueue.empty() || !isAnimPlaying(mAnimQueue.front().mGroup))
    {
        clearAnimQueue(persist);

        mAnimation->disable(mCurrentIdle);
        mCurrentIdle.clear();

        mIdleState = CharState_SpecialIdle;
        bool loopfallback = (entry.mGroup.compare(0,4,"idle") == 0);
        mAnimation->play(groupname, persist && groupname != "idle" ? Priority_Persistent : Priority_Default,
                            MWRender::Animation::BlendMask_All, false, 1.0f,
                            ((mode==2) ? "loop start" : "start"), "stop", 0.0f, count-1, loopfallback);
    }
    else
    {
        mAnimQueue.resize(1);
    }

    // "PlayGroup idle" is a special case, used to remove to stop scripted animations playing
    if (groupname == "idle")
        entry.mPersist = false;

    mAnimQueue.push_back(entry);

    return true;
}

void CharacterController::skipAnim()
{
    mSkipAnim = true;
}

bool CharacterController::isPersistentAnimPlaying()
{
    if (!mAnimQueue.empty())
    {
        AnimationQueueEntry& first = mAnimQueue.front();
        return first.mPersist && isAnimPlaying(first.mGroup);
    }

    return false;
}

bool CharacterController::isAnimPlaying(const std::string &groupName)
{
    if(mAnimation == nullptr)
        return false;
    return mAnimation->isPlaying(groupName);
}

void CharacterController::clearAnimQueue(bool clearPersistAnims)
{
    // Do not interrupt scripted animations, if we want to keep them
    if ((!isPersistentAnimPlaying() || clearPersistAnims) && !mAnimQueue.empty())
        mAnimation->disable(mAnimQueue.front().mGroup);

    for (AnimationQueue::iterator it = mAnimQueue.begin(); it != mAnimQueue.end();)
    {
        if (clearPersistAnims || !it->mPersist)
            it = mAnimQueue.erase(it);
        else
            ++it;
    }
}

void CharacterController::forceStateUpdate()
{
    if(!mAnimation)
        return;
    clearAnimQueue();

    // Make sure we canceled the current attack or spellcasting,
    // because we disabled attack animations anyway.
    mCastingManualSpell = false;
    mAttackingOrSpell = false;
    if (mUpperBodyState != UpperCharState_Nothing)
        mUpperBodyState = UpperCharState_WeapEquiped;

    refreshCurrentAnims(mIdleState, mMovementState, mJumpState, true);

    if(mDeathState != CharState_None)
    {
        playRandomDeath();
    }

    mAnimation->runAnimation(0.f);
}

CharacterController::KillResult CharacterController::kill()
{
    if (mDeathState == CharState_None)
    {
        playRandomDeath();

        mAnimation->disable(mCurrentIdle);

        mIdleState = CharState_None;
        mCurrentIdle.clear();
        return Result_DeathAnimStarted;
    }

    MWMechanics::CreatureStats& cStats = mPtr.getClass().getCreatureStats(mPtr);
    if (isAnimPlaying(mCurrentDeath))
        return Result_DeathAnimPlaying;
    if (!cStats.isDeathAnimationFinished())
    {
        cStats.setDeathAnimationFinished(true);
        return Result_DeathAnimJustFinished;
    }
    return Result_DeathAnimFinished;
}

void CharacterController::resurrect()
{
    if(mDeathState == CharState_None)
        return;

    if(mAnimation)
        mAnimation->disable(mCurrentDeath);
    mCurrentDeath.clear();
    mDeathState = CharState_None;
    mWeaponType = ESM::Weapon::None;
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
        if (mPtr.getClass().getCreatureStats(mPtr).isDeathAnimationFinished()
            || mPtr.getClass().getCreatureStats(mPtr).getMagicEffects().get(MWMechanics::EffectKey(*it)).getMagnitude() <= 0)
            mAnimation->removeEffect(*it);
    }
}

void CharacterController::updateMagicEffects()
{
    if (!mPtr.getClass().isActor())
        return;

    float light = mPtr.getClass().getCreatureStats(mPtr).getMagicEffects().get(ESM::MagicEffect::Light).getMagnitude();
    mAnimation->setLightEffect(light);

    // If you're dead you don't care about whether you've started/stopped being a vampire or not
    if (mPtr.getClass().getCreatureStats(mPtr).isDead())
        return;

    bool vampire = mPtr.getClass().getCreatureStats(mPtr).getMagicEffects().get(ESM::MagicEffect::Vampirism).getMagnitude() > 0.0f;
    mAnimation->setVampire(vampire);
}

void CharacterController::setVisibility(float visibility)
{
    // We should take actor's invisibility in account
    if (mPtr.getClass().isActor())
    {
        float alpha = 1.f;
        if (mPtr.getClass().getCreatureStats(mPtr).getMagicEffects().get(ESM::MagicEffect::Invisibility).getModifier()) // Ignore base magnitude (see bug #3555).
        {
            if (mPtr == getPlayer())
                alpha = 0.25f;
            else
                alpha = 0.05f;
        }
        float chameleon = mPtr.getClass().getCreatureStats(mPtr).getMagicEffects().get(ESM::MagicEffect::Chameleon).getMagnitude();
        if (chameleon)
        {
            alpha *= std::min(0.75f, std::max(0.25f, (100.f - chameleon)/100.f));
        }

        visibility = std::min(visibility, alpha);
    }

    // TODO: implement a dithering shader rather than just change object transparency.
    mAnimation->setAlpha(visibility);
}

void CharacterController::setAttackTypeBasedOnMovement()
{
    float *move = mPtr.getClass().getMovementSettings(mPtr).mPosition;

    if (move[1] && !move[0]) // forward-backward
        mAttackType = "thrust";
    else if (move[0] && !move[1]) //sideway
        mAttackType = "slash";
    else
        mAttackType = "chop";
}

bool CharacterController::isRandomAttackAnimation(const std::string& group) const
{
    return (group == "attack1" || group == "swimattack1" ||
            group == "attack2" || group == "swimattack2" ||
            group == "attack3" || group == "swimattack3");
}

bool CharacterController::isAttackPreparing() const
{
    return mUpperBodyState == UpperCharState_StartToMinAttack ||
            mUpperBodyState == UpperCharState_MinAttackToMaxAttack;
}

bool CharacterController::isCastingSpell() const
{
    return mCastingManualSpell || mUpperBodyState == UpperCharState_CastingSpell;
}

bool CharacterController::isReadyToBlock() const
{
    return updateCarriedLeftVisible(mWeaponType);
}

bool CharacterController::isKnockedDown() const
{
    return mHitState == CharState_KnockDown ||
            mHitState == CharState_SwimKnockDown;
}

bool CharacterController::isKnockedOut() const
{
    return mHitState == CharState_KnockOut ||
            mHitState == CharState_SwimKnockOut;
}

bool CharacterController::isTurning() const
{
    return mMovementState == CharState_TurnLeft ||
            mMovementState == CharState_TurnRight ||
            mMovementState == CharState_SwimTurnLeft ||
            mMovementState == CharState_SwimTurnRight;
}

bool CharacterController::isRecovery() const
{
    return mHitState == CharState_Hit ||
            mHitState == CharState_SwimHit;
}

bool CharacterController::isAttackingOrSpell() const
{
    return mUpperBodyState != UpperCharState_Nothing &&
            mUpperBodyState != UpperCharState_WeapEquiped;
}

bool CharacterController::isSneaking() const
{
    return mIdleState == CharState_IdleSneak ||
            mMovementState == CharState_SneakForward ||
            mMovementState == CharState_SneakBack ||
            mMovementState == CharState_SneakLeft ||
            mMovementState == CharState_SneakRight;
}

bool CharacterController::isRunning() const
{
    return mMovementState == CharState_RunForward ||
            mMovementState == CharState_RunBack ||
            mMovementState == CharState_RunLeft ||
            mMovementState == CharState_RunRight ||
            mMovementState == CharState_SwimRunForward ||
            mMovementState == CharState_SwimRunBack ||
            mMovementState == CharState_SwimRunLeft ||
            mMovementState == CharState_SwimRunRight;
}

void CharacterController::setAttackingOrSpell(bool attackingOrSpell)
{
    mAttackingOrSpell = attackingOrSpell;
}

void CharacterController::castSpell(const std::string spellId, bool manualSpell)
{
    mAttackingOrSpell = true;
    mCastingManualSpell = manualSpell;
    ActionSpell action = ActionSpell(spellId);
    action.prepare(mPtr);
}

void CharacterController::setAIAttackType(const std::string& attackType)
{
    mAttackType = attackType;
}

void CharacterController::setAttackTypeRandomly(std::string& attackType)
{
    float random = Misc::Rng::rollProbability();
    if (random >= 2/3.f)
        attackType = "thrust";
    else if (random >= 1/3.f)
        attackType = "slash";
    else
        attackType = "chop";
}

bool CharacterController::readyToPrepareAttack() const
{
    return (mHitState == CharState_None || mHitState == CharState_Block)
            && mUpperBodyState <= UpperCharState_WeapEquiped;
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

void CharacterController::setActive(int active)
{
    mAnimation->setActive(active);
}

void CharacterController::setHeadTrackTarget(const MWWorld::ConstPtr &target)
{
    mHeadTrackTarget = target;
}

void CharacterController::playSwishSound(float attackStrength)
{
    MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();

    std::string sound = "Weapon Swish";
    if(attackStrength < 0.5f)
        sndMgr->playSound3D(mPtr, sound, 1.0f, 0.8f); //Weak attack
    else if(attackStrength < 1.0f)
        sndMgr->playSound3D(mPtr, sound, 1.0f, 1.0f); //Medium attack
    else
        sndMgr->playSound3D(mPtr, sound, 1.0f, 1.2f); //Strong attack
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
        osg::NodePathList nodepaths = head->getParentalNodePaths();
        if (nodepaths.empty())
            return;
        osg::Matrixf mat = osg::computeLocalToWorld(nodepaths[0]);
        osg::Vec3f headPos = mat.getTrans();

        osg::Vec3f direction;
        if (const MWRender::Animation* anim = MWBase::Environment::get().getWorld()->getAnimation(mHeadTrackTarget))
        {
            const osg::Node* node = anim->getNode("Head");
            if (node == nullptr)
                node = anim->getNode("Bip01 Head");
            if (node != nullptr)
            {
                nodepaths = node->getParentalNodePaths();
                if (!nodepaths.empty())
                    direction = osg::computeLocalToWorld(nodepaths[0]).getTrans() - headPos;
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
