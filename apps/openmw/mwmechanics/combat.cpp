
#include "combat.hpp"

#include <components/misc/rng.hpp>
#include <components/settings/values.hpp>

#include <components/sceneutil/positionattitudetransform.hpp>

#include <components/esm3/loadench.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadsoun.hpp>

#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/globals.hpp"
#include "../mwworld/inventorystore.hpp"

#include "actorutil.hpp"
#include "difficultyscaling.hpp"
#include "movement.hpp"
#include "npcstats.hpp"
#include "pathfinding.hpp"
#include "rumble.hpp"
#include "spellcasting.hpp"
#include "spellresistance.hpp"

namespace
{

    float signedAngleRadians(const osg::Vec3f& v1, const osg::Vec3f& v2, const osg::Vec3f& normal)
    {
        return std::atan2((normal * (v1 ^ v2)), (v1 * v2));
    }

}

namespace MWMechanics
{

    bool applyOnStrikeEnchantment(const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim, const MWWorld::Ptr& object,
        const osg::Vec3f& hitPosition, const bool fromProjectile)
    {
        const ESM::RefId enchantmentName = !object.isEmpty() ? object.getClass().getEnchantment(object) : ESM::RefId();
        if (!enchantmentName.empty())
        {
            const ESM::Enchantment* enchantment
                = MWBase::Environment::get().getESMStore()->get<ESM::Enchantment>().find(enchantmentName);
            if (enchantment->mData.mType == ESM::Enchantment::WhenStrikes)
            {
                MWMechanics::CastSpell cast(attacker, victim, fromProjectile);
                cast.mHitPosition = hitPosition;
                cast.cast(object, false);
                // Apply magic effects directly instead of waiting a frame to allow soul trap to work on one-hit kills
                if (!victim.isEmpty() && victim.getClass().isActor())
                    MWBase::Environment::get().getMechanicsManager()->updateMagicEffects(victim);
                return true;
            }
        }
        return false;
    }

