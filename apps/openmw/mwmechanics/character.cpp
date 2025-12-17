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

#include <array>
#include <unordered_set>

#include <components/esm/records.hpp>
#include <components/misc/mathutil.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/rng.hpp>
#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/conversion.hpp>

#include <components/settings/values.hpp>

#include <components/sceneutil/positionattitudetransform.hpp>

#include "../mwrender/animation.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/spellcaststate.hpp"

#include "actorutil.hpp"
#include "aicombataction.hpp"
#include "creaturestats.hpp"
#include "movement.hpp"
#include "npcstats.hpp"
#include "security.hpp"
#include "spellcasting.hpp"
#include "weapontype.hpp"

namespace
{

    std::string_view getBestAttack(const ESM::Weapon* weapon)
    {
        int slash = weapon->mData.mSlash[0] + weapon->mData.mSlash[1];
        int chop = weapon->mData.mChop[0] + weapon->mData.mChop[1];
        int thrust = weapon->mData.mThrust[0] + weapon->mData.mThrust[1];
        if (slash == chop && slash == thrust)
            return "slash";
        else if (thrust >= chop && thrust >= slash)
            return "thrust";
        else if (slash >= chop && slash >= thrust)
            return "slash";
        else
            return "chop";
    }

    // Converts a movement Run state to its equivalent Walk state, if there is one.
    MWMechanics::CharacterState runStateToWalkState(MWMechanics::CharacterState state)
    {
        using namespace MWMechanics;
        switch (state)
        {
            case CharState_RunForward:
                return CharState_WalkForward;
            case CharState_RunBack:
                return CharState_WalkBack;
            case CharState_RunLeft:
                return CharState_WalkLeft;
            case CharState_RunRight:
                return CharState_WalkRight;
            case CharState_SwimRunForward:
                return CharState_SwimWalkForward;
            case CharState_SwimRunBack:
                return CharState_SwimWalkBack;
            case CharState_SwimRunLeft:
                return CharState_SwimWalkLeft;
            case CharState_SwimRunRight:
                return CharState_SwimWalkRight;
            default:
                return state;
        }
    }

    // Converts a Hit state to its equivalent Death state.
    MWMechanics::CharacterState hitStateToDeathState(MWMechanics::CharacterState state)
    {
        using namespace MWMechanics;
        switch (state)
        {
            case CharState_SwimKnockDown:
                return CharState_SwimDeathKnockDown;
            case CharState_SwimKnockOut:
                return CharState_SwimDeathKnockOut;
            case CharState_KnockDown:
                return CharState_DeathKnockDown;
            case CharState_KnockOut:
                return CharState_DeathKnockOut;
            default:
                return CharState_None;
        }
    }

    // Converts a movement state to its equivalent base animation group as long as it is a movement state.
    std::string_view movementStateToAnimGroup(MWMechanics::CharacterState state)
    {
        using namespace MWMechanics;
        switch (state)
        {
            case CharState_WalkForward:
                return "walkforward";
            case CharState_WalkBack:
                return "walkback";
            case CharState_WalkLeft:
                return "walkleft";
            case CharState_WalkRight:
                return "walkright";

            case CharState_SwimWalkForward:
                return "swimwalkforward";
            case CharState_SwimWalkBack:
                return "swimwalkback";
            case CharState_SwimWalkLeft:
                return "swimwalkleft";
            case CharState_SwimWalkRight:
                return "swimwalkright";

            case CharState_RunForward:
                return "runforward";
            case CharState_RunBack:
                return "runback";
            case CharState_RunLeft:
                return "runleft";
            case CharState_RunRight:
                return "runright";

            case CharState_SwimRunForward:
                return "swimrunforward";
            case CharState_SwimRunBack:
                return "swimrunback";
            case CharState_SwimRunLeft:
                return "swimrunleft";
            case CharState_SwimRunRight:
                return "swimrunright";

            case CharState_SneakForward:
                return "sneakforward";
            case CharState_SneakBack:
                return "sneakback";
            case CharState_SneakLeft:
                return "sneakleft";
            case CharState_SneakRight:
                return "sneakright";

            case CharState_TurnLeft:
                return "turnleft";
            case CharState_TurnRight:
                return "turnright";
            case CharState_SwimTurnLeft:
                return "swimturnleft";
            case CharState_SwimTurnRight:
                return "swimturnright";
            default:
                return {};
        }
    }

    // Converts a death state to its equivalent animation group as long as it is a death state.
    std::string_view deathStateToAnimGroup(MWMechanics::CharacterState state)
    {
        using namespace MWMechanics;
        switch (state)
        {
            case CharState_SwimDeath:
                return "swimdeath";
            case CharState_SwimDeathKnockDown:
                return "swimdeathknockdown";
            case CharState_SwimDeathKnockOut:
                return "swimdeathknockout";
            case CharState_DeathKnockDown:
                return "deathknockdown";
            case CharState_DeathKnockOut:
                return "deathknockout";
            case CharState_Death1:
                return "death1";
            case CharState_Death2:
                return "death2";
            case CharState_Death3:
                return "death3";
            case CharState_Death4:
                return "death4";
            case CharState_Death5:
                return "death5";
            default:
                return {};
        }
    }

    // Converts a hit state to its equivalent animation group as long as it is a hit state.
    std::string hitStateToAnimGroup(MWMechanics::CharacterState state)
    {
        using namespace MWMechanics;
        switch (state)
        {
            case CharState_SwimHit:
                return "swimhit";
            case CharState_SwimKnockDown:
                return "swimknockdown";
            case CharState_SwimKnockOut:
                return "swimknockout";

            case CharState_Hit:
                return "hit";
            case CharState_KnockDown:
                return "knockdown";
            case CharState_KnockOut:
                return "knockout";

            case CharState_Block:
                return "shield";

            default:
                return {};
        }
    }

    // Converts an idle state to its equivalent animation group.
    std::string idleStateToAnimGroup(MWMechanics::CharacterState state)
    {
        using namespace MWMechanics;
        switch (state)
        {
            case CharState_IdleSwim:
                return "idleswim";
            case CharState_IdleSneak:
                return "idlesneak";
            case CharState_Idle:
            case CharState_SpecialIdle:
                return "idle";
            default:
                return {};
        }
    }

    MWRender::Animation::AnimPriority getIdlePriority(MWMechanics::CharacterState state)
    {
        using namespace MWMechanics;
        MWRender::Animation::AnimPriority priority(Priority_Default);
        switch (state)
        {
            case CharState_IdleSwim:
                return Priority_SwimIdle;
            case CharState_IdleSneak:
                priority[MWRender::BoneGroup_LowerBody] = Priority_SneakIdleLowerBody;
                [[fallthrough]];
            default:
                return priority;
        }
    }

    float getFallDamage(const MWWorld::Ptr& ptr, float fallHeight)
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        const MWWorld::Store<ESM::GameSetting>& store = world->getStore().get<ESM::GameSetting>();

        const float fallDistanceMin = store.find("fFallDamageDistanceMin")->mValue.getFloat();

        if (fallHeight >= fallDistanceMin)
        {
            const float acrobaticsSkill = static_cast<float>(ptr.getClass().getSkill(ptr, ESM::Skill::Acrobatics));
            const float jumpSpellBonus = ptr.getClass()
                                             .getCreatureStats(ptr)
                                             .getMagicEffects()
                                             .getOrDefault(ESM::MagicEffect::Jump)
                                             .getMagnitude();
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

    bool isRealWeapon(int weaponType)
    {
        return weaponType != ESM::Weapon::HandToHand && weaponType != ESM::Weapon::Spell
            && weaponType != ESM::Weapon::None;
    }

}

namespace MWMechanics
{

    std::string CharacterController::chooseRandomGroup(const std::string& prefix, int* num) const
    {
        auto& prng = MWBase::Environment::get().getWorld()->getPrng();

        int numAnims = 0;
        while (mAnimation->hasAnimation(prefix + std::to_string(numAnims + 1)))
            ++numAnims;

        int roll = Misc::Rng::rollDice(numAnims, prng) + 1; // [1, numAnims]
        if (num)
            *num = roll;
        return prefix + std::to_string(roll);
    }

    void CharacterController::clearStateAnimation(std::string& anim) const
    {
        if (anim.empty())
            return;
        if (mAnimation)
            mAnimation->disable(anim);
        anim.clear();
    }

    void CharacterController::resetCurrentJumpState()
    {
        clearStateAnimation(mCurrentJump);
        mJumpState = JumpState_None;
    }

    void CharacterController::resetCurrentMovementState()
    {
        clearStateAnimation(mCurrentMovement);
        mMovementState = CharState_None;
        mMovementAnimationHasMovement = false;
    }

    void CharacterController::resetCurrentIdleState()
    {
        clearStateAnimation(mCurrentIdle);
        mIdleState = CharState_None;
    }

    void CharacterController::resetCurrentHitState()
    {
        clearStateAnimation(mCurrentHit);
        mHitState = CharState_None;
    }

    void CharacterController::resetCurrentWeaponState()
    {
        clearStateAnimation(mCurrentWeapon);
        mUpperBodyState = UpperBodyState::None;
    }

    void CharacterController::resetCurrentDeathState()
    {
        clearStateAnimation(mCurrentDeath);
        mDeathState = CharState_None;
    }

    void CharacterController::refreshHitRecoilAnims()
    {
        auto& charClass = mPtr.getClass();
        if (!charClass.isActor())
            return;
        const auto world = MWBase::Environment::get().getWorld();
        auto& stats = charClass.getCreatureStats(mPtr);
        bool knockout = stats.getFatigue().getCurrent() < 0 || stats.getFatigue().getBase() == 0;
        bool recovery = stats.getHitRecovery();
        bool knockdown = stats.getKnockedDown();
        bool block = stats.getBlock() && !knockout && !recovery && !knockdown;
        bool isSwimming = world->isSwimming(mPtr);

        stats.setBlock(false);

        if (mPtr == getPlayer() && mHitState == CharState_Block && block)
        {
            mHitState = CharState_None;
            resetCurrentIdleState();
        }

        if (!mPtr.getClass().isNpc() && mUpperBodyState > UpperBodyState::WeaponEquipped)
        {
            recovery = false;
            stats.setHitRecovery(false);
        }

        if (mHitState != CharState_None)
        {
            if (!mAnimation->isPlaying(mCurrentHit))
            {
                if (isKnockedOut() && mCurrentHit.empty() && knockout)
                    return;

                mHitState = CharState_None;
                mCurrentHit.clear();
                stats.setKnockedDown(false);
                stats.setHitRecovery(false);
                resetCurrentIdleState();
            }
            else if (isKnockedOut())
                mAnimation->setLoopingEnabled(mCurrentHit, knockout);
            return;
        }

        if (!knockout && !knockdown && !recovery && !block)
            return;

        MWRender::Animation::AnimPriority priority(Priority_Knockdown);
        std::string_view startKey = "start";
        std::string_view stopKey = "stop";
        if (knockout)
        {
            mHitState = isSwimming ? CharState_SwimKnockOut : CharState_KnockOut;
            stats.setKnockedDown(true);
        }
        else if (knockdown)
        {
            mHitState = isSwimming ? CharState_SwimKnockDown : CharState_KnockDown;
        }
        else if (recovery)
        {
            mHitState = isSwimming ? CharState_SwimHit : CharState_Hit;
            priority = Priority_Hit;
        }
        else if (block)
        {
            mHitState = CharState_Block;
            priority = Priority_Hit;
            priority[MWRender::BoneGroup_LeftArm] = Priority_Block;
            priority[MWRender::BoneGroup_LowerBody] = Priority_WeaponLowerBody;
            startKey = "block start";
            stopKey = "block stop";
        }

        mCurrentHit = hitStateToAnimGroup(mHitState);

        if (isRecovery())
        {
            mCurrentHit = chooseRandomGroup(mCurrentHit);
            if (mHitState == CharState_SwimHit && !mAnimation->hasAnimation(mCurrentHit))
                mCurrentHit = chooseRandomGroup(hitStateToAnimGroup(CharState_Hit));
        }

        // Cancel upper body animations
        if (isKnockedOut() || isKnockedDown())
        {
            if (!mCurrentWeapon.empty())
                mAnimation->disable(mCurrentWeapon);
            if (mUpperBodyState > UpperBodyState::WeaponEquipped)
            {
                mUpperBodyState = UpperBodyState::WeaponEquipped;
                if (mWeaponType > ESM::Weapon::None)
                    mAnimation->showWeapons(true);
            }
            else if (mUpperBodyState < UpperBodyState::WeaponEquipped)
            {
                mUpperBodyState = UpperBodyState::None;
            }
        }

        if (!mAnimation->hasAnimation(mCurrentHit))
        {
            mCurrentHit.clear();
            return;
        }

        playBlendedAnimation(mCurrentHit, priority, MWRender::BlendMask_All, true, 1, startKey, stopKey, 0.0f,
            std::numeric_limits<uint32_t>::max());
    }

    void CharacterController::refreshJumpAnims(JumpingState jump, bool force)
    {
        if (!force && jump == mJumpState)
            return;

        if (jump == JumpState_None)
        {
            if (!mCurrentJump.empty())
                resetCurrentIdleState();
            resetCurrentJumpState();
            return;
        }

        std::string_view weapShortGroup = getWeaponShortGroup(mWeaponType);
        std::string jumpAnimName = "jump";
        jumpAnimName += weapShortGroup;
        MWRender::Animation::BlendMask jumpmask = MWRender::BlendMask_All;
        if (!weapShortGroup.empty() && !mAnimation->hasAnimation(jumpAnimName))
            jumpAnimName = fallbackShortWeaponGroup("jump", &jumpmask);

        if (!mAnimation->hasAnimation(jumpAnimName))
        {
            if (!mCurrentJump.empty())
                resetCurrentIdleState();
            resetCurrentJumpState();
            return;
        }

        bool startAtLoop = (jump == mJumpState);
        mJumpState = jump;
        clearStateAnimation(mCurrentJump);

        mCurrentJump = jumpAnimName;
        if (mJumpState == JumpState_InAir)
            playBlendedAnimation(jumpAnimName, Priority_Jump, jumpmask, false, 1.0f,
                startAtLoop ? "loop start" : "start", "stop", 0.f, std::numeric_limits<uint32_t>::max());
        else if (mJumpState == JumpState_Landing)
            playBlendedAnimation(jumpAnimName, Priority_Jump, jumpmask, true, 1.0f, "loop stop", "stop", 0.0f, 0);
    }

