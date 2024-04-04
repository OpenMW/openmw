#ifndef OPENMW_COMPONENTS_SETTINGS_CATEGORIES_GAME_H
#define OPENMW_COMPONENTS_SETTINGS_CATEGORIES_GAME_H

#include <components/detournavigator/collisionshapetype.hpp>
#include <components/settings/sanitizerimpl.hpp>
#include <components/settings/settingvalue.hpp>

#include <osg/Math>
#include <osg/Vec2f>
#include <osg/Vec3f>

#include <cstdint>
#include <string>
#include <string_view>

namespace Settings
{
    struct GameCategory : WithIndex
    {
        using WithIndex::WithIndex;

        SettingValue<int> mShowOwned{ mIndex, "Game", "show owned", makeEnumSanitizerInt({ 0, 1, 2, 3 }) };
        SettingValue<bool> mShowProjectileDamage{ mIndex, "Game", "show projectile damage" };
        SettingValue<bool> mShowMeleeInfo{ mIndex, "Game", "show melee info" };
        SettingValue<bool> mShowEnchantChance{ mIndex, "Game", "show enchant chance" };
        SettingValue<bool> mBestAttack{ mIndex, "Game", "best attack" };
        SettingValue<int> mDifficulty{ mIndex, "Game", "difficulty", makeClampSanitizerInt(-500, 500) };
        // We have to cap it since using high values (larger than 7168) will make some quests harder or impossible to
        // complete (bug #1876)
        SettingValue<int> mActorsProcessingRange{ mIndex, "Game", "actors processing range",
            makeClampSanitizerInt(3584, 7168) };
        SettingValue<bool> mClassicReflectedAbsorbSpellsBehavior{ mIndex, "Game",
            "classic reflected absorb spells behavior" };
        SettingValue<bool> mClassicCalmSpellsBehavior{ mIndex, "Game", "classic calm spells behavior" };
        SettingValue<bool> mShowEffectDuration{ mIndex, "Game", "show effect duration" };
        SettingValue<bool> mPreventMerchantEquipping{ mIndex, "Game", "prevent merchant equipping" };
        SettingValue<bool> mEnchantedWeaponsAreMagical{ mIndex, "Game", "enchanted weapons are magical" };
        SettingValue<bool> mFollowersAttackOnSight{ mIndex, "Game", "followers attack on sight" };
        SettingValue<bool> mCanLootDuringDeathAnimation{ mIndex, "Game", "can loot during death animation" };
        SettingValue<bool> mRebalanceSoulGemValues{ mIndex, "Game", "rebalance soul gem values" };
        SettingValue<bool> mUseAdditionalAnimSources{ mIndex, "Game", "use additional anim sources" };
        SettingValue<bool> mSmoothAnimTransitions{ mIndex, "Game", "smooth animation transitions" };
        SettingValue<bool> mBarterDispositionChangeIsPermanent{ mIndex, "Game",
            "barter disposition change is permanent" };
        SettingValue<int> mStrengthInfluencesHandToHand{ mIndex, "Game", "strength influences hand to hand",
            makeEnumSanitizerInt({ 0, 1, 2 }) };
        SettingValue<bool> mWeaponSheathing{ mIndex, "Game", "weapon sheathing" };
        SettingValue<bool> mShieldSheathing{ mIndex, "Game", "shield sheathing" };
        SettingValue<bool> mOnlyAppropriateAmmunitionBypassesResistance{ mIndex, "Game",
            "only appropriate ammunition bypasses resistance" };
        SettingValue<bool> mUseMagicItemAnimations{ mIndex, "Game", "use magic item animations" };
        SettingValue<bool> mNormaliseRaceSpeed{ mIndex, "Game", "normalise race speed" };
        SettingValue<float> mProjectilesEnchantMultiplier{ mIndex, "Game", "projectiles enchant multiplier",
            makeClampSanitizerFloat(0, 1) };
        SettingValue<bool> mUncappedDamageFatigue{ mIndex, "Game", "uncapped damage fatigue" };
        SettingValue<bool> mTurnToMovementDirection{ mIndex, "Game", "turn to movement direction" };
        SettingValue<bool> mSmoothMovement{ mIndex, "Game", "smooth movement" };
        SettingValue<float> mSmoothMovementPlayerTurningDelay{ mIndex, "Game", "smooth movement player turning delay",
            makeMaxSanitizerFloat(0.01f) };
        SettingValue<bool> mNPCsAvoidCollisions{ mIndex, "Game", "NPCs avoid collisions" };
        SettingValue<bool> mNPCsGiveWay{ mIndex, "Game", "NPCs give way" };
        SettingValue<bool> mSwimUpwardCorrection{ mIndex, "Game", "swim upward correction" };
        SettingValue<float> mSwimUpwardCoef{ mIndex, "Game", "swim upward coef", makeClampSanitizerFloat(-1, 1) };
        SettingValue<bool> mTrainersTrainingSkillsBasedOnBaseSkill{ mIndex, "Game",
            "trainers training skills based on base skill" };
        SettingValue<bool> mAlwaysAllowStealingFromKnockedOutActors{ mIndex, "Game",
            "always allow stealing from knocked out actors" };
        SettingValue<bool> mGraphicHerbalism{ mIndex, "Game", "graphic herbalism" };
        SettingValue<bool> mAllowActorsToFollowOverWaterSurface{ mIndex, "Game",
            "allow actors to follow over water surface" };
        SettingValue<osg::Vec3f> mDefaultActorPathfindHalfExtents{ mIndex, "Game",
            "default actor pathfind half extents", makeMaxStrictSanitizerVec3f(osg::Vec3f(0, 0, 0)) };
        SettingValue<bool> mDayNightSwitches{ mIndex, "Game", "day night switches" };
        SettingValue<bool> mUnarmedCreatureAttacksDamageArmor{ mIndex, "Game",
            "unarmed creature attacks damage armor" };
        SettingValue<DetourNavigator::CollisionShapeType> mActorCollisionShapeType{ mIndex, "Game",
            "actor collision shape type" };
        SettingValue<bool> mPlayerMovementIgnoresAnimation{ mIndex, "Game", "player movement ignores animation" };
    };
}

#endif