    bool blockMeleeAttack(const MWWorld::Ptr& attacker, const MWWorld::Ptr& blocker, const MWWorld::Ptr& weapon,
        float damage, float attackStrength)
    {
        if (!blocker.getClass().hasInventoryStore(blocker))
            return false;

        MWMechanics::CreatureStats& blockerStats = blocker.getClass().getCreatureStats(blocker);

        if (blockerStats.getKnockedDown() // Used for both knockout or knockdown
            || blockerStats.getHitRecovery() || blockerStats.isParalyzed())
            return false;

        if (!MWBase::Environment::get().getMechanicsManager()->isReadyToBlock(blocker))
            return false;

        MWWorld::InventoryStore& inv = blocker.getClass().getInventoryStore(blocker);
        MWWorld::ContainerStoreIterator shield = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        if (shield == inv.end() || shield->getType() != ESM::Armor::sRecordId)
            return false;

        if (!blocker.getRefData().getBaseNode())
            return false; // shouldn't happen

        float angleDegrees = osg::RadiansToDegrees(signedAngleRadians(
            (attacker.getRefData().getPosition().asVec3() - blocker.getRefData().getPosition().asVec3()),
            blocker.getRefData().getBaseNode()->getAttitude() * osg::Vec3f(0, 1, 0), osg::Vec3f(0, 0, 1)));

        const MWWorld::Store<ESM::GameSetting>& gmst
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();
        static const float fCombatBlockLeftAngle = gmst.find("fCombatBlockLeftAngle")->mValue.getFloat();
        if (angleDegrees < fCombatBlockLeftAngle)
            return false;
        static const float fCombatBlockRightAngle = gmst.find("fCombatBlockRightAngle")->mValue.getFloat();
        if (angleDegrees > fCombatBlockRightAngle)
            return false;

        MWMechanics::CreatureStats& attackerStats = attacker.getClass().getCreatureStats(attacker);

        float blockTerm = blocker.getClass().getSkill(blocker, ESM::Skill::Block)
            + 0.2f * blockerStats.getAttribute(ESM::Attribute::Agility).getModified()
            + 0.1f * blockerStats.getAttribute(ESM::Attribute::Luck).getModified();
        float enemySwing = attackStrength;
        static const float fSwingBlockMult = gmst.find("fSwingBlockMult")->mValue.getFloat();
        static const float fSwingBlockBase = gmst.find("fSwingBlockBase")->mValue.getFloat();
        float swingTerm = enemySwing * fSwingBlockMult + fSwingBlockBase;

        float blockerTerm = blockTerm * swingTerm;
        if (blocker.getClass().getMovementSettings(blocker).mPosition[1] <= 0)
        {
            static const float fBlockStillBonus = gmst.find("fBlockStillBonus")->mValue.getFloat();
            blockerTerm *= fBlockStillBonus;
        }
        blockerTerm *= blockerStats.getFatigueTerm();

        float attackerSkill = 0;
        if (weapon.isEmpty())
            attackerSkill = attacker.getClass().getSkill(attacker, ESM::Skill::HandToHand);
        else
            attackerSkill = attacker.getClass().getSkill(attacker, weapon.getClass().getEquipmentSkill(weapon));
        float attackerTerm = attackerSkill + 0.2f * attackerStats.getAttribute(ESM::Attribute::Agility).getModified()
            + 0.1f * attackerStats.getAttribute(ESM::Attribute::Luck).getModified();
        attackerTerm *= attackerStats.getFatigueTerm();

        static const int iBlockMaxChance = gmst.find("iBlockMaxChance")->mValue.getInteger();
        static const int iBlockMinChance = gmst.find("iBlockMinChance")->mValue.getInteger();
        int x = std::clamp<int>(blockerTerm - attackerTerm, iBlockMinChance, iBlockMaxChance);

        auto& prng = MWBase::Environment::get().getWorld()->getPrng();
        if (Misc::Rng::roll0to99(prng) < x)
        {
            MWBase::SoundManager* sndMgr = MWBase::Environment::get().getSoundManager();
            const ESM::RefId skill = shield->getClass().getEquipmentSkill(*shield);
            if (skill == ESM::Skill::LightArmor)
                sndMgr->playSound3D(blocker, ESM::RefId::stringRefId("Light Armor Hit"), 1.0f, 1.0f);
            else if (skill == ESM::Skill::MediumArmor)
                sndMgr->playSound3D(blocker, ESM::RefId::stringRefId("Medium Armor Hit"), 1.0f, 1.0f);
            else if (skill == ESM::Skill::HeavyArmor)
                sndMgr->playSound3D(blocker, ESM::RefId::stringRefId("Heavy Armor Hit"), 1.0f, 1.0f);

            if (blocker == MWMechanics::getPlayer())
                MWMechanics::Rumble::onPlayerBlock(attackStrength, damage);
            if (attacker == MWMechanics::getPlayer())
                MWMechanics::Rumble::onPlayerAttackWasBlocked(damage);

            // Reduce shield durability by incoming damage
            int shieldhealth = shield->getClass().getItemHealth(*shield);

            shieldhealth -= std::min(shieldhealth, int(damage));
            shield->getCellRef().setCharge(shieldhealth);
            if (shieldhealth == 0)
                inv.unequipItem(*shield);
            // Reduce blocker fatigue
            static const float fFatigueBlockBase = gmst.find("fFatigueBlockBase")->mValue.getFloat();
            static const float fFatigueBlockMult = gmst.find("fFatigueBlockMult")->mValue.getFloat();
            static const float fWeaponFatigueBlockMult = gmst.find("fWeaponFatigueBlockMult")->mValue.getFloat();
            MWMechanics::DynamicStat<float> fatigue = blockerStats.getFatigue();
            float normalizedEncumbrance = blocker.getClass().getNormalizedEncumbrance(blocker);
            normalizedEncumbrance = std::min(1.f, normalizedEncumbrance);
            float fatigueLoss = fFatigueBlockBase + normalizedEncumbrance * fFatigueBlockMult;
            if (!weapon.isEmpty())
                fatigueLoss += weapon.getClass().getWeight(weapon) * attackStrength * fWeaponFatigueBlockMult;
            fatigue.setCurrent(fatigue.getCurrent() - fatigueLoss);
            blockerStats.setFatigue(fatigue);

            blockerStats.setBlock(true);

            if (blocker == getPlayer())
                blocker.getClass().skillUsageSucceeded(blocker, ESM::Skill::Block, ESM::Skill::Block_Success);

            return true;
        }
        return false;
    }