    bool CharacterController::onOpen() const
    {
        if (mPtr.getType() == ESM::Container::sRecordId && mAnimation)
        {
            if (!mAnimation->hasAnimation("containeropen"))
                return true;

            if (mAnimation->isPlaying("containeropen"))
                return false;

            if (mAnimation->isPlaying("containerclose"))
                return false;

            mAnimation->play(
                "containeropen", Priority_Scripted, MWRender::BlendMask_All, false, 1.0f, "start", "stop", 0.f, 0);
            if (mAnimation->isPlaying("containeropen"))
                return false;
        }

        return true;
    }

    void CharacterController::onClose() const
    {
        if (mPtr.getType() == ESM::Container::sRecordId)
        {
            if (!mAnimation || !mAnimation->hasAnimation("containerclose"))
                return;

            float complete, startPoint = 0.f;
            bool animPlaying = mAnimation->getInfo("containeropen", &complete);
            if (animPlaying)
                startPoint = 1.f - complete;

            mAnimation->play("containerclose", Priority_Scripted, MWRender::BlendMask_All, false, 1.0f, "start", "stop",
                startPoint, 0);
        }
    }

    std::string_view CharacterController::getWeaponAnimation(int weaponType) const
    {
        std::string_view weaponGroup = getWeaponType(weaponType)->mLongGroup;
        if (isRealWeapon(weaponType) && !mAnimation->hasAnimation(weaponGroup))
        {
            static const std::string_view oneHandFallback = getWeaponType(ESM::Weapon::LongBladeOneHand)->mLongGroup;
            static const std::string_view twoHandFallback = getWeaponType(ESM::Weapon::LongBladeTwoHand)->mLongGroup;

            const ESM::WeaponType* weapInfo = getWeaponType(weaponType);

            // For real two-handed melee weapons use 2h swords animations as fallback, otherwise use the 1h ones
            if (weapInfo->mFlags & ESM::WeaponType::TwoHanded && weapInfo->mWeaponClass == ESM::WeaponType::Melee)
                weaponGroup = twoHandFallback;
            else
                weaponGroup = oneHandFallback;
        }
        else if (weaponType == ESM::Weapon::HandToHand && !mPtr.getClass().isBipedal(mPtr))
            return "attack1";

        return weaponGroup;
    }

    std::string_view CharacterController::getWeaponShortGroup(int weaponType) const
    {
        if (weaponType == ESM::Weapon::HandToHand && !mPtr.getClass().isBipedal(mPtr))
            return {};
        return getWeaponType(weaponType)->mShortGroup;
    }

    std::string CharacterController::fallbackShortWeaponGroup(
        const std::string& baseGroupName, MWRender::Animation::BlendMask* blendMask) const
    {
        if (!isRealWeapon(mWeaponType))
        {
            if (blendMask != nullptr)
                *blendMask = MWRender::BlendMask_LowerBody;

            return baseGroupName;
        }

        static const std::string_view oneHandFallback = getWeaponShortGroup(ESM::Weapon::LongBladeOneHand);
        static const std::string_view twoHandFallback = getWeaponShortGroup(ESM::Weapon::LongBladeTwoHand);

        std::string groupName = baseGroupName;
        const ESM::WeaponType* weapInfo = getWeaponType(mWeaponType);

        // For real two-handed melee weapons use 2h swords animations as fallback, otherwise use the 1h ones
        if (weapInfo->mFlags & ESM::WeaponType::TwoHanded && weapInfo->mWeaponClass == ESM::WeaponType::Melee)
            groupName += twoHandFallback;
        else
            groupName += oneHandFallback;

        // Special case for crossbows - we should apply 1h animations a fallback only for lower body
        if (mWeaponType == ESM::Weapon::MarksmanCrossbow && blendMask != nullptr)
            *blendMask = MWRender::BlendMask_LowerBody;

        if (!mAnimation->hasAnimation(groupName))
        {
            groupName = baseGroupName;
            if (blendMask != nullptr)
                *blendMask = MWRender::BlendMask_LowerBody;
        }

        return groupName;
    }

    void CharacterController::refreshMovementAnims(CharacterState movement, bool force)
    {
        if (movement == mMovementState && !force)
            return;

        std::string_view movementAnimGroup = movementStateToAnimGroup(movement);

        if (movementAnimGroup.empty())
        {
            if (!mCurrentMovement.empty())
                resetCurrentIdleState();
            resetCurrentMovementState();
            return;
        }
        std::string movementAnimName{ movementAnimGroup };

        mMovementState = movement;
        std::string::size_type swimpos = movementAnimName.find("swim");
        if (!mAnimation->hasAnimation(movementAnimName))
        {
            if (swimpos != std::string::npos)
            {
                movementAnimName.erase(swimpos, 4);
                swimpos = std::string::npos;
            }
        }

        MWRender::Animation::BlendMask movemask = MWRender::BlendMask_All;

        std::string_view weapShortGroup = getWeaponShortGroup(mWeaponType);

        // Non-biped creatures don't use spellcasting-specific movement animations.
        if (!isRealWeapon(mWeaponType) && !mPtr.getClass().isBipedal(mPtr))
            weapShortGroup = {};

        if (swimpos == std::string::npos && !weapShortGroup.empty())
        {
            std::string weapMovementAnimName;
            // Spellcasting stance turning is a special case
            if (mWeaponType == ESM::Weapon::Spell && isTurning())
            {
                weapMovementAnimName = weapShortGroup;
                weapMovementAnimName += movementAnimName;
            }
            else
            {
                weapMovementAnimName = movementAnimName;
                weapMovementAnimName += weapShortGroup;
            }

            if (!mAnimation->hasAnimation(weapMovementAnimName))
                weapMovementAnimName = fallbackShortWeaponGroup(movementAnimName, &movemask);

            movementAnimName = std::move(weapMovementAnimName);
        }

        if (!mAnimation->hasAnimation(movementAnimName))
        {
            std::string::size_type runpos = movementAnimName.find("run");
            if (runpos != std::string::npos)
                movementAnimName.replace(runpos, 3, "walk");

            if (!mAnimation->hasAnimation(movementAnimName))
            {
                if (!mCurrentMovement.empty())
                    resetCurrentIdleState();
                resetCurrentMovementState();
                return;
            }
        }

        // If we're playing the same animation, start it from the point it ended
        float startpoint = 0.f;
        if (!mCurrentMovement.empty() && movementAnimName == mCurrentMovement)
            mAnimation->getInfo(mCurrentMovement, &startpoint);

        mMovementAnimationHasMovement = true;

        clearStateAnimation(mCurrentMovement);
        mCurrentMovement = std::move(movementAnimName);

        // For non-flying creatures, MW uses the Walk animation to calculate the animation velocity
        // even if we are running. This must be replicated, otherwise the observed speed would differ drastically.
        mAdjustMovementAnimSpeed = true;
        if (mPtr.getClass().getType() == ESM::Creature::sRecordId
            && !(mPtr.get<ESM::Creature>()->mBase->mFlags & ESM::Creature::Flies))
        {
            CharacterState walkState = runStateToWalkState(mMovementState);
            std::string_view anim = movementStateToAnimGroup(walkState);

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
            mMovementAnimSpeed = mAnimation->getVelocity(mCurrentMovement);

            if (mMovementAnimSpeed <= 1.0f)
            {
                // The first person anims don't have any velocity to calculate a speed multiplier from.
                // We use the third person velocities instead.
                // FIXME: should be pulled from the actual animation, but it is not presently loaded.
                bool sneaking = mMovementState == CharState_SneakForward || mMovementState == CharState_SneakBack
                    || mMovementState == CharState_SneakLeft || mMovementState == CharState_SneakRight;
                mMovementAnimSpeed = (sneaking ? 33.5452f : (isRunning() ? 222.857f : 154.064f));
                mMovementAnimationHasMovement = false;
            }
        }

        playBlendedAnimation(mCurrentMovement, Priority_Movement, movemask, false, 1.f, "start", "stop", startpoint,
            std::numeric_limits<uint32_t>::max(), true);
    }

    void CharacterController::refreshIdleAnims(CharacterState idle, bool force)
    {
        // FIXME: if one of the below states is close to their last animation frame (i.e. will be disabled in the coming
        // update), the idle animation should be displayed
        if (((mUpperBodyState != UpperBodyState::None && mUpperBodyState != UpperBodyState::WeaponEquipped)
                || mMovementState != CharState_None || !mCurrentHit.empty())
            && !mPtr.getClass().isBipedal(mPtr))
        {
            resetCurrentIdleState();
            return;
        }

        if (!force && idle == mIdleState && (mAnimation->isPlaying(mCurrentIdle) || !mAnimQueue.empty()))
            return;

        mIdleState = idle;

        std::string idleGroup = idleStateToAnimGroup(mIdleState);
        if (idleGroup.empty())
        {
            resetCurrentIdleState();
            return;
        }

        MWRender::Animation::AnimPriority priority = getIdlePriority(mIdleState);
        size_t numLoops = std::numeric_limits<uint32_t>::max();

        // Only play "idleswim" or "idlesneak" if they exist. Otherwise, fallback to
        // "idle"+weapon or "idle".
        bool fallback = mIdleState != CharState_Idle && !mAnimation->hasAnimation(idleGroup);
        if (fallback)
        {
            priority = getIdlePriority(CharState_Idle);
            idleGroup = idleStateToAnimGroup(CharState_Idle);
        }

        if (fallback || mIdleState == CharState_Idle || mIdleState == CharState_SpecialIdle)
        {
            std::string_view weapShortGroup = getWeaponShortGroup(mWeaponType);
            if (!weapShortGroup.empty())
            {
                std::string weapIdleGroup = idleGroup;
                weapIdleGroup += weapShortGroup;
                if (!mAnimation->hasAnimation(weapIdleGroup))
                    weapIdleGroup = fallbackShortWeaponGroup(idleGroup);
                idleGroup = std::move(weapIdleGroup);

                // play until the Loop Stop key 2 to 5 times, then play until the Stop key
                // this replicates original engine behavior for the "Idle1h" 1st-person animation
                auto& prng = MWBase::Environment::get().getWorld()->getPrng();
                numLoops = 1 + Misc::Rng::rollDice(4, prng);
            }
        }

        if (!mAnimation->hasAnimation(idleGroup))
        {
            resetCurrentIdleState();
            return;
        }

        float startPoint = 0.f;
        // There is no need to restart anim if the new and old anims are the same.
        // Just update the number of loops.
        if (mCurrentIdle == idleGroup)
            mAnimation->getInfo(mCurrentIdle, &startPoint);

        clearStateAnimation(mCurrentIdle);
        mCurrentIdle = std::move(idleGroup);
        playBlendedAnimation(mCurrentIdle, priority, MWRender::BlendMask_All, false, 1.0f, "start", "stop", startPoint,
            static_cast<uint32_t>(numLoops), true);
    }

    void CharacterController::refreshCurrentAnims(
        CharacterState idle, CharacterState movement, JumpingState jump, bool force)
    {
        // If the current animation is scripted, do not touch it
        if (isScriptedAnimPlaying())
            return;

        refreshHitRecoilAnims();
        refreshJumpAnims(jump, force);
        refreshMovementAnims(movement, force);

        // idle handled last as it can depend on the other states
        refreshIdleAnims(idle, force);
    }

    void CharacterController::playDeath(float startpoint, CharacterState death)
    {
        mDeathState = death;
        mCurrentDeath = deathStateToAnimGroup(mDeathState);
        mPtr.getClass().getCreatureStats(mPtr).setDeathAnimation(
            static_cast<signed char>(mDeathState - CharState_Death1));

        // For dead actors, refreshCurrentAnims is no longer called, so we need to disable the movement state manually.
        // Note that these animations wouldn't actually be visible (due to the Death animation's priority being higher).
        // However, they could still trigger text keys, such as Hit events, or sounds.
        resetCurrentMovementState();
        resetCurrentWeaponState();
        resetCurrentHitState();
        resetCurrentIdleState();
        resetCurrentJumpState();

        playBlendedAnimation(
            mCurrentDeath, Priority_Death, MWRender::BlendMask_All, false, 1.0f, "start", "stop", startpoint, 0);
    }

    CharacterState CharacterController::chooseRandomDeathState() const
    {
        int selected = 0;
        chooseRandomGroup("death", &selected);
        return static_cast<CharacterState>(CharState_Death1 + (selected - 1));
    }

