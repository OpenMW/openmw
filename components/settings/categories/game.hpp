#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_GAME_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_GAME_H

#include "components/settings/sanitizerimpl.hpp"
#include "components/settings/settingvalue.hpp"

#include <osg/Math>
#include <osg/Vec2f>
#include <osg/Vec3f>

#include <cstdint>
#include <string>
#include <string_view>

namespace Settings
{
    struct GameCategory
    {
        SettingValue<int> mShowOwned{ "Game", "show owned", makeEnumSanitizerInt({ 0, 1, 2, 3 }) };
        SettingValue<bool> mShowProjectileDamage{ "Game", "show projectile damage" };
        SettingValue<bool> mShowMeleeInfo{ "Game", "show melee info" };
        SettingValue<bool> mShowEnchantChance{ "Game", "show enchant chance" };
        SettingValue<bool> mBestAttack{ "Game", "best attack" };
        SettingValue<int> mDifficulty{ "Game", "difficulty", makeClampSanitizerInt(-500, 500) };
        SettingValue<int> mActorsProcessingRange{ "Game", "actors processing range",
            makeClampSanitizerInt(3584, 7168) };
        SettingValue<bool> mClassicReflectedAbsorbSpellsBehavior{ "Game", "classic reflected absorb spells behavior" };
        SettingValue<bool> mClassicCalmSpellsBehavior{ "Game", "classic calm spells behavior" };
        SettingValue<bool> mShowEffectDuration{ "Game", "show effect duration" };
        SettingValue<bool> mPreventMerchantEquipping{ "Game", "prevent merchant equipping" };
        SettingValue<bool> mEnchantedWeaponsAreMagical{ "Game", "enchanted weapons are magical" };
        SettingValue<bool> mFollowersAttackOnSight{ "Game", "followers attack on sight" };
        SettingValue<bool> mCanLootDuringDeathAnimation{ "Game", "can loot during death animation" };
        SettingValue<bool> mRebalanceSoulGemValues{ "Game", "rebalance soul gem values" };
        SettingValue<bool> mUseAdditionalAnimSources{ "Game", "use additional anim sources" };
        SettingValue<bool> mBarterDispositionChangeIsPermanent{ "Game", "barter disposition change is permanent" };
        SettingValue<int> mStrengthInfluencesHandToHand{ "Game", "strength influences hand to hand",
            makeEnumSanitizerInt({ 0, 1, 2 }) };
        SettingValue<bool> mWeaponSheathing{ "Game", "weapon sheathing" };
        SettingValue<bool> mShieldSheathing{ "Game", "shield sheathing" };
        SettingValue<bool> mOnlyAppropriateAmmunitionBypassesResistance{ "Game",
            "only appropriate ammunition bypasses resistance" };
        SettingValue<bool> mUseMagicItemAnimations{ "Game", "use magic item animations" };
        SettingValue<bool> mNormaliseRaceSpeed{ "Game", "normalise race speed" };
        SettingValue<float> mProjectilesEnchantMultiplier{ "Game", "projectiles enchant multiplier",
            makeClampSanitizerFloat(0, 1) };
        SettingValue<bool> mUncappedDamageFatigue{ "Game", "uncapped damage fatigue" };
        SettingValue<bool> mTurnToMovementDirection{ "Game", "turn to movement direction" };
        SettingValue<bool> mSmoothMovement{ "Game", "smooth movement" };
        SettingValue<float> mSmoothMovementPlayerTurningDelay{ "Game", "smooth movement player turning delay",
            makeMaxSanitizerFloat(0.01f) };
        SettingValue<bool> mNPCsAvoidCollisions{ "Game", "NPCs avoid collisions" };
        SettingValue<bool> mNPCsGiveWay{ "Game", "NPCs give way" };
        SettingValue<bool> mSwimUpwardCorrection{ "Game", "swim upward correction" };
        SettingValue<float> mSwimUpwardCoef{ "Game", "swim upward coef", makeClampSanitizerFloat(-1, 1) };
        SettingValue<bool> mTrainersTrainingSkillsBasedOnBaseSkill{ "Game",
            "trainers training skills based on base skill" };
        SettingValue<bool> mAlwaysAllowStealingFromKnockedOutActors{ "Game",
            "always allow stealing from knocked out actors" };
        SettingValue<bool> mGraphicHerbalism{ "Game", "graphic herbalism" };
        SettingValue<bool> mAllowActorsToFollowOverWaterSurface{ "Game", "allow actors to follow over water surface" };
        SettingValue<osg::Vec3f> mDefaultActorPathfindHalfExtents{ "Game", "default actor pathfind half extents",
            makeMaxStrictSanitizerVec3f(osg::Vec3f(0, 0, 0)) };
        SettingValue<bool> mDayNightSwitches{ "Game", "day night switches" };
        SettingValue<bool> mUnarmedCreatureAttacksDamageArmor{ "Game", "unarmed creature attacks damage armor" };
        SettingValue<int> mActorCollisionShapeType{ "Game", "actor collision shape type",
            makeEnumSanitizerInt({ 0, 1, 2 }) };
    };
}

#endif