    bool isNormalWeapon(const MWWorld::Ptr& weapon)
    {
        if (weapon.isEmpty())
            return false;

        const int flags = weapon.get<ESM::Weapon>()->mBase->mData.mFlags;
        bool isSilver = flags & ESM::Weapon::Silver;
        bool isMagical = flags & ESM::Weapon::Magical;
        bool isEnchanted = !weapon.getClass().getEnchantment(weapon).empty();

        return !isSilver && !isMagical && (!isEnchanted || !Settings::game().mEnchantedWeaponsAreMagical);
    }

    void resistNormalWeapon(
        const MWWorld::Ptr& actor, const MWWorld::Ptr& attacker, const MWWorld::Ptr& weapon, float& damage)
    {
        if (weapon.isEmpty() || !isNormalWeapon(weapon))
            return;

        const MWMechanics::MagicEffects& effects = actor.getClass().getCreatureStats(actor).getMagicEffects();
        const float resistance = effects.getOrDefault(ESM::MagicEffect::ResistNormalWeapons).getMagnitude() / 100.f;
        const float weakness = effects.getOrDefault(ESM::MagicEffect::WeaknessToNormalWeapons).getMagnitude() / 100.f;

        damage *= 1.f - std::min(1.f, resistance - weakness);

        if (resistance - weakness >= 1.f && attacker == getPlayer())
            MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicTargetResistsWeapons}");
    }

    void applyWerewolfDamageMult(const MWWorld::Ptr& actor, const MWWorld::Ptr& weapon, float& damage)
    {
        if (damage == 0 || weapon.isEmpty() || !actor.getClass().isNpc())
            return;

        const int flags = weapon.get<ESM::Weapon>()->mBase->mData.mFlags;
        bool isSilver = flags & ESM::Weapon::Silver;

        if (isSilver && actor.getClass().getNpcStats(actor).isWerewolf())
        {
            const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
            damage *= store.get<ESM::GameSetting>().find("fWereWolfSilverWeaponDamageMult")->mValue.getFloat();
        }
    }

    void projectileHit(const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim, MWWorld::Ptr weapon,
        const MWWorld::Ptr& projectile, const osg::Vec3f& hitPosition, float attackStrength)
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        const MWWorld::Store<ESM::GameSetting>& gmst = world->getStore().get<ESM::GameSetting>();

        bool validVictim = !victim.isEmpty() && victim.getClass().isActor();

        ESM::RefId weaponSkill = ESM::Skill::Marksman;
        if (!weapon.isEmpty())
            weaponSkill = weapon.getClass().getEquipmentSkill(weapon);

        float damage = 0.f;
        if (validVictim)
        {
            if (attacker == getPlayer())
                MWBase::Environment::get().getWindowManager()->setEnemy(victim);

            int skillValue = attacker.getClass().getSkill(attacker, weaponSkill);

            if (Misc::Rng::roll0to99(world->getPrng()) >= getHitChance(attacker, victim, skillValue))
            {
                MWBase::Environment::get().getLuaManager()->onHit(attacker, victim, weapon, projectile, 0,
                    attackStrength, damage, false, hitPosition, false, MWMechanics::DamageSourceType::Ranged);
                MWMechanics::reduceWeaponCondition(damage, false, weapon, attacker);
                return;
            }

            {
                const auto& attack = weapon.get<ESM::Weapon>()->mBase->mData.mChop;
                damage = attack[0] + ((attack[1] - attack[0]) * attackStrength); // Bow/crossbow damage
            }
            {
                // Arrow/bolt damage
                // NB in case of thrown weapons, we are applying the damage twice since projectile == weapon
                const auto& attack = projectile.get<ESM::Weapon>()->mBase->mData.mChop;
                damage += attack[0] + ((attack[1] - attack[0]) * attackStrength);
            }
            adjustWeaponDamage(damage, weapon, attacker);
        }

        reduceWeaponCondition(damage, validVictim, weapon, attacker);

        if (validVictim)
        {
            if (weapon == projectile || Settings::game().mOnlyAppropriateAmmunitionBypassesResistance
                || isNormalWeapon(weapon))
                resistNormalWeapon(victim, attacker, projectile, damage);
            applyWerewolfDamageMult(victim, projectile, damage);

            if (attacker == getPlayer())
                attacker.getClass().skillUsageSucceeded(attacker, weaponSkill, ESM::Skill::Weapon_SuccessfulHit);

            const MWMechanics::AiSequence& sequence = victim.getClass().getCreatureStats(victim).getAiSequence();
            bool unaware = attacker == getPlayer() && !sequence.isInCombat()
                && !MWBase::Environment::get().getMechanicsManager()->awarenessCheck(attacker, victim);
            bool knockedDown = victim.getClass().getCreatureStats(victim).getKnockedDown();
            if (knockedDown || unaware)
            {
                static const float fCombatKODamageMult = gmst.find("fCombatKODamageMult")->mValue.getFloat();
                damage *= fCombatKODamageMult;
                if (!knockedDown)
                    MWBase::Environment::get().getSoundManager()->playSound3D(
                        victim, ESM::RefId::stringRefId("critical damage"), 1.0f, 1.0f);
            }
        }

        // Apply "On hit" effect of the projectile
        bool appliedEnchantment = applyOnStrikeEnchantment(attacker, victim, projectile, hitPosition, true);

        if (validVictim)
        {
            // Non-enchanted arrows shot at enemies have a chance to turn up in their inventory
            if (victim != getPlayer() && !appliedEnchantment)
            {
                static const float fProjectileThrownStoreChance
                    = gmst.find("fProjectileThrownStoreChance")->mValue.getFloat();
                if (Misc::Rng::rollProbability(world->getPrng()) < fProjectileThrownStoreChance / 100.f)
                    victim.getClass().getContainerStore(victim).add(projectile, 1);
            }

            MWBase::Environment::get().getLuaManager()->onHit(attacker, victim, weapon, projectile, 0, attackStrength,
                damage, true, hitPosition, true, MWMechanics::DamageSourceType::Ranged);
        }
    }

    float getHitChance(const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim, int skillValue)
    {
        MWMechanics::CreatureStats& stats = attacker.getClass().getCreatureStats(attacker);
        const MWMechanics::MagicEffects& mageffects = stats.getMagicEffects();

        MWBase::World* world = MWBase::Environment::get().getWorld();
        const MWWorld::Store<ESM::GameSetting>& gmst = world->getStore().get<ESM::GameSetting>();

        float defenseTerm = 0;
        MWMechanics::CreatureStats& victimStats = victim.getClass().getCreatureStats(victim);
        if (victimStats.getFatigue().getCurrent() >= 0)
        {
            // Maybe we should keep an aware state for actors updated every so often instead of testing every time
            bool unaware = (!victimStats.getAiSequence().isInCombat()) && (attacker == getPlayer())
                && (!MWBase::Environment::get().getMechanicsManager()->awarenessCheck(attacker, victim));
            if (!(victimStats.getKnockedDown() || victimStats.isParalyzed() || unaware))
            {
                defenseTerm = victimStats.getEvasion();
            }
            static const float fCombatInvisoMult = gmst.find("fCombatInvisoMult")->mValue.getFloat();
            defenseTerm += std::min(100.f,
                fCombatInvisoMult
                    * victimStats.getMagicEffects().getOrDefault(ESM::MagicEffect::Chameleon).getMagnitude());
            defenseTerm += std::min(100.f,
                fCombatInvisoMult
                    * victimStats.getMagicEffects().getOrDefault(ESM::MagicEffect::Invisibility).getMagnitude());
        }
        float attackTerm = skillValue + (stats.getAttribute(ESM::Attribute::Agility).getModified() / 5.0f)
            + (stats.getAttribute(ESM::Attribute::Luck).getModified() / 10.0f);
        attackTerm *= stats.getFatigueTerm();
        attackTerm += mageffects.getOrDefault(ESM::MagicEffect::FortifyAttack).getMagnitude()
            - mageffects.getOrDefault(ESM::MagicEffect::Blind).getMagnitude();

        return round(attackTerm - defenseTerm);
    }

    void applyElementalShields(const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim)
    {
        // Don't let elemental shields harm the player in god mode.
        bool godmode = attacker == getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState();
        if (godmode)
            return;
        auto& prng = MWBase::Environment::get().getWorld()->getPrng();
        for (int i = 0; i < 3; ++i)
        {
            float magnitude = victim.getClass()
                                  .getCreatureStats(victim)
                                  .getMagicEffects()
                                  .getOrDefault(ESM::MagicEffect::FireShield + i)
                                  .getMagnitude();

            if (!magnitude)
                continue;

            CreatureStats& attackerStats = attacker.getClass().getCreatureStats(attacker);
            float saveTerm = attacker.getClass().getSkill(attacker, ESM::Skill::Destruction)
                + 0.2f * attackerStats.getAttribute(ESM::Attribute::Willpower).getModified()
                + 0.1f * attackerStats.getAttribute(ESM::Attribute::Luck).getModified();

            float fatigueMax = attackerStats.getFatigue().getModified();
            float fatigueCurrent = attackerStats.getFatigue().getCurrent();

            float normalisedFatigue = floor(fatigueMax) == 0 ? 1 : std::max(0.0f, (fatigueCurrent / fatigueMax));

            saveTerm *= 1.25f * normalisedFatigue;

            float x = std::max(0.f, saveTerm - Misc::Rng::roll0to99(prng));

            int element = ESM::MagicEffect::FireDamage;
            if (i == 1)
                element = ESM::MagicEffect::ShockDamage;
            if (i == 2)
                element = ESM::MagicEffect::FrostDamage;

            float elementResistance
                = MWMechanics::getEffectResistanceAttribute(element, &attackerStats.getMagicEffects());

            x = std::min(100.f, x + elementResistance);

            static const float fElementalShieldMult = MWBase::Environment::get()
                                                          .getESMStore()
                                                          ->get<ESM::GameSetting>()
                                                          .find("fElementalShieldMult")
                                                          ->mValue.getFloat();
            x = fElementalShieldMult * magnitude * (1.f - 0.01f * x);

            // Note swapped victim and attacker, since the attacker takes the damage here.
            x = scaleDamage(x, victim, attacker);

            MWMechanics::DynamicStat<float> health = attackerStats.getHealth();
            health.setCurrent(health.getCurrent() - x);
            attackerStats.setHealth(health);

            MWBase::Environment::get().getSoundManager()->playSound3D(
                attacker, ESM::RefId::stringRefId("Health Damage"), 1.0f, 1.0f);
        }
    }

    void reduceWeaponCondition(float damage, bool hit, MWWorld::Ptr& weapon, const MWWorld::Ptr& attacker)
    {
        if (weapon.isEmpty())
            return;

        if (!hit)
            damage = 0.f;

        const bool weaphashealth = weapon.getClass().hasItemHealth(weapon);
        if (weaphashealth)
        {
            int weaphealth = weapon.getClass().getItemHealth(weapon);

            bool godmode
                = attacker == MWMechanics::getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState();

            // weapon condition does not degrade when godmode is on
            if (!godmode)
            {
                const float fWeaponDamageMult = MWBase::Environment::get()
                                                    .getESMStore()
                                                    ->get<ESM::GameSetting>()
                                                    .find("fWeaponDamageMult")
                                                    ->mValue.getFloat();
                float x = std::max(1.f, fWeaponDamageMult * damage);

                weaphealth -= std::min(int(x), weaphealth);
                weapon.getCellRef().setCharge(weaphealth);
            }

            // Weapon broken? unequip it
            if (weaphealth == 0)
                weapon = *attacker.getClass().getInventoryStore(attacker).unequipItem(weapon);
        }
    }

    void adjustWeaponDamage(float& damage, const MWWorld::Ptr& weapon, const MWWorld::Ptr& attacker)
    {
        if (weapon.isEmpty())
            return;

        const bool weaphashealth = weapon.getClass().hasItemHealth(weapon);
        if (weaphashealth)
        {
            damage *= weapon.getClass().getItemNormalizedHealth(weapon);
        }

        static const float fDamageStrengthBase = MWBase::Environment::get()
                                                     .getESMStore()
                                                     ->get<ESM::GameSetting>()
                                                     .find("fDamageStrengthBase")
                                                     ->mValue.getFloat();
        static const float fDamageStrengthMult = MWBase::Environment::get()
                                                     .getESMStore()
                                                     ->get<ESM::GameSetting>()
                                                     .find("fDamageStrengthMult")
                                                     ->mValue.getFloat();
        damage *= fDamageStrengthBase
            + (attacker.getClass().getCreatureStats(attacker).getAttribute(ESM::Attribute::Strength).getModified()
                * fDamageStrengthMult * 0.1f);
    }

    void getHandToHandDamage(
        const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim, float& damage, bool& healthdmg, float attackStrength)
    {
        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();
        static const float minstrike = store.get<ESM::GameSetting>().find("fMinHandToHandMult")->mValue.getFloat();
        static const float maxstrike = store.get<ESM::GameSetting>().find("fMaxHandToHandMult")->mValue.getFloat();
        damage = static_cast<float>(attacker.getClass().getSkill(attacker, ESM::Skill::HandToHand));
        damage *= minstrike + ((maxstrike - minstrike) * attackStrength);

        MWMechanics::CreatureStats& otherstats = victim.getClass().getCreatureStats(victim);
        healthdmg = otherstats.isParalyzed() || otherstats.getKnockedDown();
        bool isWerewolf = (attacker.getClass().isNpc() && attacker.getClass().getNpcStats(attacker).isWerewolf());

        // Options in the launcher's combo box: unarmedFactorsStrengthComboBox
        // 0 = Do not factor strength into hand-to-hand combat.
        // 1 = Factor into werewolf hand-to-hand combat.
        // 2 = Ignore werewolves.
        const int factorStrength = Settings::game().mStrengthInfluencesHandToHand;
        if (factorStrength == 1 || (factorStrength == 2 && !isWerewolf))
        {
            damage
                *= attacker.getClass().getCreatureStats(attacker).getAttribute(ESM::Attribute::Strength).getModified()
                / 40.0f;
        }

        if (isWerewolf)
        {
            healthdmg = true;
            // GLOB instead of GMST because it gets updated during a quest
            damage *= MWBase::Environment::get().getWorld()->getGlobalFloat(MWWorld::Globals::sWerewolfClawMult);
        }
        if (healthdmg)
        {
            static const float fHandtoHandHealthPer
                = store.get<ESM::GameSetting>().find("fHandtoHandHealthPer")->mValue.getFloat();
            damage *= fHandtoHandHealthPer;
        }

        MWBase::SoundManager* sndMgr = MWBase::Environment::get().getSoundManager();
        if (isWerewolf)
        {
            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
            const ESM::Sound* sound = store.get<ESM::Sound>().searchRandom("WolfHit", prng);
            if (sound)
                sndMgr->playSound3D(victim, sound->mId, 1.0f, 1.0f);
        }
        else if (!healthdmg)
            sndMgr->playSound3D(victim, ESM::RefId::stringRefId("Hand To Hand Hit"), 1.0f, 1.0f);
    }

    void applyFatigueLoss(const MWWorld::Ptr& attacker, const MWWorld::Ptr& weapon, float attackStrength)
    {
        // somewhat of a guess, but using the weapon weight makes sense
        const MWWorld::Store<ESM::GameSetting>& store
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();
        static const float fFatigueAttackBase = store.find("fFatigueAttackBase")->mValue.getFloat();
        static const float fFatigueAttackMult = store.find("fFatigueAttackMult")->mValue.getFloat();
        static const float fWeaponFatigueMult = store.find("fWeaponFatigueMult")->mValue.getFloat();
        CreatureStats& stats = attacker.getClass().getCreatureStats(attacker);
        MWMechanics::DynamicStat<float> fatigue = stats.getFatigue();
        const float normalizedEncumbrance = attacker.getClass().getNormalizedEncumbrance(attacker);

        bool godmode = attacker == MWMechanics::getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState();

        if (!godmode)
        {
            float fatigueLoss = fFatigueAttackBase + normalizedEncumbrance * fFatigueAttackMult;
            if (!weapon.isEmpty())
                fatigueLoss += weapon.getClass().getWeight(weapon) * attackStrength * fWeaponFatigueMult;
            fatigue.setCurrent(fatigue.getCurrent() - fatigueLoss);
            stats.setFatigue(fatigue);
        }
    }

    float getFightDistanceBias(const MWWorld::Ptr& actor1, const MWWorld::Ptr& actor2)
    {
        osg::Vec3f pos1(actor1.getRefData().getPosition().asVec3());
        osg::Vec3f pos2(actor2.getRefData().getPosition().asVec3());

        float d = getAggroDistance(actor1, pos1, pos2);

        static const int iFightDistanceBase = MWBase::Environment::get()
                                                  .getESMStore()
                                                  ->get<ESM::GameSetting>()
                                                  .find("iFightDistanceBase")
                                                  ->mValue.getInteger();
        static const float fFightDistanceMultiplier = MWBase::Environment::get()
                                                          .getESMStore()
                                                          ->get<ESM::GameSetting>()
                                                          .find("fFightDistanceMultiplier")
                                                          ->mValue.getFloat();

        return (iFightDistanceBase - fFightDistanceMultiplier * d);
    }

    float getAggroDistance(const MWWorld::Ptr& actor, const osg::Vec3f& lhs, const osg::Vec3f& rhs)
    {
        if (canActorMoveByZAxis(actor))
            return distanceIgnoreZ(lhs, rhs);
        return distance(lhs, rhs);
    }

    float getDistanceToBounds(const MWWorld::Ptr& actor, const MWWorld::Ptr& target)
    {
        osg::Vec3f actorPos(actor.getRefData().getPosition().asVec3());
        osg::Vec3f targetPos(target.getRefData().getPosition().asVec3());
        MWBase::World* world = MWBase::Environment::get().getWorld();

        float dist = (targetPos - actorPos).length();
        dist -= world->getHalfExtents(actor).y();
        dist -= world->getHalfExtents(target).y();
        return dist;
    }

    float getMeleeWeaponReach(const MWWorld::Ptr& actor, const MWWorld::Ptr& weapon)
    {
        MWBase::World* world = MWBase::Environment::get().getWorld();
        const MWWorld::Store<ESM::GameSetting>& store = world->getStore().get<ESM::GameSetting>();
        const float fCombatDistance = store.find("fCombatDistance")->mValue.getFloat();
        if (!weapon.isEmpty())
            return fCombatDistance * weapon.get<ESM::Weapon>()->mBase->mData.mReach;
        if (actor.getClass().isNpc())
            return fCombatDistance * store.find("fHandToHandReach")->mValue.getFloat();
        return fCombatDistance;
    }

    bool isInMeleeReach(const MWWorld::Ptr& actor, const MWWorld::Ptr& target, const float reach)
    {
        const float heightDiff = actor.getRefData().getPosition().pos[2] - target.getRefData().getPosition().pos[2];
        return std::abs(heightDiff) < reach && getDistanceToBounds(actor, target) < reach;
    }

    std::pair<MWWorld::Ptr, osg::Vec3f> getHitContact(const MWWorld::Ptr& actor, float reach)
    {
        // Lasciate ogne speranza, voi ch'entrate
        MWWorld::Ptr result;
        osg::Vec3f hitPos;
        float minDist = std::numeric_limits<float>::max();
        MWBase::World* world = MWBase::Environment::get().getWorld();
        const MWWorld::Store<ESM::GameSetting>& store = world->getStore().get<ESM::GameSetting>();

        // These GMSTs are not in degrees. They're tolerance angle sines multiplied by 90.
        // With the default values of 60, the actual tolerance angles are roughly 41.8 degrees.
        // Don't think too hard about it. In this place, thinking can cause permanent damage to your mental health.
        const float fCombatAngleXY = store.find("fCombatAngleXY")->mValue.getFloat() / 90.f;
        const float fCombatAngleZ = store.find("fCombatAngleZ")->mValue.getFloat() / 90.f;

        const ESM::Position& posdata = actor.getRefData().getPosition();
        const osg::Vec3f actorPos(posdata.asVec3());
        const osg::Vec3f actorDirXY = osg::Quat(posdata.rot[2], osg::Vec3(0, 0, -1)) * osg::Vec3f(0, 1, 0);
        // Only the player can look up, apparently.
        const float actorVerticalAngle = actor == getPlayer() ? -std::sin(posdata.rot[0]) : 0.f;
        const float actorEyeLevel = world->getHalfExtents(actor, true).z() * 2.f * 0.85f;
        const osg::Vec3f actorEyePos{ actorPos.x(), actorPos.y(), actorPos.z() + actorEyeLevel };
        const bool canMoveByZ = canActorMoveByZAxis(actor);

        // The player can target any active actor, non-playable actors only target their targets
        std::vector<MWWorld::Ptr> targets;
        if (actor != getPlayer())
            actor.getClass().getCreatureStats(actor).getAiSequence().getCombatTargets(targets);
        else
            MWBase::Environment::get().getMechanicsManager()->getActorsInRange(
                actorPos, Settings::game().mActorsProcessingRange, targets);

        for (MWWorld::Ptr& target : targets)
        {
            if (actor == target || target.getClass().getCreatureStats(target).isDead())
                continue;

            const float dist = getDistanceToBounds(actor, target);
            if (dist >= minDist || !isInMeleeReach(actor, target, reach))
                continue;

            const osg::Vec3f targetPos(target.getRefData().getPosition().asVec3());

            // Horizontal angle checks.
            osg::Vec2f actorToTargetXY{ targetPos.x() - actorPos.x(), targetPos.y() - actorPos.y() };
            actorToTargetXY.normalize();

            // Use dot product to check if the target is behind first...
            if (actorToTargetXY.x() * actorDirXY.x() + actorToTargetXY.y() * actorDirXY.y() <= 0.f)
                continue;

            // And then perp dot product to calculate the hit angle sine.
            // This gives us a horizontal hit range of [-asin(fCombatAngleXY / 90); asin(fCombatAngleXY / 90)]
            if (std::abs(actorToTargetXY.x() * actorDirXY.y() - actorToTargetXY.y() * actorDirXY.x()) > fCombatAngleXY)
                continue;

            // Vertical angle checks. Nice cliff racer hack, Todd.
            if (!canMoveByZ)
            {
                // The idea is that the body should always be possible to hit.
                // fCombatAngleZ is the tolerance for hitting the target's feet or head.
                osg::Vec3f actorToTargetFeet = targetPos - actorEyePos;
                osg::Vec3f actorToTargetHead = actorToTargetFeet;
                actorToTargetFeet.normalize();
                actorToTargetHead.z() += world->getHalfExtents(target, true).z() * 2.f;
                actorToTargetHead.normalize();

                if (actorVerticalAngle - actorToTargetHead.z() > fCombatAngleZ
                    || actorVerticalAngle - actorToTargetFeet.z() < -fCombatAngleZ)
                    continue;
            }

            // Gotta use physics somehow!
            if (!world->getLOS(actor, target))
                continue;

            minDist = dist;
            result = target;
        }

        // This hit position is currently used for spawning the blood effect.
        // Morrowind does this elsewhere, but roughly at the same time
        // and it would be hard to track the original hit results outside of this function
        // without code duplication
        // The idea is to use a random point on a plane in front of the target
        // that is defined by its width and height
        if (!result.isEmpty())
        {
            osg::Vec3f resultPos(result.getRefData().getPosition().asVec3());
            osg::Vec3f dirToActor = actorPos - resultPos;
            dirToActor.normalize();

            hitPos = resultPos + dirToActor * world->getHalfExtents(result).y();
            // -25% to 25% of width
            float xOffset = Misc::Rng::deviate(0.f, 0.25f, world->getPrng());
            // 20% to 100% of height
            float zOffset = Misc::Rng::deviate(0.6f, 0.4f, world->getPrng());
            hitPos.x() += world->getHalfExtents(result).x() * 2.f * xOffset;
            hitPos.z() += world->getHalfExtents(result).z() * 2.f * zOffset;
        }

        return std::make_pair(result, hitPos);
    }

    bool friendlyHit(const MWWorld::Ptr& attacker, const MWWorld::Ptr& target, bool complain)
    {
        const MWWorld::Ptr& player = getPlayer();
        if (attacker != player)
            return false;

        std::set<MWWorld::Ptr> followersAttacker;
        MWBase::Environment::get().getMechanicsManager()->getActorsSidingWith(attacker, followersAttacker);
        if (followersAttacker.find(target) == followersAttacker.end())
            return false;

        MWMechanics::CreatureStats& statsTarget = target.getClass().getCreatureStats(target);
        if (statsTarget.getAiSequence().isInCombat())
            return true;
        statsTarget.friendlyHit();
        if (statsTarget.getFriendlyHits() >= 4)
            return false;

        if (complain)
            MWBase::Environment::get().getDialogueManager()->say(target, ESM::RefId::stringRefId("hit"));
        return true;
    }

}