    void CharacterController::playRandomDeath(float startpoint)
    {
        if (mPtr == getPlayer())
        {
            // The first-person animations do not include death, so we need to
            // force-switch to third person before playing the death animation.
            MWBase::Environment::get().getWorld()->useDeathCamera();
        }

        mDeathState = hitStateToDeathState(mHitState);
        if (mDeathState == CharState_None && MWBase::Environment::get().getWorld()->isSwimming(mPtr))
            mDeathState = CharState_SwimDeath;

        if (mDeathState == CharState_None
            || (mAnimation && !mAnimation->hasAnimation(deathStateToAnimGroup(mDeathState))))
            mDeathState = chooseRandomDeathState();

        // Do not interrupt scripted animation by death
        if (!mAnimation || isScriptedAnimPlaying())
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

    CharacterController::CharacterController(const MWWorld::Ptr& ptr, MWRender::Animation& anim)
        : mPtr(ptr)
        , mAnimation(&anim)
    {
        mAnimation->setTextKeyListener(this);

        const MWWorld::Class& cls = mPtr.getClass();
        if (cls.isActor())
        {
            /* Accumulate along X/Y only for now, until we can figure out how we should
             * handle knockout and death which moves the character down. */
            mAnimation->setAccumulation(osg::Vec3f(1.0f, 1.0f, 0.0f));

            if (cls.hasInventoryStore(mPtr))
            {
                getActiveWeapon(mPtr, &mWeaponType);
                if (mWeaponType != ESM::Weapon::None)
                {
                    mUpperBodyState = UpperBodyState::WeaponEquipped;
                    mCurrentWeapon = getWeaponAnimation(mWeaponType);
                }

                if (mWeaponType != ESM::Weapon::None && mWeaponType != ESM::Weapon::Spell
                    && mWeaponType != ESM::Weapon::HandToHand)
                {
                    mAnimation->showWeapons(true);
                    // Note: controllers for ranged weapon should use time for beginning of animation to play shooting
                    // properly, for other weapons they should use absolute time. Some mods rely on this behaviour (to
                    // rotate throwing projectiles, for example)
                    ESM::WeaponType::Class weaponClass = getWeaponType(mWeaponType)->mWeaponClass;
                    bool useRelativeDuration = weaponClass == ESM::WeaponType::Ranged;
                    mAnimation->setWeaponGroup(mCurrentWeapon, useRelativeDuration);
                }

                mAnimation->showCarriedLeft(updateCarriedLeftVisible(mWeaponType));
            }

            if (!cls.getCreatureStats(mPtr).isDead())
            {
                mIdleState = CharState_Idle;
                if (cls.getCreatureStats(mPtr).getFallHeight() > 0)
                    mJumpState = JumpState_InAir;
            }
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

                    mFloatToSurface = cStats.getHealth().getBase() != 0;
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
        if (mDeathState == CharState_None && (!cls.isActor() || !cls.getCreatureStats(mPtr).isDead()))
            refreshCurrentAnims(mIdleState, mMovementState, mJumpState, true);

        mAnimation->runAnimation(0.f);

        unpersistAnimationState();
    }

    CharacterController::~CharacterController()
    {
        detachAnimation();
    }

    void CharacterController::detachAnimation()
    {
        if (mAnimation)
        {
            persistAnimationState();
            mAnimation->setTextKeyListener(nullptr);
            mAnimation = nullptr;
        }
    }

    void CharacterController::handleTextKey(
        std::string_view groupname, SceneUtil::TextKeyMap::ConstIterator key, const SceneUtil::TextKeyMap& map)
    {
        if (!mAnimation)
            return;
        std::string_view evt = key->second;

        MWBase::Environment::get().getLuaManager()->animationTextKey(mPtr, key->second);

        if (evt.substr(0, 7) == "sound: ")
        {
            MWBase::SoundManager* sndMgr = MWBase::Environment::get().getSoundManager();
            sndMgr->playSound3D(mPtr, ESM::RefId::stringRefId(evt.substr(7)), 1.0f, 1.0f);
            return;
        }

        auto& charClass = mPtr.getClass();
        if (evt.substr(0, 10) == "soundgen: ")
        {
            std::string_view soundgen = evt.substr(10);

            // The event can optionally contain volume and pitch modifiers
            float volume = 1.0f;
            float pitch = 1.0f;

            if (soundgen.find(' ') != std::string::npos)
            {
                std::vector<std::string_view> tokens;
                Misc::StringUtils::split(soundgen, tokens);
                soundgen = tokens[0];

                if (tokens.size() >= 2)
                {
                    volume = Misc::StringUtils::toNumeric<float>(tokens[1], volume);
                }

                if (tokens.size() >= 3)
                {
                    pitch = Misc::StringUtils::toNumeric<float>(tokens[2], pitch);
                }
            }

            const ESM::RefId sound = charClass.getSoundIdFromSndGen(mPtr, soundgen);
            if (!sound.empty())
            {
                MWBase::SoundManager* sndMgr = MWBase::Environment::get().getSoundManager();
                if (soundgen == "left" || soundgen == "right")
                {
                    sndMgr->playSound3D(
                        mPtr, sound, volume, pitch, MWSound::Type::Foot, MWSound::PlayMode::NoPlayerLocal);
                }
                else
                {
                    sndMgr->playSound3D(mPtr, sound, volume, pitch);
                }
            }
            return;
        }

        if (evt.substr(0, groupname.size()) != groupname || evt.substr(groupname.size(), 2) != ": ")
        {
            // Not ours, skip it
            return;
        }

        std::string_view action = evt.substr(groupname.size() + 2);
        if (action == "equip attach")
        {
            if (groupname == "shield")
                mAnimation->showCarriedLeft(true);
            else if (mUpperBodyState == UpperBodyState::Equipping)
                mAnimation->showWeapons(true);
        }
        else if (action == "unequip detach")
        {
            if (groupname == "shield")
                mAnimation->showCarriedLeft(false);
            else if (mUpperBodyState == UpperBodyState::Unequipping)
                mAnimation->showWeapons(false);
        }
        else if (action == "chop hit" || action == "slash hit" || action == "thrust hit" || action == "hit")
        {
            int attackType = -1;
            if (action == "hit")
            {
                if (groupname == "attack1" || groupname == "swimattack1")
                    attackType = ESM::Weapon::AT_Chop;
                else if (groupname == "attack2" || groupname == "swimattack2")
                    attackType = ESM::Weapon::AT_Slash;
                else if (groupname == "attack3" || groupname == "swimattack3")
                    attackType = ESM::Weapon::AT_Thrust;
            }
            else if (action == "chop hit")
                attackType = ESM::Weapon::AT_Chop;
            else if (action == "slash hit")
                attackType = ESM::Weapon::AT_Slash;
            else if (action == "thrust hit")
                attackType = ESM::Weapon::AT_Thrust;
            // We want to avoid hit keys that come out of nowhere (e.g. in the follow animation)
            // and processing multiple hit keys for a single attack
            if (mReadyToHit)
            {
                charClass.hit(mPtr, mAttackStrength, attackType, mAttackVictim, mAttackHitPos, mAttackSuccess);
                mReadyToHit = false;
            }
        }
        else if (isRandomAttackAnimation(groupname) && action == "start")
        {
            std::multimap<float, std::string>::const_iterator hitKey = key;

            // Not all animations have a hit key defined. If there is none, the hit happens with the start key.
            bool hasHitKey = false;
            while (hitKey != map.end())
            {
                if (hitKey->second.starts_with(groupname))
                {
                    std::string_view suffix = std::string_view(hitKey->second).substr(groupname.size());
                    if (suffix == ": hit")
                    {
                        hasHitKey = true;
                        break;
                    }
                    if (suffix == ": stop")
                        break;
                }
                ++hitKey;
            }
            if (!hasHitKey)
            {
                // State update doesn't expect the start key to be the hit key,
                // so we have to do this early.
                prepareHit();

                if (groupname == "attack1" || groupname == "swimattack1")
                    charClass.hit(
                        mPtr, mAttackStrength, ESM::Weapon::AT_Chop, mAttackVictim, mAttackHitPos, mAttackSuccess);
                else if (groupname == "attack2" || groupname == "swimattack2")
                    charClass.hit(
                        mPtr, mAttackStrength, ESM::Weapon::AT_Slash, mAttackVictim, mAttackHitPos, mAttackSuccess);
                else if (groupname == "attack3" || groupname == "swimattack3")
                    charClass.hit(
                        mPtr, mAttackStrength, ESM::Weapon::AT_Thrust, mAttackVictim, mAttackHitPos, mAttackSuccess);
            }
        }
        else if (action == "shoot attach")
            mAnimation->attachArrow();
        else if (action == "shoot release")
        {
            // See notes for melee release above
            if (mReadyToHit)
            {
                mAnimation->releaseArrow(mAttackStrength);
                mReadyToHit = false;
            }
        }
        else if (action == "shoot follow attach")
            mAnimation->attachArrow();
        // Make sure this key is actually for the RangeType we are casting. The flame atronach has
        // the same animation for all range types, so there are 3 "release" keys on the same time, one for each range
        // type.
        else if (groupname == "spellcast" && action == mAttackType + " release")
        {
            if (mCanCast)
                MWBase::Environment::get().getWorld()->castSpell(mPtr, mCastingScriptedSpell);
            mCastingScriptedSpell = false;
            mCanCast = false;
        }
        else if (groupname == "containeropen" && action == "loot")
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Container, mPtr);
    }

    void CharacterController::updatePtr(const MWWorld::Ptr& ptr)
    {
        mPtr = ptr;
    }

    void CharacterController::updateIdleStormState(bool inwater) const
    {
        if (!mAnimation->hasAnimation("idlestorm"))
            return;

        bool animPlaying = mAnimation->isPlaying("idlestorm");
        if (mUpperBodyState != UpperBodyState::None || inwater)
        {
            if (animPlaying)
                mAnimation->disable("idlestorm");
            return;
        }

        const auto world = MWBase::Environment::get().getWorld();
        if (world->isInStorm())
        {
            osg::Vec3f stormDirection = world->getStormDirection();
            osg::Vec3f characterDirection = mPtr.getRefData().getBaseNode()->getAttitude() * osg::Vec3f(0, 1, 0);
            stormDirection.normalize();
            characterDirection.normalize();
            if (stormDirection * characterDirection < -0.5f)
            {
                if (!animPlaying)
                {
                    int mask = MWRender::BlendMask_Torso | MWRender::BlendMask_RightArm;
                    playBlendedAnimation("idlestorm", Priority_Storm, mask, true, 1.0f, "start", "stop", 0.0f,
                        std::numeric_limits<uint32_t>::max(), true);
                }
                else
                {
                    mAnimation->setLoopingEnabled("idlestorm", true);
                }
                return;
            }
        }

        if (animPlaying)
        {
            mAnimation->setLoopingEnabled("idlestorm", false);
        }
    }

    bool CharacterController::updateCarriedLeftVisible(const int weaptype) const
    {
        // Shields/torches shouldn't be visible during any operation involving two hands
        // There seems to be no text keys for this purpose, except maybe for "[un]equip start/stop",
        // but they are also present in weapon drawing animation.
        return mAnimation->updateCarriedLeftVisible(weaptype);
    }

    float CharacterController::calculateWindUp() const
    {
        if (!mAnimation || mCurrentWeapon.empty() || mWeaponType == ESM::Weapon::PickProbe
            || isRandomAttackAnimation(mCurrentWeapon))
            return -1.f;

        float minAttackTime = mAnimation->getTextKeyTime(mCurrentWeapon + ": " + mAttackType + " min attack");
        float maxAttackTime = mAnimation->getTextKeyTime(mCurrentWeapon + ": " + mAttackType + " max attack");
        if (minAttackTime == -1.f || minAttackTime >= maxAttackTime)
            return -1.f;

        return std::clamp(
            (mAnimation->getCurrentTime(mCurrentWeapon) - minAttackTime) / (maxAttackTime - minAttackTime), 0.f, 1.f);
    }

    void CharacterController::prepareHit()
    {
        if (mReadyToHit)
            return;

        auto& prng = MWBase::Environment::get().getWorld()->getPrng();
        mAttackStrength = calculateWindUp();
        if (mAttackStrength == -1.f)
            mAttackStrength = std::min(1.f, 0.1f + Misc::Rng::rollClosedProbability(prng));
        ESM::WeaponType::Class weapclass = getWeaponType(mWeaponType)->mWeaponClass;
        if (weapclass != ESM::WeaponType::Ranged && weapclass != ESM::WeaponType::Thrown)
        {
            mAttackSuccess = mPtr.getClass().evaluateHit(mPtr, mAttackVictim, mAttackHitPos);
            if (!mAttackSuccess)
                mAttackStrength = 0.f;
            playSwishSound();
        }

        mReadyToHit = true;
    }

    bool CharacterController::updateWeaponState()
    {
        // If the current animation is scripted, we can't do anything here.
        if (isScriptedAnimPlaying())
            return false;

        const auto world = MWBase::Environment::get().getWorld();
        auto& prng = world->getPrng();
        MWBase::SoundManager* sndMgr = MWBase::Environment::get().getSoundManager();

        const MWWorld::Class& cls = mPtr.getClass();
        CreatureStats& stats = cls.getCreatureStats(mPtr);
        int weaptype = ESM::Weapon::None;
        if (stats.getDrawState() == DrawState::Weapon)
            weaptype = ESM::Weapon::HandToHand;
        else if (stats.getDrawState() == DrawState::Spell)
            weaptype = ESM::Weapon::Spell;

        const bool isWerewolf = cls.isNpc() && cls.getNpcStats(mPtr).isWerewolf();

        const ESM::RefId* downSoundId = nullptr;
        bool weaponChanged = false;
        bool ammunition = true;
        float weapSpeed = 1.f;
        if (cls.hasInventoryStore(mPtr))
        {
            MWWorld::InventoryStore& inv = cls.getInventoryStore(mPtr);
            MWWorld::ContainerStoreIterator weapon = getActiveWeapon(mPtr, &weaptype);
            if (stats.getDrawState() == DrawState::Spell)
                weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);

            MWWorld::Ptr newWeapon;
            if (weapon != inv.end())
            {
                newWeapon = *weapon;
                if (isRealWeapon(mWeaponType))
                    downSoundId = &newWeapon.getClass().getDownSoundId(newWeapon);
            }
            // weapon->HtH switch: weapon is empty already, so we need to take sound from previous weapon
            else if (!mWeapon.isEmpty() && weaptype == ESM::Weapon::HandToHand && mWeaponType != ESM::Weapon::Spell)
                downSoundId = &mWeapon.getClass().getDownSoundId(mWeapon);

            if (mWeapon != newWeapon)
            {
                mWeapon = newWeapon;
                weaponChanged = true;
            }

            if (stats.getDrawState() == DrawState::Weapon && !mWeapon.isEmpty()
                && mWeapon.getType() == ESM::Weapon::sRecordId)
            {
                weapSpeed = mWeapon.get<ESM::Weapon>()->mBase->mData.mSpeed;
                MWWorld::ConstContainerStoreIterator ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);
                int ammotype = getWeaponType(mWeapon.get<ESM::Weapon>()->mBase->mData.mType)->mAmmoType;
                if (ammotype != ESM::Weapon::None)
                    ammunition = ammo != inv.end() && ammo->get<ESM::Weapon>()->mBase->mData.mType == ammotype;
                // Cancel attack if we no longer have ammunition
                if (!ammunition)
                {
                    if (mUpperBodyState == UpperBodyState::AttackWindUp)
                    {
                        mAnimation->disable(mCurrentWeapon);
                        mUpperBodyState = UpperBodyState::WeaponEquipped;
                    }
                    setAttackingOrSpell(false);
                }
            }

            MWWorld::ConstContainerStoreIterator torch = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
            if (torch != inv.end() && torch->getType() == ESM::Light::sRecordId
                && updateCarriedLeftVisible(mWeaponType))
            {
                if (mAnimation->isPlaying("shield"))
                    mAnimation->disable("shield");

                playBlendedAnimation("torch", Priority_Torch, MWRender::BlendMask_LeftArm, false, 1.0f, "start", "stop",
                    0.0f, std::numeric_limits<uint32_t>::max(), true);
            }
            else if (mAnimation->isPlaying("torch"))
            {
                mAnimation->disable("torch");
            }
        }

        MWRender::Animation::AnimPriority priorityWeapon(Priority_Default);
        if (cls.isBipedal(mPtr))
        {
            // For bipeds, blend weapon animations with lower body animations with higher priority
            // For non-bipeds, movement takes priority
            priorityWeapon = Priority_Weapon;
            priorityWeapon[MWRender::BoneGroup_LowerBody] = Priority_WeaponLowerBody;
        }

        bool forcestateupdate = false;

        // We should not play equipping animation and sound during weapon->weapon transition
        const bool isStillWeapon = isRealWeapon(mWeaponType) && isRealWeapon(weaptype);

        // If the current weapon type was changed in the middle of attack (e.g. by Equip console command or when bound
        // spell expires), we should force actor to the "weapon equipped" state, interrupt attack and update animations.
        if (isStillWeapon && mWeaponType != weaptype && mUpperBodyState > UpperBodyState::WeaponEquipped)
        {
            forcestateupdate = true;
            if (!mCurrentWeapon.empty())
                mAnimation->disable(mCurrentWeapon);
            mUpperBodyState = UpperBodyState::WeaponEquipped;
            setAttackingOrSpell(false);
            mAnimation->showWeapons(true);
        }

        if (!isKnockedOut() && !isKnockedDown() && !isRecovery())
        {
            std::string weapgroup;
            if (((!isWerewolf && cls.isBipedal(mPtr)) || mWeaponType != ESM::Weapon::Spell) && weaptype != mWeaponType
                && mUpperBodyState <= UpperBodyState::AttackWindUp && mUpperBodyState != UpperBodyState::Unequipping
                && !isStillWeapon)
            {
                // We can not play un-equip animation if weapon changed since last update
                if (!weaponChanged)
                {
                    // Note: we do not disable the weapon unequipping animation automatically to avoid body desync
                    weapgroup = getWeaponAnimation(mWeaponType);
                    int unequipMask = MWRender::BlendMask_All;
                    mUpperBodyState = UpperBodyState::Unequipping;
                    bool useShieldAnims = mAnimation->useShieldAnimations();
                    if (useShieldAnims && mWeaponType != ESM::Weapon::HandToHand && mWeaponType != ESM::Weapon::Spell
                        && !(mWeaponType == ESM::Weapon::None && weaptype == ESM::Weapon::Spell))
                    {
                        unequipMask = unequipMask | ~MWRender::BlendMask_LeftArm;
                        mAnimation->disable("shield");
                        playBlendedAnimation("shield", Priority_Block, MWRender::BlendMask_LeftArm, true, 1.0f,
                            "unequip start", "unequip stop", 0.0f, 0);
                    }
                    else if (mWeaponType == ESM::Weapon::HandToHand)
                        mAnimation->showCarriedLeft(false);

                    mAnimation->disable(weapgroup);
                    playBlendedAnimation(
                        weapgroup, priorityWeapon, unequipMask, false, 1.0f, "unequip start", "unequip stop", 0.0f, 0);

                    mAnimation->detachArrow();

                    // If we do not have the "unequip detach" key, hide weapon manually.
                    if (mAnimation->getTextKeyTime(weapgroup + ": unequip detach") < 0)
                        mAnimation->showWeapons(false);
                }

                if (downSoundId && !downSoundId->empty())
                {
                    sndMgr->playSound3D(mPtr, *downSoundId, 1.0f, 1.0f);
                }
            }

            float complete;
            bool animPlaying = mAnimation->getInfo(mCurrentWeapon, &complete);
            if (!animPlaying || complete >= 1.0f)
            {
                // Weapon is changed, no current animation (e.g. unequipping or attack).
                // Start equipping animation now.
                if (weaptype != mWeaponType && mUpperBodyState <= UpperBodyState::WeaponEquipped)
                {
                    forcestateupdate = true;
                    bool useShieldAnims = mAnimation->useShieldAnimations();
                    if (!useShieldAnims)
                        mAnimation->showCarriedLeft(updateCarriedLeftVisible(weaptype));

                    weapgroup = getWeaponAnimation(weaptype);

                    // Note: controllers for ranged weapon should use time for beginning of animation to play shooting
                    // properly, for other weapons they should use absolute time. Some mods rely on this behaviour (to
                    // rotate throwing projectiles, for example)
                    ESM::WeaponType::Class weaponClass = getWeaponType(weaptype)->mWeaponClass;
                    bool useRelativeDuration = weaponClass == ESM::WeaponType::Ranged;
                    mAnimation->setWeaponGroup(weapgroup, useRelativeDuration);

                    if (!isStillWeapon)
                    {
                        if (animPlaying)
                            mAnimation->disable(mCurrentWeapon);
                        if (weaptype != ESM::Weapon::None)
                        {
                            mAnimation->showWeapons(false);
                            int equipMask = MWRender::BlendMask_All;
                            mUpperBodyState = UpperBodyState::Equipping;
                            if (useShieldAnims && weaptype != ESM::Weapon::Spell)
                            {
                                equipMask = equipMask | ~MWRender::BlendMask_LeftArm;
                                mAnimation->disable("shield");
                                playBlendedAnimation("shield", Priority_Block, MWRender::BlendMask_LeftArm, true, 1.0f,
                                    "equip start", "equip stop", 0.0f, 0);
                            }

                            if (weaptype != ESM::Weapon::Spell || cls.isBipedal(mPtr))
                            {
                                playBlendedAnimation(weapgroup, priorityWeapon, equipMask, true, 1.0f, "equip start",
                                    "equip stop", 0.0f, 0);
                            }

                            // If we do not have the "equip attach" key, show weapon manually.
                            if (weaptype != ESM::Weapon::Spell
                                && mAnimation->getTextKeyTime(weapgroup + ": equip attach") < 0)
                            {
                                mAnimation->showWeapons(true);
                            }

                            if (!mWeapon.isEmpty() && mWeaponType != ESM::Weapon::HandToHand && isRealWeapon(weaptype))
                            {
                                const ESM::RefId& upSoundId = mWeapon.getClass().getUpSoundId(mWeapon);
                                if (!upSoundId.empty())
                                    sndMgr->playSound3D(mPtr, upSoundId, 1.0f, 1.0f);
                            }
                        }
                    }

                    if (isWerewolf)
                    {
                        const MWWorld::ESMStore& store = world->getStore();
                        const ESM::Sound* sound = store.get<ESM::Sound>().searchRandom("WolfEquip", prng);
                        if (sound)
                        {
                            sndMgr->playSound3D(mPtr, sound->mId, 1.0f, 1.0f);
                        }
                    }

                    mWeaponType = weaptype;
                    mCurrentWeapon = weapgroup;
                }

                // Make sure that we disabled unequipping animation
                if (mUpperBodyState == UpperBodyState::Unequipping)
                {
                    resetCurrentWeaponState();
                    mWeaponType = ESM::Weapon::None;
                }
            }
        }

        if (isWerewolf)
        {
            const ESM::RefId wolfRun = ESM::RefId::stringRefId("WolfRun");
            if (isRunning() && !world->isSwimming(mPtr) && mWeaponType == ESM::Weapon::None)
            {
                if (!sndMgr->getSoundPlaying(mPtr, wolfRun))
                    sndMgr->playSound3D(mPtr, wolfRun, 1.0f, 1.0f, MWSound::Type::Sfx, MWSound::PlayMode::Loop);
            }
            else
                sndMgr->stopSound3D(mPtr, wolfRun);
        }

        float complete = 0.f;
        bool animPlaying = false;
        ESM::WeaponType::Class weapclass = getWeaponType(mWeaponType)->mWeaponClass;
        if (getAttackingOrSpell())
        {
            bool resetIdle = true;
            if (mUpperBodyState == UpperBodyState::WeaponEquipped
                && (mHitState == CharState_None || mHitState == CharState_Block))
            {
                mAttackStrength = -1.f;
                mReadyToHit = false;

                // Randomize attacks for non-bipedal creatures
                if (!cls.isBipedal(mPtr)
                    && (!mAnimation->hasAnimation(mCurrentWeapon) || isRandomAttackAnimation(mCurrentWeapon)))
                {
                    mCurrentWeapon = chooseRandomAttackAnimation();
                }

                if (mWeaponType == ESM::Weapon::Spell)
                {
                    // Unset casting flag, otherwise pressing the mouse button down would
                    // continue casting every frame if there is no animation
                    setAttackingOrSpell(false);
                    if (mPtr == getPlayer())
                    {
                        // For the player, set the spell we want to cast
                        // This has to be done at the start of the casting animation,
                        // *not* when selecting a spell in the GUI (otherwise you could change the spell mid-animation)
                        const ESM::RefId& selectedSpell
                            = MWBase::Environment::get().getWindowManager()->getSelectedSpell();
                        stats.getSpells().setSelectedSpell(selectedSpell);
                    }
                    ESM::RefId spellid = stats.getSpells().getSelectedSpell();
                    bool isMagicItem = false;

                    // Play hand VFX and allow castSpell use (assuming an animation is going to be played) if
                    // spellcasting is successful. Scripted spellcasting bypasses restrictions.
                    MWWorld::SpellCastState spellCastResult = MWWorld::SpellCastState::Success;
                    if (!mCastingScriptedSpell)
                        spellCastResult = world->startSpellCast(mPtr);
                    mCanCast = spellCastResult == MWWorld::SpellCastState::Success;

                    if (spellid.empty() && cls.hasInventoryStore(mPtr))
                    {
                        MWWorld::InventoryStore& inv = cls.getInventoryStore(mPtr);
                        if (inv.getSelectedEnchantItem() != inv.end())
                        {
                            const MWWorld::Ptr& enchantItem = *inv.getSelectedEnchantItem();
                            spellid = enchantItem.getClass().getEnchantment(enchantItem);
                            isMagicItem = true;
                        }
                    }

                    if (isMagicItem && !Settings::game().mUseMagicItemAnimations)
                    {
                        world->breakInvisibility(mPtr);
                        // Enchanted items by default do not use casting animations
                        world->castSpell(mPtr);
                        resetIdle = false;
                        // Spellcasting animation needs to "play" for at least one frame to reset the aiming factor
                        animPlaying = true;
                        mUpperBodyState = UpperBodyState::Casting;
                    }
                    // Play the spellcasting animation/VFX if the spellcasting was successful or failed due to
                    // insufficient magicka. Used up powers are exempt from this from some reason.
                    else if (!spellid.empty() && spellCastResult != MWWorld::SpellCastState::PowerAlreadyUsed)
                    {
                        world->breakInvisibility(mPtr);
                        MWMechanics::CastSpell cast(mPtr, {}, false, mCastingScriptedSpell);

                        const std::vector<ESM::IndexedENAMstruct>* effects{ nullptr };
                        const MWWorld::ESMStore& store = world->getStore();
                        if (isMagicItem)
                        {
                            const ESM::Enchantment* enchantment = store.get<ESM::Enchantment>().find(spellid);
                            effects = &enchantment->mEffects.mList;
                            cast.playSpellCastingEffects(enchantment);
                        }
                        else
                        {
                            const ESM::Spell* spell = store.get<ESM::Spell>().find(spellid);
                            effects = &spell->mEffects.mList;
                            cast.playSpellCastingEffects(spell);
                        }
                        if (!effects->empty())
                        {
                            if (mCanCast)
                            {
                                const ESM::MagicEffect* effect = store.get<ESM::MagicEffect>().find(
                                    effects->back().mData.mEffectID); // use last effect of list for color of VFX_Hands

                                const ESM::Static* castStatic
                                    = world->getStore().get<ESM::Static>().find(ESM::RefId::stringRefId("VFX_Hands"));

                                const VFS::Path::Normalized castStaticModel
                                    = Misc::ResourceHelpers::correctMeshPath(VFS::Path::Normalized(castStatic->mModel));

                                if (mAnimation->getNode("Bip01 L Hand"))
                                    mAnimation->addEffect(
                                        castStaticModel.value(), "", false, "Bip01 L Hand", effect->mParticle);

                                if (mAnimation->getNode("Bip01 R Hand"))
                                    mAnimation->addEffect(
                                        castStaticModel.value(), "", false, "Bip01 R Hand", effect->mParticle);
                            }
                            // first effect used for casting animation
                            const ESM::ENAMstruct& firstEffect = effects->front().mData;

                            std::string startKey;
                            std::string stopKey;
                            if (isRandomAttackAnimation(mCurrentWeapon))
                            {
                                startKey = "start";
                                stopKey = "stop";
                                if (mCanCast)
                                    world->castSpell(mPtr,
                                        mCastingScriptedSpell); // No "release" text key to use, so cast immediately
                                mCastingScriptedSpell = false;
                                mCanCast = false;
                            }
                            else
                            {
                                switch (firstEffect.mRange)
                                {
                                    case 0:
                                        mAttackType = "self";
                                        break;
                                    case 1:
                                        mAttackType = "touch";
                                        break;
                                    case 2:
                                        mAttackType = "target";
                                        break;
                                }

                                startKey = mAttackType + " start";
                                stopKey = mAttackType + " stop";
                            }
                            playBlendedAnimation(mCurrentWeapon, priorityWeapon, MWRender::BlendMask_All, false, 1,
                                startKey, stopKey, 0.0f, 0);
                            mUpperBodyState = UpperBodyState::Casting;
                        }
                    }
                    else
                    {
                        resetIdle = false;
                    }
                }
                else
                {
                    std::string startKey = "start";
                    std::string stopKey = "stop";

                    MWBase::LuaManager::ActorControls* actorControls
                        = MWBase::Environment::get().getLuaManager()->getActorControls(mPtr);
                    const bool aiInactive
                        = actorControls->mDisableAI || !MWBase::Environment::get().getMechanicsManager()->isAIActive();
                    if (mWeaponType != ESM::Weapon::PickProbe && !isRandomAttackAnimation(mCurrentWeapon))
                    {
                        if (weapclass == ESM::WeaponType::Ranged || weapclass == ESM::WeaponType::Thrown)
                            mAttackType = "shoot";
                        else if (mPtr == getPlayer())
                        {
                            if (Settings::game().mBestAttack)
                            {
                                if (!mWeapon.isEmpty() && mWeapon.getType() == ESM::Weapon::sRecordId)
                                {
                                    mAttackType = getBestAttack(mWeapon.get<ESM::Weapon>()->mBase);
                                }
                                else
                                {
                                    // There is no "best attack" for Hand-to-Hand
                                    mAttackType = getRandomAttackType();
                                }
                            }
                            else
                            {
                                mAttackType = getMovementBasedAttackType();
                            }
                        }
                        else if (aiInactive)
                        {
                            mAttackType = getDesiredAttackType();
                            if (mAttackType == "")
                                mAttackType = getRandomAttackType();
                        }

                        // else if (mPtr != getPlayer()) use mAttackType set by AiCombat
                        startKey = mAttackType + ' ' + startKey;
                        stopKey = mAttackType + " max attack";
                    }

                    mUpperBodyState = UpperBodyState::AttackWindUp;

                    // Reset the attack results when the attack starts.
                    // Strictly speaking this should probably be done when the attack ends,
                    // but the attack animation might be cancelled in a myriad different ways.
                    mAttackSuccess = false;
                    mAttackVictim = MWWorld::Ptr();
                    mAttackHitPos = osg::Vec3f();

                    playBlendedAnimation(mCurrentWeapon, priorityWeapon, MWRender::BlendMask_All, false, weapSpeed,
                        startKey, stopKey, 0.0f, 0);
                }
            }

            // We should not break swim and sneak animations
            if (resetIdle && mIdleState != CharState_IdleSneak && mIdleState != CharState_IdleSwim)
            {
                resetCurrentIdleState();
            }
        }

        // Random attack and pick/probe animations never have wind up and are played to their end.
        // Other animations must be released when the attack state is unset.
        if (mUpperBodyState == UpperBodyState::AttackWindUp
            && (mWeaponType == ESM::Weapon::PickProbe || isRandomAttackAnimation(mCurrentWeapon)
                || !getAttackingOrSpell()))
        {
            mUpperBodyState = UpperBodyState::AttackRelease;
            world->breakInvisibility(mPtr);
            if (mWeaponType == ESM::Weapon::PickProbe)
            {
                // TODO: this will only work for the player, and needs to be fixed if NPCs should ever use
                // lockpicks/probes.
                MWWorld::Ptr target = world->getFocusObject();

                if (!target.isEmpty())
                {
                    std::string_view resultMessage, resultSound;
                    if (mWeapon.getType() == ESM::Lockpick::sRecordId)
                        Security(mPtr).pickLock(target, mWeapon, resultMessage, resultSound);
                    else if (mWeapon.getType() == ESM::Probe::sRecordId)
                        Security(mPtr).probeTrap(target, mWeapon, resultMessage, resultSound);
                    if (!resultMessage.empty())
                        MWBase::Environment::get().getWindowManager()->messageBox(resultMessage);
                    if (!resultSound.empty())
                        sndMgr->playSound3D(target, ESM::RefId::stringRefId(resultSound), 1.0f, 1.0f);
                }
            }
            // Evaluate the attack results and play the swish sound.
            // Attack animations with no hit key do this earlier.
            else
            {
                prepareHit();
            }

            if (mWeaponType == ESM::Weapon::PickProbe || isRandomAttackAnimation(mCurrentWeapon))
                mUpperBodyState = UpperBodyState::AttackEnd;
        }

        if (mUpperBodyState == UpperBodyState::AttackRelease)
        {
            // The release state might have been reached before reaching the wind-up section. We'll play the new section
            // only when the wind-up section is reached.
            float currentTime = mAnimation->getCurrentTime(mCurrentWeapon);
            float minAttackTime = mAnimation->getTextKeyTime(mCurrentWeapon + ": " + mAttackType + " min attack");
            float maxAttackTime = mAnimation->getTextKeyTime(mCurrentWeapon + ": " + mAttackType + " max attack");
            if (minAttackTime <= currentTime && currentTime <= maxAttackTime)
            {
                std::string hit = mAttackType != "shoot" ? "hit" : "release";

                float startPoint = 0.f;

                // Skip a bit of the pre-hit section based on the attack strength
                if (minAttackTime != -1.f && minAttackTime < maxAttackTime)
                {
                    startPoint = 1.f - mAttackStrength;
                    float minHitTime = mAnimation->getTextKeyTime(mCurrentWeapon + ": " + mAttackType + " min hit");
                    float hitTime = mAnimation->getTextKeyTime(mCurrentWeapon + ": " + mAttackType + ' ' + hit);
                    if (maxAttackTime <= minHitTime && minHitTime < hitTime)
                        startPoint *= (minHitTime - maxAttackTime) / (hitTime - maxAttackTime);
                }

                mAnimation->disable(mCurrentWeapon);
                playBlendedAnimation(mCurrentWeapon, priorityWeapon, MWRender::BlendMask_All, false, weapSpeed,
                    mAttackType + " max attack", mAttackType + ' ' + hit, startPoint, 0);
            }

            animPlaying = mAnimation->getInfo(mCurrentWeapon, &complete);

            // Try playing the "follow" section if the attack animation ended naturally or didn't play at all.
            if (!animPlaying || (currentTime >= maxAttackTime && complete >= 1.f))
            {
                std::string start = "follow start";
                std::string stop = "follow stop";

                if (mAttackType != "shoot")
                {
                    std::string strength = mAttackStrength < 0.33f ? "small"
                        : mAttackStrength < 0.66f                  ? "medium"
                                                                   : "large";
                    start = strength + ' ' + start;
                    stop = strength + ' ' + stop;
                }

                mReadyToHit = false;

                if (animPlaying)
                    mAnimation->disable(mCurrentWeapon);
                playBlendedAnimation(mCurrentWeapon, priorityWeapon, MWRender::BlendMask_All, false, weapSpeed,
                    mAttackType + ' ' + start, mAttackType + ' ' + stop, 0.0f, 0);
                mUpperBodyState = UpperBodyState::AttackEnd;

                animPlaying = mAnimation->getInfo(mCurrentWeapon, &complete);
            }
        }

        if (!animPlaying)
            animPlaying = mAnimation->getInfo(mCurrentWeapon, &complete);

        if (!animPlaying || complete >= 1.f)
        {
            if (mUpperBodyState == UpperBodyState::Equipping || mUpperBodyState == UpperBodyState::AttackEnd
                || mUpperBodyState == UpperBodyState::Casting)
            {
                if (ammunition && mWeaponType == ESM::Weapon::MarksmanCrossbow)
                    mAnimation->attachArrow();

                // Cancel stagger animation at the end of an attack to avoid abrupt transitions
                // in favor of a different abrupt transition, like Morrowind
                if (mUpperBodyState != UpperBodyState::Equipping && isRecovery())
                    mAnimation->disable(mCurrentHit);

                if (animPlaying)
                    mAnimation->disable(mCurrentWeapon);

                mUpperBodyState = UpperBodyState::WeaponEquipped;
            }
            else if (mUpperBodyState == UpperBodyState::Unequipping)
            {
                if (animPlaying)
                    mAnimation->disable(mCurrentWeapon);
                mUpperBodyState = UpperBodyState::None;
            }
        }

        mAnimation->setPitchFactor(0.f);
        if (mUpperBodyState > UpperBodyState::WeaponEquipped
            && (weapclass == ESM::WeaponType::Ranged || weapclass == ESM::WeaponType::Thrown))
        {
            mAnimation->setPitchFactor(1.f);

            // A smooth transition can be provided if a pre-wind-up section is defined. Random attack animations never
            // have one.
            if (mUpperBodyState == UpperBodyState::AttackWindUp && !isRandomAttackAnimation(mCurrentWeapon))
            {
                float currentTime = mAnimation->getCurrentTime(mCurrentWeapon);
                float minAttackTime = mAnimation->getTextKeyTime(mCurrentWeapon + ": " + mAttackType + " min attack");
                float startTime = mAnimation->getTextKeyTime(mCurrentWeapon + ": " + mAttackType + " start");
                if (startTime <= currentTime && currentTime < minAttackTime)
                    mAnimation->setPitchFactor((currentTime - startTime) / (minAttackTime - startTime));
            }
            else if (mUpperBodyState == UpperBodyState::AttackEnd)
            {
                // technically we do not need a pitch for crossbow reload animation,
                // but we should avoid abrupt repositioning
                if (mWeaponType == ESM::Weapon::MarksmanCrossbow)
                    mAnimation->setPitchFactor(std::max(0.f, 1.f - complete * 10.f));
                else
                    mAnimation->setPitchFactor(1.f - complete);
            }
        }

        mAnimation->setAccurateAiming(mUpperBodyState > UpperBodyState::WeaponEquipped);

        return forcestateupdate;
    }

    void CharacterController::updateAnimQueue()
    {
        if (mAnimQueue.empty())
            return;

        if (!mAnimation->isPlaying(mAnimQueue.front().mGroup))
        {
            // Playing animations through mwscript is weird. If an animation is
            // a looping animation (idle or other cyclical animations), then they
            // will end as expected. However, if they are non-looping animations, they
            // will stick around forever or until another animation appears in the queue.
            bool shouldPlayOrRestart = mAnimQueue.size() > 1;
            if (shouldPlayOrRestart || !mAnimQueue.front().mScripted
                || (mAnimQueue.front().mLoopCount == 0 && mAnimQueue.front().mLooping))
            {
                mAnimation->setPlayScriptedOnly(false);
                mAnimation->disable(mAnimQueue.front().mGroup);
                mAnimQueue.pop_front();
                shouldPlayOrRestart = true;
            }
            else
                // A non-looping animation will stick around forever, so only restart if the animation
                // actually was removed for some reason.
                shouldPlayOrRestart = !mAnimation->getInfo(mAnimQueue.front().mGroup)
                    && mAnimation->hasAnimation(mAnimQueue.front().mGroup);

            if (shouldPlayOrRestart)
            {
                // Move on to the remaining items of the queue
                playAnimQueue();
            }
        }
        else
        {
            float complete;
            uint32_t loopcount;
            mAnimation->getInfo(mAnimQueue.front().mGroup, &complete, nullptr, &loopcount);
            mAnimQueue.front().mLoopCount = loopcount;
            mAnimQueue.front().mTime = complete;
        }

        if (!mAnimQueue.empty())
            mAnimation->setLoopingEnabled(mAnimQueue.front().mGroup, mAnimQueue.size() <= 1);
    }

    void CharacterController::playAnimQueue(bool loopStart)
    {
        if (!mAnimQueue.empty())
        {
            clearStateAnimation(mCurrentIdle);
            mIdleState = CharState_SpecialIdle;
            auto priority = mAnimQueue.front().mScripted ? Priority_Scripted : Priority_Default;
            mAnimation->setPlayScriptedOnly(mAnimQueue.front().mScripted);
            if (mAnimQueue.front().mScripted)
                mAnimation->play(mAnimQueue.front().mGroup, priority, MWRender::BlendMask_All, false,
                    mAnimQueue.front().mSpeed, (loopStart ? "loop start" : mAnimQueue.front().mStartKey),
                    mAnimQueue.front().mStopKey, mAnimQueue.front().mTime, mAnimQueue.front().mLoopCount,
                    mAnimQueue.front().mLooping);
            else
                playBlendedAnimation(mAnimQueue.front().mGroup, priority, MWRender::BlendMask_All, false,
                    mAnimQueue.front().mSpeed, (loopStart ? "loop start" : mAnimQueue.front().mStartKey),
                    mAnimQueue.front().mStopKey, mAnimQueue.front().mTime, mAnimQueue.front().mLoopCount,
                    mAnimQueue.front().mLooping);
        }
    }

    void CharacterController::update(float duration)
    {
        if (!mAnimation)
            return;
        MWBase::World* world = MWBase::Environment::get().getWorld();
        MWBase::SoundManager* sndMgr = MWBase::Environment::get().getSoundManager();
        const MWWorld::Class& cls = mPtr.getClass();
        osg::Vec3f movement(0.f, 0.f, 0.f);
        float speed = 0.f;

        updateMagicEffects();

        bool isPlayer = mPtr == MWMechanics::getPlayer();
        bool isFirstPersonPlayer = isPlayer && MWBase::Environment::get().getWorld()->isFirstPerson();
        bool godmode = isPlayer && MWBase::Environment::get().getWorld()->getGodModeState();

        float scale = mPtr.getCellRef().getScale();

        if (!Settings::game().mNormaliseRaceSpeed && cls.isNpc())
        {
            const ESM::NPC* npc = mPtr.get<ESM::NPC>()->mBase;
            const ESM::Race* race = world->getStore().get<ESM::Race>().find(npc->mRace);
            float weight = npc->isMale() ? race->mData.mMaleWeight : race->mData.mFemaleWeight;
            scale *= weight;
        }

        if (cls.isActor() && cls.getCreatureStats(mPtr).wasTeleported())
        {
            mSmoothedSpeed = osg::Vec2f();
            cls.getCreatureStats(mPtr).setTeleported(false);
        }

        if (!cls.isActor())
            updateAnimQueue();
        else if (!cls.getCreatureStats(mPtr).isDead())
        {
            bool onground = world->isOnGround(mPtr);
            bool inwater = world->isSwimming(mPtr);
            bool flying = world->isFlying(mPtr);
            bool solid = world->isActorCollisionEnabled(mPtr);
            // Can't run and sneak while flying (see speed formula in Npc/Creature::getSpeed)
            bool sneak
                = cls.getCreatureStats(mPtr).getStance(MWMechanics::CreatureStats::Stance_Sneak) && !flying && !inwater;
            bool isrunning = cls.getCreatureStats(mPtr).getStance(MWMechanics::CreatureStats::Stance_Run) && !flying;
            CreatureStats& stats = cls.getCreatureStats(mPtr);
            Movement& movementSettings = cls.getMovementSettings(mPtr);

            // Force Jump Logic

            bool isMoving
                = (std::abs(movementSettings.mPosition[0]) > .5 || std::abs(movementSettings.mPosition[1]) > .5);
            if (!inwater && !flying)
            {
                // Force Jump
                if (stats.getMovementFlag(MWMechanics::CreatureStats::Flag_ForceJump))
                    movementSettings.mPosition[2] = onground ? 1.f : 0.f;
                // Force Move Jump, only jump if they're otherwise moving
                if (stats.getMovementFlag(MWMechanics::CreatureStats::Flag_ForceMoveJump) && isMoving)
                    movementSettings.mPosition[2] = onground ? 1.f : 0.f;
            }

            osg::Vec3f rot = cls.getRotationVector(mPtr);
            osg::Vec3f vec(movementSettings.asVec3());
            movementSettings.mSpeedFactor = std::min(vec.length(), 1.f);
            vec.normalize();

            const bool smoothMovement = Settings::game().mSmoothMovement;
            if (smoothMovement)
            {
                float angle = mPtr.getRefData().getPosition().rot[2];
                osg::Vec2f targetSpeed
                    = Misc::rotateVec2f(osg::Vec2f(vec.x(), vec.y()), -angle) * movementSettings.mSpeedFactor;
                osg::Vec2f delta = targetSpeed - mSmoothedSpeed;
                float speedDelta = movementSettings.mSpeedFactor - mSmoothedSpeed.length();
                float deltaLen = delta.length();

                float maxDelta;
                if (isFirstPersonPlayer)
                    maxDelta = 1;
                else if (std::abs(speedDelta) < deltaLen / 2)
                    // Turning is smooth for player and less smooth for NPCs (otherwise NPC can miss a path point).
                    maxDelta = duration * (isPlayer ? 1.0f / Settings::game().mSmoothMovementPlayerTurningDelay : 6.f);
                else if (isPlayer && speedDelta < -deltaLen / 2)
                    // As soon as controls are released, mwinput switches player from running to walking.
                    // So stopping should be instant for player, otherwise it causes a small twitch.
                    maxDelta = 1;
                else // In all other cases speeding up and stopping are smooth.
                    maxDelta = duration * 3.f;

                if (deltaLen > maxDelta)
                    delta *= maxDelta / deltaLen;
                mSmoothedSpeed += delta;

                osg::Vec2f newSpeed = Misc::rotateVec2f(mSmoothedSpeed, angle);
                movementSettings.mSpeedFactor = newSpeed.normalize();
                vec.x() = newSpeed.x();
                vec.y() = newSpeed.y();

                const float eps = 0.001f;
                if (movementSettings.mSpeedFactor < eps)
                {
                    movementSettings.mSpeedFactor = 0;
                    vec.x() = 0;
                    vec.y() = 1;
                }
                else if ((vec.y() < 0) != mIsMovingBackward)
                {
                    if (targetSpeed.length() < eps || (movementSettings.mPosition[1] < 0) == mIsMovingBackward)
                        vec.y() = mIsMovingBackward ? -eps : eps;
                }
                vec.normalize();
            }

            float effectiveRotation = rot.z();
            bool canMove = cls.getMaxSpeed(mPtr) > 0;
            const bool turnToMovementDirection = Settings::game().mTurnToMovementDirection;
            const bool isBiped = mPtr.getClass().isBipedal(mPtr);
            if (!isBiped || !turnToMovementDirection || isFirstPersonPlayer)
            {
                movementSettings.mIsStrafing = std::abs(vec.x()) > std::abs(vec.y()) * 2;
                stats.setSideMovementAngle(0);
            }
            else if (canMove)
            {
                float targetMovementAngle
                    = vec.y() >= 0 ? std::atan2(-vec.x(), vec.y()) : std::atan2(vec.x(), -vec.y());
                movementSettings.mIsStrafing = (stats.getDrawState() != MWMechanics::DrawState::Nothing || inwater)
                    && std::abs(targetMovementAngle) > osg::DegreesToRadians(60.0f);
                if (movementSettings.mIsStrafing)
                    targetMovementAngle = 0;
                float delta = targetMovementAngle - stats.getSideMovementAngle();
                float cosDelta = cosf(delta);

                if ((vec.y() < 0) == mIsMovingBackward)
                    movementSettings.mSpeedFactor
                        *= std::min(std::max(cosDelta, 0.f) + 0.3f, 1.f); // slow down when turn
                if (std::abs(delta) < osg::DegreesToRadians(20.0f))
                    mIsMovingBackward = vec.y() < 0;

                float maxDelta = osg::PIf * duration * (2.5f - cosDelta);
                delta = std::clamp(delta, -maxDelta, maxDelta);
                stats.setSideMovementAngle(stats.getSideMovementAngle() + delta);
                effectiveRotation += delta;
            }

            mAnimation->setLegsYawRadians(stats.getSideMovementAngle());
            if (stats.getDrawState() == MWMechanics::DrawState::Nothing || inwater)
                mAnimation->setUpperBodyYawRadians(stats.getSideMovementAngle() / 2);
            else
                mAnimation->setUpperBodyYawRadians(stats.getSideMovementAngle() / 4);
            if (smoothMovement && !isPlayer && !inwater)
                mAnimation->setUpperBodyYawRadians(mAnimation->getUpperBodyYawRadians() + mAnimation->getHeadYaw() / 2);

            speed = cls.getCurrentSpeed(mPtr);
            vec.x() *= speed;
            vec.y() *= speed;

            if (isKnockedOut() || isKnockedDown() || isRecovery() || isScriptedAnimPlaying())
                vec = osg::Vec3f();

            CharacterState movestate = CharState_None;
            CharacterState idlestate = CharState_None;
            JumpingState jumpstate = JumpState_None;

            const MWWorld::Store<ESM::GameSetting>& gmst = world->getStore().get<ESM::GameSetting>();
            if (vec.x() != 0.f || vec.y() != 0.f)
            {
                // advance athletics
                if (isPlayer)
                {
                    if (inwater)
                    {
                        mSecondsOfSwimming += duration;
                        while (mSecondsOfSwimming > 1)
                        {
                            cls.skillUsageSucceeded(mPtr, ESM::Skill::Athletics, ESM::Skill::Athletics_SwimOneSecond);
                            mSecondsOfSwimming -= 1;
                        }
                    }
                    else if (isrunning && !sneak)
                    {
                        mSecondsOfRunning += duration;
                        while (mSecondsOfRunning > 1)
                        {
                            cls.skillUsageSucceeded(mPtr, ESM::Skill::Athletics, ESM::Skill::Athletics_RunOneSecond);
                            mSecondsOfRunning -= 1;
                        }
                    }
                }

                if (!godmode)
                {
                    // reduce fatigue
                    float fatigueLoss = 0.f;
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
                    fatigueLoss *= movementSettings.mSpeedFactor;
                    DynamicStat<float> fatigue = cls.getCreatureStats(mPtr).getFatigue();
                    fatigue.setCurrent(fatigue.getCurrent() - fatigueLoss, fatigue.getCurrent() < 0);
                    cls.getCreatureStats(mPtr).setFatigue(fatigue);
                }
            }

            bool wasInJump = mInJump;
            mInJump = false;
            const float jumpHeight = cls.getJump(mPtr);
            if (jumpHeight <= 0.f || sneak || inwater || flying || !solid)
            {
                vec.z() = 0.f;
                // Following code might assign some vertical movement regardless, need to reset this manually
                // This is used for jumping detection
                movementSettings.mPosition[2] = 0;
            }

            if (!inwater && !flying && solid)
            {
                // In the air (either getting up —ascending part of jump— or falling).
                if (!onground)
                {
                    mInJump = true;
                    jumpstate = JumpState_InAir;

                    static const float fJumpMoveBase = gmst.find("fJumpMoveBase")->mValue.getFloat();
                    static const float fJumpMoveMult = gmst.find("fJumpMoveMult")->mValue.getFloat();
                    float factor = fJumpMoveBase
                        + fJumpMoveMult * mPtr.getClass().getSkill(mPtr, ESM::Skill::Acrobatics) / 100.f;
                    factor = std::min(1.f, factor);
                    vec.x() *= factor;
                    vec.y() *= factor;
                    vec.z() = 0.0f;
                }
                // Started a jump.
                else if (mJumpState != JumpState_InAir && vec.z() > 0.f)
                {
                    mInJump = true;
                    if (vec.x() == 0 && vec.y() == 0)
                        vec.z() = jumpHeight;
                    else
                    {
                        osg::Vec3f lat(vec.x(), vec.y(), 0.0f);
                        lat.normalize();
                        vec = osg::Vec3f(lat.x(), lat.y(), 1.0f) * jumpHeight * 0.707f;
                    }
                }
            }

            if (!mInJump)
            {
                if (mJumpState == JumpState_InAir && !flying && solid && wasInJump)
                {
                    float height = cls.getCreatureStats(mPtr).land(isPlayer);
                    float healthLost = 0.f;
                    if (!inwater)
                        healthLost = getFallDamage(mPtr, height);

                    if (healthLost > 0.0f)
                    {
                        const float fatigueTerm = cls.getCreatureStats(mPtr).getFatigueTerm();

                        // inflict fall damages
                        if (!godmode)
                        {
                            DynamicStat<float> health = cls.getCreatureStats(mPtr).getHealth();
                            float realHealthLost = healthLost * (1.0f - 0.25f * fatigueTerm);
                            health.setCurrent(health.getCurrent() - realHealthLost);
                            cls.getCreatureStats(mPtr).setHealth(health);
                            sndMgr->playSound3D(mPtr, ESM::RefId::stringRefId("Health Damage"), 1.0f, 1.0f);
                            if (isPlayer)
                                MWBase::Environment::get().getWindowManager()->activateHitOverlay();
                        }

                        const float acrobaticsSkill = cls.getSkill(mPtr, ESM::Skill::Acrobatics);
                        if (healthLost > (acrobaticsSkill * fatigueTerm))
                        {
                            if (!godmode)
                                cls.getCreatureStats(mPtr).setKnockedDown(true);
                        }
                        else
                        {
                            // report acrobatics progression
                            if (isPlayer)
                                cls.skillUsageSucceeded(mPtr, ESM::Skill::Acrobatics, ESM::Skill::Acrobatics_Fall);
                        }
                    }

                    if (mPtr.getClass().isNpc())
                    {
                        std::string_view sound;
                        osg::Vec3f pos(mPtr.getRefData().getPosition().asVec3());
                        if (world->isUnderwater(mPtr.getCell(), pos) || world->isWalkingOnWater(mPtr))
                            sound = "DefaultLandWater";
                        else if (onground)
                            sound = "DefaultLand";

                        if (!sound.empty())
                            sndMgr->playSound3D(mPtr, ESM::RefId::stringRefId(sound), 1.f, 1.f, MWSound::Type::Foot,
                                MWSound::PlayMode::NoPlayerLocal);
                    }
                }

                if (mAnimation->isPlaying(mCurrentJump))
                    jumpstate = JumpState_Landing;

                vec.z() = 0.0f;

                if (movementSettings.mIsStrafing)
                {
                    if (vec.x() > 0.0f)
                        movestate = (inwater ? (isrunning ? CharState_SwimRunRight : CharState_SwimWalkRight)
                                             : (sneak ? CharState_SneakRight
                                                      : (isrunning ? CharState_RunRight : CharState_WalkRight)));
                    else if (vec.x() < 0.0f)
                        movestate = (inwater
                                ? (isrunning ? CharState_SwimRunLeft : CharState_SwimWalkLeft)
                                : (sneak ? CharState_SneakLeft : (isrunning ? CharState_RunLeft : CharState_WalkLeft)));
                }
                else if (vec.length2() > 0.0f)
                {
                    if (vec.y() >= 0.0f)
                        movestate = (inwater ? (isrunning ? CharState_SwimRunForward : CharState_SwimWalkForward)
                                             : (sneak ? CharState_SneakForward
                                                      : (isrunning ? CharState_RunForward : CharState_WalkForward)));
                    else
                        movestate = (inwater
                                ? (isrunning ? CharState_SwimRunBack : CharState_SwimWalkBack)
                                : (sneak ? CharState_SneakBack : (isrunning ? CharState_RunBack : CharState_WalkBack)));
                }
                else
                {
                    // It seems only bipedal actors use turning animations.
                    // Also do not use turning animations in the first-person view and when sneaking.
                    if (!sneak && !isFirstPersonPlayer && isBiped)
                    {
                        if (effectiveRotation > 0.f)
                            movestate = inwater ? CharState_SwimTurnRight : CharState_TurnRight;
                        else if (effectiveRotation < 0.f)
                            movestate = inwater ? CharState_SwimTurnLeft : CharState_TurnLeft;
                    }
                }
            }

            if (turnToMovementDirection && !isFirstPersonPlayer && isBiped
                && (movestate == CharState_SwimRunForward || movestate == CharState_SwimWalkForward
                    || movestate == CharState_SwimRunBack || movestate == CharState_SwimWalkBack))
            {
                float swimmingPitch = mAnimation->getBodyPitchRadians();
                float targetSwimmingPitch = -mPtr.getRefData().getPosition().rot[0];
                float maxSwimPitchDelta = 3.0f * duration;
                swimmingPitch += std::clamp(targetSwimmingPitch - swimmingPitch, -maxSwimPitchDelta, maxSwimPitchDelta);
                mAnimation->setBodyPitchRadians(swimmingPitch);
            }
            else
                mAnimation->setBodyPitchRadians(0);

            if (inwater && isPlayer && !isFirstPersonPlayer && Settings::game().mSwimUpwardCorrection)
            {
                const float swimUpwardCoef = Settings::game().mSwimUpwardCoef;
                vec.z() = std::abs(vec.y()) * swimUpwardCoef;
                vec.y() *= std::sqrt(1.0f - swimUpwardCoef * swimUpwardCoef);
            }

            if (isBiped)
            {
                if (mTurnAnimationThreshold > 0)
                    mTurnAnimationThreshold -= duration;

                if (movestate == CharState_TurnRight || movestate == CharState_TurnLeft
                    || movestate == CharState_SwimTurnRight || movestate == CharState_SwimTurnLeft)
                {
                    mTurnAnimationThreshold = 0.05f;
                }
                else if (movestate == CharState_None && isTurning() && mTurnAnimationThreshold > 0)
                {
                    movestate = mMovementState;
                }
            }

            if (movestate != CharState_None)
            {
                clearAnimQueue();
                jumpstate = JumpState_None;
            }

            updateAnimQueue();
            if (!mAnimQueue.empty())
                idlestate = CharState_SpecialIdle;
            else if (sneak && !mInJump)
                idlestate = CharState_IdleSneak;
            else
                idlestate = CharState_Idle;

            if (inwater)
                idlestate = CharState_IdleSwim;

            if (!mSkipAnim)
            {
                refreshCurrentAnims(idlestate, movestate, jumpstate, updateWeaponState());
                updateIdleStormState(inwater);
            }

            if (isTurning())
            {
                if (duration > 0)
                {
                    float turnSpeed = std::min(1.5f, std::abs(rot.z()) / duration / static_cast<float>(osg::PI));
                    mAnimation->adjustSpeedMult(mCurrentMovement, turnSpeed);
                }
            }
            else if (mMovementState != CharState_None && mAdjustMovementAnimSpeed)
            {
                // Vanilla caps the played animation speed.
                const float maxSpeedMult = 10.f;
                const float speedMult = speed / mMovementAnimSpeed;
                mAnimation->adjustSpeedMult(mCurrentMovement, std::min(maxSpeedMult, speedMult));
                // Make sure the actual speed is the "expected" speed even though the animation is slower
                if (isMovementAnimationControlled())
                    scale *= std::max(1.f, speedMult / maxSpeedMult);
            }

            if (!mSkipAnim)
            {
                if (!isKnockedDown() && !isKnockedOut())
                {
                    if (rot != osg::Vec3f())
                        world->rotateObject(mPtr, rot, true);
                }
                else // avoid z-rotating for knockdown
                {
                    if (rot.x() != 0 && rot.y() != 0)
                    {
                        rot.z() = 0.0f;
                        world->rotateObject(mPtr, rot, true);
                    }
                }

                updateHeadTracking(duration);
            }

            movement = vec;
            movementSettings.mPosition[0] = movementSettings.mPosition[1] = 0;

            // Can't reset jump state (mPosition[2]) here in full; we don't know for sure whether the PhysicsSystem will
            // actually handle it in this frame due to the fixed minimum timestep used for the physics update. It will
            // be reset in PhysicsSystem::move once the jump is handled.
            if (movement.z() == 0.f)
                movementSettings.mPosition[2] = 0;
        }
        else if (cls.getCreatureStats(mPtr).isDead())
        {
            // initial start of death animation for actors that started the game as dead
            // not done in constructor since we need to give scripts a chance to set the mSkipAnim flag
            if (!mSkipAnim && mDeathState != CharState_None && mCurrentDeath.empty())
            {
                // Fast-forward death animation to end for persisting corpses or corpses after end of death animation
                if (cls.isPersistent(mPtr) || cls.getCreatureStats(mPtr).isDeathAnimationFinished())
                    playDeath(1.f, mDeathState);
            }
        }

        osg::Vec3f movementFromAnimation
            = mAnimation->runAnimation(mSkipAnim && !isScriptedAnimPlaying() ? 0.f : duration);

        if (mPtr.getClass().isActor() && !isScriptedAnimPlaying())
        {
            if (isMovementAnimationControlled())
            {
                if (duration != 0.f && movementFromAnimation != osg::Vec3f())
                {
                    movementFromAnimation /= duration;

                    // Ensure we're moving in the right general direction.
                    // In vanilla, all horizontal movement is taken from animations, even when moving diagonally (which
                    // doesn't have a corresponding animation). So to achieve diagonal movement, we have to rotate the
                    // movement taken from the animation to the intended direction.
                    //
                    // Note that while a complete movement animation cycle will have a well defined direction, no
                    // individual frame will, and therefore we have to determine the direction based on the currently
                    // playing cycle instead.
                    if (speed > 0.f)
                    {
                        float animMovementAngle = getAnimationMovementDirection();
                        float targetMovementAngle = std::atan2(-movement.x(), movement.y());
                        float diff = targetMovementAngle - animMovementAngle;
                        movementFromAnimation = osg::Quat(diff, osg::Vec3f(0, 0, 1)) * movementFromAnimation;
                    }

                    movement = movementFromAnimation;
                }
                else
                {
                    movement = osg::Vec3f();
                }
            }
            else if (mSkipAnim)
            {
                movement = osg::Vec3f();
            }

            if (mFloatToSurface && world->isSwimming(mPtr))
            {
                if (cls.getCreatureStats(mPtr).isDead()
                    || (!godmode
                        && cls.getCreatureStats(mPtr)
                                .getMagicEffects()
                                .getOrDefault(ESM::MagicEffect::Paralyze)
                                .getModifier()
                            > 0))
                {
                    movement.z() = 1.0;
                }
            }

            movement.x() *= scale;
            movement.y() *= scale;
            world->queueMovement(mPtr, movement);
        }

        mSkipAnim = false;

        mAnimation->enableHeadAnimation(cls.isActor() && !cls.getCreatureStats(mPtr).isDead());
    }

    void CharacterController::persistAnimationState() const
    {
        ESM::AnimationState& state = mPtr.getRefData().getAnimationState();

        state.mScriptedAnims.clear();
        for (AnimationQueue::const_iterator iter = mAnimQueue.begin(); iter != mAnimQueue.end(); ++iter)
        {
            // TODO: Probably want to presist lua animations too
            if (!iter->mScripted)
                continue;

            ESM::AnimationState::ScriptedAnimation anim;
            anim.mGroup = iter->mGroup;

            if (iter == mAnimQueue.begin() && mAnimation)
            {
                float complete;
                uint32_t loopcount;
                mAnimation->getInfo(anim.mGroup, &complete, nullptr, &loopcount);
                anim.mTime = complete;
                anim.mLoopCount = loopcount;
            }
            else
            {
                anim.mLoopCount = iter->mLoopCount;
                anim.mTime = 0.f;
            }

            state.mScriptedAnims.push_back(std::move(anim));
        }
    }

    void CharacterController::unpersistAnimationState()
    {
        const ESM::AnimationState& state = mPtr.getRefData().getAnimationState();

        if (!state.mScriptedAnims.empty())
        {
            clearAnimQueue();
            for (const ESM::AnimationState::ScriptedAnimation& animation : state.mScriptedAnims)
            {
                AnimationQueueEntry entry;
                entry.mGroup = animation.mGroup;
                entry.mLoopCount = static_cast<uint32_t>(
                    std::min<uint64_t>(animation.mLoopCount, std::numeric_limits<uint32_t>::max()));
                entry.mLooping = mAnimation->isLoopingAnimation(entry.mGroup);
                entry.mScripted = true;
                entry.mStartKey = "start";
                entry.mStopKey = "stop";
                entry.mSpeed = 1.f;
                entry.mTime = animation.mTime;
                if (animation.mAbsolute)
                {
                    float start = mAnimation->getTextKeyTime(animation.mGroup + ": start");
                    float stop = mAnimation->getTextKeyTime(animation.mGroup + ": stop");
                    float time = std::clamp(animation.mTime, start, stop);
                    entry.mTime = (time - start) / (stop - start);
                }

                mAnimQueue.push_back(std::move(entry));
            }

            playAnimQueue();
        }
    }

    void CharacterController::playBlendedAnimation(const std::string& groupname, const MWRender::AnimPriority& priority,
        int blendMask, bool autodisable, float speedmult, std::string_view start, std::string_view stop,
        float startpoint, uint32_t loops, bool loopfallback) const
    {
        if (mLuaAnimations)
            MWBase::Environment::get().getLuaManager()->playAnimation(mPtr, groupname, priority, blendMask, autodisable,
                speedmult, start, stop, startpoint, loops, loopfallback);
        else
            mAnimation->play(
                groupname, priority, blendMask, autodisable, speedmult, start, stop, startpoint, loops, loopfallback);
    }

    bool CharacterController::playGroup(std::string_view groupname, int mode, uint32_t count, bool scripted)
    {
        if (!mAnimation || !mAnimation->hasAnimation(groupname))
            return false;

        // We should not interrupt scripted animations with non-scripted ones
        if (isScriptedAnimPlaying() && !scripted)
            return true;

        bool looping = mAnimation->isLoopingAnimation(groupname);

        // If this animation is a looped animation that is already playing
        // and has not yet reached the end of the loop, allow it to continue animating with its existing loop count
        // and remove any other animations that were queued.
        // This emulates observed behavior from the original allows the script "OutsideBanner" to animate banners
        // correctly.
        if (!mAnimQueue.empty() && mAnimQueue.front().mGroup == groupname && looping
            && mAnimation->isPlaying(groupname))
        {
            float endOfLoop = mAnimation->getTextKeyTime(mAnimQueue.front().mGroup + ": loop stop");

            if (endOfLoop < 0) // if no Loop Stop key was found, use the Stop key
                endOfLoop = mAnimation->getTextKeyTime(mAnimQueue.front().mGroup + ": stop");

            if (endOfLoop > 0 && (mAnimation->getCurrentTime(mAnimQueue.front().mGroup) < endOfLoop))
            {
                mAnimQueue.resize(1);
                return true;
            }
        }

        // The loop count in vanilla is weird.
        // if played with a count of 0, all objects play exactly once from start to stop.
        // But if the count is x > 0, actors and non-actors behave differently. actors will loop
        // exactly x times, while non-actors will loop x+1 instead.
        if (mPtr.getClass().isActor() && count > 0)
            count--;

        AnimationQueueEntry entry;
        entry.mGroup = groupname;
        entry.mLoopCount = count;
        entry.mTime = 0.f;
        // "PlayGroup idle" is a special case, used to remove to stop scripted animations playing
        entry.mScripted = (scripted && groupname != "idle");
        entry.mLooping = looping;
        entry.mSpeed = 1.f;
        entry.mStartKey = ((mode == 2) ? "loop start" : "start");
        entry.mStopKey = "stop";

        bool playImmediately = false;

        if (mode != 0 || mAnimQueue.empty() || !isAnimPlaying(mAnimQueue.front().mGroup))
        {
            clearAnimQueue(scripted);

            playImmediately = true;
        }
        else
        {
            mAnimQueue.resize(1);
        }

        mAnimQueue.push_back(std::move(entry));

        if (playImmediately)
            playAnimQueue(mode == 2);

        return true;
    }

    bool CharacterController::playGroupLua(std::string_view groupname, float speed, std::string_view startKey,
        std::string_view stopKey, uint32_t loops, bool forceLoop)
    {
        // Note: In mwscript, "idle" is a special case used to clear the anim queue.
        // In lua we offer an explicit clear method instead so this method does not treat "idle" special.

        if (!mAnimation || !mAnimation->hasAnimation(groupname))
            return false;

        AnimationQueueEntry entry;
        entry.mGroup = groupname;
        // Note: MWScript gives one less loop to actors than non-actors.
        // But this is the Lua version. We don't need to reproduce this weirdness here.
        entry.mLoopCount = loops;
        entry.mStartKey = startKey;
        entry.mStopKey = stopKey;
        entry.mLooping = mAnimation->isLoopingAnimation(groupname) || forceLoop;
        entry.mScripted = true;
        entry.mSpeed = speed;
        entry.mTime = 0;

        if (mAnimQueue.size() > 1)
            mAnimQueue.resize(1);
        mAnimQueue.push_back(std::move(entry));

        if (mAnimQueue.size() == 1)
            playAnimQueue();

        return true;
    }

    void CharacterController::enableLuaAnimations(bool enable)
    {
        mLuaAnimations = enable;
    }

    void CharacterController::skipAnim()
    {
        mSkipAnim = true;
    }

    bool CharacterController::isScriptedAnimPlaying() const
    {
        // If the front of the anim queue is scripted, morrowind treats it as if it's
        // still playing even if it's actually done.
        if (!mAnimQueue.empty())
            return mAnimQueue.front().mScripted;

        return false;
    }

    bool CharacterController::isAnimPlaying(std::string_view groupName) const
    {
        if (mAnimation == nullptr)
            return false;
        return mAnimation->isPlaying(groupName);
    }

    bool CharacterController::isMovementAnimationControlled() const
    {
        if (mHitState != CharState_None)
            return true;

        if (Settings::game().mPlayerMovementIgnoresAnimation && mPtr == getPlayer())
            return false;

        if (mInJump)
            return false;

        bool movementAnimationControlled = mIdleState != CharState_None;
        if (mMovementState != CharState_None)
            movementAnimationControlled = mMovementAnimationHasMovement;
        return movementAnimationControlled;
    }

    void CharacterController::clearAnimQueue(bool clearScriptedAnims)
    {
        // Do not interrupt scripted animations, if we want to keep them
        if (mAnimation && (!isScriptedAnimPlaying() || clearScriptedAnims) && !mAnimQueue.empty())
            mAnimation->disable(mAnimQueue.front().mGroup);

        if (clearScriptedAnims)
        {
            if (mAnimation)
                mAnimation->setPlayScriptedOnly(false);
            mAnimQueue.clear();
            return;
        }

        std::erase_if(mAnimQueue, [](const AnimationQueueEntry& entry) { return !entry.mScripted; });
    }

    void CharacterController::forceStateUpdate()
    {
        if (!mAnimation)
            return;
        clearAnimQueue();

        // Make sure we canceled the current attack or spellcasting,
        // because we disabled attack animations anyway.
        mCanCast = false;
        mCastingScriptedSpell = false;
        setAttackingOrSpell(false);
        if (mUpperBodyState != UpperBodyState::None)
            mUpperBodyState = UpperBodyState::WeaponEquipped;

        refreshCurrentAnims(mIdleState, mMovementState, mJumpState, true);

        if (mDeathState != CharState_None)
        {
            playRandomDeath();
        }

        updateAnimQueue();

        mAnimation->runAnimation(0.f);
    }

    CharacterController::KillResult CharacterController::kill()
    {
        if (mDeathState == CharState_None)
        {
            playRandomDeath();
            resetCurrentIdleState();
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
        if (mDeathState == CharState_None)
            return;

        resetCurrentDeathState();
        mWeaponType = ESM::Weapon::None;
    }

    void CharacterController::updateContinuousVfx() const
    {
        // Keeping track of when to stop a continuous VFX seems to be very difficult to do inside the spells code,
        // as it's extremely spread out (ActiveSpells, Spells, InventoryStore effects, etc...) so we do it here.

        // Stop any effects that are no longer active
        std::vector<std::string_view> effects = mAnimation->getLoopingEffects();

        for (std::string_view effectStr : effects)
        {
            auto effectId = ESM::MagicEffectId(effectStr);

            if (mPtr.getClass().getCreatureStats(mPtr).isDeathAnimationFinished()
                    || mPtr.getClass()
                            .getCreatureStats(mPtr)
                            .getMagicEffects()
                            .getOrDefault(MWMechanics::EffectKey(effectId))
                            .getMagnitude()
                        <= 0)
                mAnimation->removeEffect(effectStr);
        }
    }

    void CharacterController::updateMagicEffects() const
    {
        if (!mPtr.getClass().isActor())
            return;

        float light = mPtr.getClass()
                          .getCreatureStats(mPtr)
                          .getMagicEffects()
                          .getOrDefault(ESM::MagicEffect::Light)
                          .getMagnitude();
        mAnimation->setLightEffect(light);

        // If you're dead you don't care about whether you've started/stopped being a vampire or not
        if (mPtr.getClass().getCreatureStats(mPtr).isDead())
            return;

        bool vampire = mPtr.getClass()
                           .getCreatureStats(mPtr)
                           .getMagicEffects()
                           .getOrDefault(ESM::MagicEffect::Vampirism)
                           .getMagnitude()
            > 0.0f;
        mAnimation->setVampire(vampire);
    }

    void CharacterController::setVisibility(float visibility) const
    {
        if (!mAnimation)
            return;
        // We should take actor's invisibility in account
        if (mPtr.getClass().isActor())
        {
            float alpha = 1.f;
            if (mPtr.getClass()
                    .getCreatureStats(mPtr)
                    .getMagicEffects()
                    .getOrDefault(ESM::MagicEffect::Invisibility)
                    .getModifier()) // Ignore base magnitude (see bug #3555).
            {
                if (mPtr == getPlayer())
                    alpha = 0.25f;
                else
                    alpha = 0.05f;
            }
            float chameleon = mPtr.getClass()
                                  .getCreatureStats(mPtr)
                                  .getMagicEffects()
                                  .getOrDefault(ESM::MagicEffect::Chameleon)
                                  .getMagnitude();
            if (chameleon)
            {
                alpha *= std::clamp(1.f - chameleon / 100.f, 0.25f, 0.75f);
            }

            visibility = std::min(visibility, alpha);
        }

        // TODO: implement a dithering shader rather than just change object transparency.
        mAnimation->setAlpha(visibility);
    }

    std::string_view CharacterController::getMovementBasedAttackType() const
    {
        float* move = mPtr.getClass().getMovementSettings(mPtr).mPosition;
        if (std::abs(move[1]) > std::abs(move[0]) + 0.2f) // forward-backward
            return "thrust";
        if (std::abs(move[0]) > std::abs(move[1]) + 0.2f) // sideway
            return "slash";
        return "chop";
    }

    bool CharacterController::isRandomAttackAnimation(std::string_view group)
    {
        return (group == "attack1" || group == "swimattack1" || group == "attack2" || group == "swimattack2"
            || group == "attack3" || group == "swimattack3");
    }

    bool CharacterController::isAttackPreparing() const
    {
        return mUpperBodyState == UpperBodyState::AttackWindUp;
    }

    bool CharacterController::isCastingSpell() const
    {
        return mCastingScriptedSpell || mUpperBodyState == UpperBodyState::Casting;
    }

    bool CharacterController::isReadyToBlock() const
    {
        return mAnimation && updateCarriedLeftVisible(mWeaponType);
    }

    bool CharacterController::isKnockedDown() const
    {
        return mHitState == CharState_KnockDown || mHitState == CharState_SwimKnockDown;
    }

    bool CharacterController::isKnockedOut() const
    {
        return mHitState == CharState_KnockOut || mHitState == CharState_SwimKnockOut;
    }

    bool CharacterController::isTurning() const
    {
        return mMovementState == CharState_TurnLeft || mMovementState == CharState_TurnRight
            || mMovementState == CharState_SwimTurnLeft || mMovementState == CharState_SwimTurnRight;
    }

    bool CharacterController::isRecovery() const
    {
        return mHitState == CharState_Hit || mHitState == CharState_SwimHit;
    }

    bool CharacterController::isAttackingOrSpell() const
    {
        return mUpperBodyState != UpperBodyState::None && mUpperBodyState != UpperBodyState::WeaponEquipped;
    }

    bool CharacterController::isSneaking() const
    {
        return mIdleState == CharState_IdleSneak || mMovementState == CharState_SneakForward
            || mMovementState == CharState_SneakBack || mMovementState == CharState_SneakLeft
            || mMovementState == CharState_SneakRight;
    }

    bool CharacterController::isRunning() const
    {
        return mMovementState == CharState_RunForward || mMovementState == CharState_RunBack
            || mMovementState == CharState_RunLeft || mMovementState == CharState_RunRight
            || mMovementState == CharState_SwimRunForward || mMovementState == CharState_SwimRunBack
            || mMovementState == CharState_SwimRunLeft || mMovementState == CharState_SwimRunRight;
    }

    void CharacterController::setAttackingOrSpell(bool attackingOrSpell) const
    {
        mPtr.getClass().getCreatureStats(mPtr).setAttackingOrSpell(attackingOrSpell);
    }

    void CharacterController::castSpell(const ESM::RefId& spellId, bool scriptedSpell)
    {
        setAttackingOrSpell(true);
        mCastingScriptedSpell = scriptedSpell;
        ActionSpell action = ActionSpell(spellId);
        action.prepare(mPtr);
    }

    void CharacterController::setAIAttackType(std::string_view attackType)
    {
        mAttackType = attackType;
    }

    std::string_view CharacterController::getRandomAttackType()
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        float random = Misc::Rng::rollProbability(world->getPrng());
        if (random >= 2 / 3.f)
            return "thrust";
        if (random >= 1 / 3.f)
            return "slash";
        return "chop";
    }

    bool CharacterController::readyToPrepareAttack() const
    {
        return (mHitState == CharState_None || mHitState == CharState_Block)
            && mUpperBodyState <= UpperBodyState::WeaponEquipped;
    }

    bool CharacterController::readyToStartAttack() const
    {
        if (mHitState != CharState_None && mHitState != CharState_Block)
            return false;

        return mUpperBodyState == UpperBodyState::WeaponEquipped;
    }

    float CharacterController::getAttackStrength() const
    {
        return mAttackStrength;
    }

    bool CharacterController::getAttackingOrSpell() const
    {
        return mPtr.getClass().getCreatureStats(mPtr).getAttackingOrSpell();
    }

    std::string_view CharacterController::getDesiredAttackType() const
    {
        return mPtr.getClass().getCreatureStats(mPtr).getAttackType();
    }

    void CharacterController::setActive(int active) const
    {
        if (mAnimation)
            mAnimation->setActive(active);
    }

    void CharacterController::setHeadTrackTarget(const MWWorld::ConstPtr& target)
    {
        mHeadTrackTarget = target;
    }

    void CharacterController::playSwishSound() const
    {
        static ESM::RefId weaponSwish = ESM::RefId::stringRefId("Weapon Swish");
        const ESM::RefId* soundId = &weaponSwish;
        float volume = 0.98f + mAttackStrength * 0.02f;
        float pitch = 0.75f + mAttackStrength * 0.4f;

        const MWWorld::Class& cls = mPtr.getClass();
        if (cls.isNpc() && cls.getNpcStats(mPtr).isWerewolf())
        {
            MWBase::World* world = MWBase::Environment::get().getWorld();
            const MWWorld::ESMStore& store = world->getStore();
            const ESM::Sound* sound = store.get<ESM::Sound>().searchRandom("WolfSwing", world->getPrng());
            if (sound)
                soundId = &sound->mId;
        }

        if (!soundId->empty())
            MWBase::Environment::get().getSoundManager()->playSound3D(mPtr, *soundId, volume, pitch);
    }

    float CharacterController::getAnimationMovementDirection() const
    {
        if (!mAnimation)
            return 0.f;
        switch (mMovementState)
        {
            case CharState_RunLeft:
            case CharState_SneakLeft:
            case CharState_SwimWalkLeft:
            case CharState_SwimRunLeft:
            case CharState_WalkLeft:
                return osg::PI_2f;
            case CharState_RunRight:
            case CharState_SneakRight:
            case CharState_SwimWalkRight:
            case CharState_SwimRunRight:
            case CharState_WalkRight:
                return -osg::PI_2f;
            case CharState_RunForward:
            case CharState_SneakForward:
            case CharState_SwimRunForward:
            case CharState_SwimWalkForward:
            case CharState_WalkForward:
                return mAnimation->getLegsYawRadians();
            case CharState_RunBack:
            case CharState_SneakBack:
            case CharState_SwimWalkBack:
            case CharState_SwimRunBack:
            case CharState_WalkBack:
                return mAnimation->getLegsYawRadians() - osg::PIf;
            default:
                return 0.0f;
        }
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
                    direction = MWBase::Environment::get().getWorld()->aimToTarget(mPtr, mHeadTrackTarget, false);
            }
            direction.normalize();

            if (!mPtr.getRefData().getBaseNode())
                return;
            const osg::Vec3f actorDirection = mPtr.getRefData().getBaseNode()->getAttitude() * osg::Vec3f(0, 1, 0);

            zAngleRadians
                = std::atan2(actorDirection.x(), actorDirection.y()) - std::atan2(direction.x(), direction.y());
            zAngleRadians = static_cast<float>(
                Misc::normalizeAngle(zAngleRadians - mAnimation->getHeadYaw()) + mAnimation->getHeadYaw());
            zAngleRadians *= (1 - direction.z() * direction.z());
            xAngleRadians = std::asin(direction.z());
        }

        const float xLimit = osg::DegreesToRadians(40.f);
        const float zLimit = osg::DegreesToRadians(30.f);
        float zLimitOffset = mAnimation->getUpperBodyYawRadians();
        xAngleRadians = std::clamp(xAngleRadians, -xLimit, xLimit);
        zAngleRadians = std::clamp(zAngleRadians, -zLimit + zLimitOffset, zLimit + zLimitOffset);

        float factor = duration * 5;
        factor = std::min(factor, 1.f);
        xAngleRadians = (1.f - factor) * mAnimation->getHeadPitch() + factor * xAngleRadians;
        zAngleRadians = (1.f - factor) * mAnimation->getHeadYaw() + factor * zAngleRadians;

        mAnimation->setHeadPitch(xAngleRadians);
        mAnimation->setHeadYaw(zAngleRadians);
    }

    MWWorld::MovementDirectionFlags CharacterController::getSupportedMovementDirections() const
    {
        if (!mAnimation)
            return 0;
        using namespace std::string_view_literals;
        // There are fallbacks in the CharacterController::refreshMovementAnims for certain animations. Arrays below
        // represent them.
        constexpr std::array all = { ""sv };
        constexpr std::array walk = { "walk"sv };
        constexpr std::array swimWalk = { "swimwalk"sv, "walk"sv };
        constexpr std::array sneak = { "sneak"sv };
        constexpr std::array run = { "run"sv, "walk"sv };
        constexpr std::array swimRun = { "swimrun"sv, "run"sv, "walk"sv };
        constexpr std::array swim = { "swim"sv };
        switch (mMovementState)
        {
            case CharState_None:
            case CharState_SpecialIdle:
            case CharState_Idle:
            case CharState_IdleSwim:
            case CharState_IdleSneak:
                return mAnimation->getSupportedMovementDirections(all);
            case CharState_WalkForward:
            case CharState_WalkBack:
            case CharState_WalkLeft:
            case CharState_WalkRight:
                return mAnimation->getSupportedMovementDirections(walk);
            case CharState_SwimWalkForward:
            case CharState_SwimWalkBack:
            case CharState_SwimWalkLeft:
            case CharState_SwimWalkRight:
                return mAnimation->getSupportedMovementDirections(swimWalk);
            case CharState_RunForward:
            case CharState_RunBack:
            case CharState_RunLeft:
            case CharState_RunRight:
                return mAnimation->getSupportedMovementDirections(run);
            case CharState_SwimRunForward:
            case CharState_SwimRunBack:
            case CharState_SwimRunLeft:
            case CharState_SwimRunRight:
                return mAnimation->getSupportedMovementDirections(swimRun);
            case CharState_SneakForward:
            case CharState_SneakBack:
            case CharState_SneakLeft:
            case CharState_SneakRight:
                return mAnimation->getSupportedMovementDirections(sneak);
            case CharState_TurnLeft:
            case CharState_TurnRight:
                return mAnimation->getSupportedMovementDirections(all);
            case CharState_SwimTurnLeft:
            case CharState_SwimTurnRight:
                return mAnimation->getSupportedMovementDirections(swim);
            case CharState_Death1:
            case CharState_Death2:
            case CharState_Death3:
            case CharState_Death4:
            case CharState_Death5:
            case CharState_SwimDeath:
            case CharState_SwimDeathKnockDown:
            case CharState_SwimDeathKnockOut:
            case CharState_DeathKnockDown:
            case CharState_DeathKnockOut:
            case CharState_Hit:
            case CharState_SwimHit:
            case CharState_KnockDown:
            case CharState_KnockOut:
            case CharState_SwimKnockDown:
            case CharState_SwimKnockOut:
            case CharState_Block:
                return mAnimation->getSupportedMovementDirections(all);
        }
        return 0;
    }

}
