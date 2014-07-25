#include "combat.hpp"

#include <OgreSceneNode.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/difficultyscaling.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwbase/windowmanager.hpp"

namespace
{

Ogre::Radian signedAngle(Ogre::Vector3 v1, Ogre::Vector3 v2, Ogre::Vector3 normal)
{
    return Ogre::Math::ATan2(
                normal.dotProduct( v1.crossProduct(v2) ),
                v1.dotProduct(v2)
                );
}

void applyEnchantment (const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim, const MWWorld::Ptr& object, const Ogre::Vector3& hitPosition)
{
    std::string enchantmentName = !object.isEmpty() ? object.getClass().getEnchantment(object) : "";
    if (!enchantmentName.empty())
    {
        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(
                    enchantmentName);
        if (enchantment->mData.mType == ESM::Enchantment::WhenStrikes)
        {
            MWMechanics::CastSpell cast(attacker, victim);
            cast.mHitPosition = hitPosition;
            cast.cast(object);
        }
    }
}

}

namespace MWMechanics
{

    bool blockMeleeAttack(const MWWorld::Ptr &attacker, const MWWorld::Ptr &blocker, const MWWorld::Ptr &weapon, float damage)
    {
        if (!blocker.getClass().hasInventoryStore(blocker))
            return false;

        MWMechanics::CreatureStats& blockerStats = blocker.getClass().getCreatureStats(blocker);

        if (blockerStats.getKnockedDown() // Used for both knockout or knockdown
                || blockerStats.getHitRecovery()
                || blockerStats.getMagicEffects().get(ESM::MagicEffect::Paralyze).mMagnitude > 0)
            return false;

        // Don't block when in spellcasting state (shield is equipped, but not visible)
        if (blockerStats.getDrawState() == DrawState_Spell)
            return false;

        MWWorld::InventoryStore& inv = blocker.getClass().getInventoryStore(blocker);

        // Don't block when in hand-to-hand combat (shield is equipped, but not visible)
        if (blockerStats.getDrawState() == DrawState_Weapon &&
                inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight) == inv.end())
            return false;

        MWWorld::ContainerStoreIterator shield = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        if (shield == inv.end() || shield->getTypeName() != typeid(ESM::Armor).name())
            return false;

        Ogre::Degree angle = signedAngle (Ogre::Vector3(attacker.getRefData().getPosition().pos) - Ogre::Vector3(blocker.getRefData().getPosition().pos),
                                          blocker.getRefData().getBaseNode()->getOrientation().yAxis(), Ogre::Vector3(0,0,1));

        const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
        if (angle.valueDegrees() < gmst.find("fCombatBlockLeftAngle")->getFloat())
            return false;
        if (angle.valueDegrees() > gmst.find("fCombatBlockRightAngle")->getFloat())
            return false;

        MWMechanics::CreatureStats& attackerStats = attacker.getClass().getCreatureStats(attacker);

        float blockTerm = blocker.getClass().getSkill(blocker, ESM::Skill::Block) + 0.2 * blockerStats.getAttribute(ESM::Attribute::Agility).getModified()
            + 0.1 * blockerStats.getAttribute(ESM::Attribute::Luck).getModified();
        float enemySwing = attackerStats.getAttackStrength();
        float swingTerm = enemySwing * gmst.find("fSwingBlockMult")->getFloat() + gmst.find("fSwingBlockBase")->getFloat();

        float blockerTerm = blockTerm * swingTerm;
        if (blocker.getClass().getMovementSettings(blocker).mPosition[1] <= 0)
            blockerTerm *= gmst.find("fBlockStillBonus")->getFloat();
        blockerTerm *= blockerStats.getFatigueTerm();

        float attackerSkill = attacker.getClass().getSkill(attacker, weapon.getClass().getEquipmentSkill(weapon));
        float attackerTerm = attackerSkill + 0.2 * attackerStats.getAttribute(ESM::Attribute::Agility).getModified()
                + 0.1 * attackerStats.getAttribute(ESM::Attribute::Luck).getModified();
        attackerTerm *= attackerStats.getFatigueTerm();

        int x = int(blockerTerm - attackerTerm);
        int iBlockMaxChance = gmst.find("iBlockMaxChance")->getInt();
        int iBlockMinChance = gmst.find("iBlockMinChance")->getInt();
        x = std::min(iBlockMaxChance, std::max(iBlockMinChance, x));

        int roll = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 100; // [0, 99]
        if (roll < x)
        {
            // Reduce shield durability by incoming damage
            int shieldhealth = shield->getClass().getItemHealth(*shield);

            shieldhealth -= std::min(shieldhealth, int(damage));
            shield->getCellRef().setCharge(shieldhealth);
            if (shieldhealth == 0)
                inv.unequipItem(*shield, blocker);

            // Reduce blocker fatigue
            const float fFatigueBlockBase = gmst.find("fFatigueBlockBase")->getFloat();
            const float fFatigueBlockMult = gmst.find("fFatigueBlockMult")->getFloat();
            const float fWeaponFatigueBlockMult = gmst.find("fWeaponFatigueBlockMult")->getFloat();
            MWMechanics::DynamicStat<float> fatigue = blockerStats.getFatigue();
            float normalizedEncumbrance = blocker.getClass().getEncumbrance(blocker) / blocker.getClass().getCapacity(blocker);
            normalizedEncumbrance = std::min(1.f, normalizedEncumbrance);
            float fatigueLoss = fFatigueBlockBase + normalizedEncumbrance * fFatigueBlockMult;
            fatigueLoss += weapon.getClass().getWeight(weapon) * attackerStats.getAttackStrength() * fWeaponFatigueBlockMult;
            fatigue.setCurrent(fatigue.getCurrent() - fatigueLoss);
            blockerStats.setFatigue(fatigue);

            blockerStats.setBlock(true);

            if (blocker.getClass().isNpc())
                blocker.getClass().skillUsageSucceeded(blocker, ESM::Skill::Block, 0);

            return true;
        }
        return false;
    }

    void resistNormalWeapon(const MWWorld::Ptr &actor, const MWWorld::Ptr& attacker, const MWWorld::Ptr &weapon, float &damage)
    {
        MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
        float resistance = std::min(100.f, stats.getMagicEffects().get(ESM::MagicEffect::ResistNormalWeapons).mMagnitude
                - stats.getMagicEffects().get(ESM::MagicEffect::WeaknessToNormalWeapons).mMagnitude);

        float multiplier = 1.f - resistance / 100.f;

        if (!(weapon.get<ESM::Weapon>()->mBase->mData.mFlags & ESM::Weapon::Silver
              || weapon.get<ESM::Weapon>()->mBase->mData.mFlags & ESM::Weapon::Magical))
            damage *= multiplier;

        if ((weapon.get<ESM::Weapon>()->mBase->mData.mFlags & ESM::Weapon::Silver)
                && actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
            damage *= MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fWereWolfSilverWeaponDamageMult")->getFloat();

        if (damage == 0 && attacker.getRefData().getHandle() == "player")
            MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicTargetResistsWeapons}");
    }

    void projectileHit(const MWWorld::Ptr &attacker, const MWWorld::Ptr &victim, MWWorld::Ptr weapon, const MWWorld::Ptr &projectile,
                       const Ogre::Vector3& hitPosition)
    {
        MWBase::World *world = MWBase::Environment::get().getWorld();
        const MWWorld::Store<ESM::GameSetting> &gmst = world->getStore().get<ESM::GameSetting>();

        MWMechanics::CreatureStats& attackerStats = attacker.getClass().getCreatureStats(attacker);

        const MWWorld::Class &othercls = victim.getClass();
        if(!othercls.isActor()) // Can't hit non-actors
            return;
        MWMechanics::CreatureStats &otherstats = victim.getClass().getCreatureStats(victim);
        if(otherstats.isDead()) // Can't hit dead actors
            return;

        if(attacker.getRefData().getHandle() == "player")
            MWBase::Environment::get().getWindowManager()->setEnemy(victim);

        int weapskill = ESM::Skill::Marksman;
        if(!weapon.isEmpty())
            weapskill = weapon.getClass().getEquipmentSkill(weapon);

        float skillValue = attacker.getClass().getSkill(attacker,
                                           weapon.getClass().getEquipmentSkill(weapon));

        if((::rand()/(RAND_MAX+1.0)) > getHitChance(attacker, victim, skillValue)/100.0f)
        {
            victim.getClass().onHit(victim, 0.0f, false, projectile, attacker, false);
            return;
        }

        float damage = 0.0f;

        float fDamageStrengthBase = gmst.find("fDamageStrengthBase")->getFloat();
        float fDamageStrengthMult = gmst.find("fDamageStrengthMult")->getFloat();

        const unsigned char* attack = weapon.get<ESM::Weapon>()->mBase->mData.mChop;
        damage = attack[0] + ((attack[1]-attack[0])*attackerStats.getAttackStrength()); // Bow/crossbow damage
        if (weapon != projectile)
        {
            // Arrow/bolt damage
            attack = projectile.get<ESM::Weapon>()->mBase->mData.mChop;
            damage += attack[0] + ((attack[1]-attack[0])*attackerStats.getAttackStrength());
        }

        damage *= fDamageStrengthBase +
                (attackerStats.getAttribute(ESM::Attribute::Strength).getModified() * fDamageStrengthMult * 0.1);


        if(attacker.getRefData().getHandle() == "player")
            attacker.getClass().skillUsageSucceeded(attacker, weapskill, 0);

        if (victim.getClass().getCreatureStats(victim).getKnockedDown())
            damage *= gmst.find("fCombatKODamageMult")->getFloat();

        // Apply "On hit" effect of the weapon
        applyEnchantment(attacker, victim, weapon, hitPosition);
        if (weapon != projectile)
            applyEnchantment(attacker, victim, projectile, hitPosition);

        if (damage > 0)
            MWBase::Environment::get().getWorld()->spawnBloodEffect(victim, hitPosition);

        // Arrows shot at enemies have a chance to turn up in their inventory
        if (victim != MWBase::Environment::get().getWorld()->getPlayerPtr())
        {
            float fProjectileThrownStoreChance = gmst.find("fProjectileThrownStoreChance")->getFloat();
            if ((::rand()/(RAND_MAX+1.0)) < fProjectileThrownStoreChance/100.f)
                victim.getClass().getContainerStore(victim).add(projectile, 1, victim);
        }

        victim.getClass().onHit(victim, damage, true, projectile, attacker, true);
    }

    float getHitChance(const MWWorld::Ptr &attacker, const MWWorld::Ptr &victim, int skillValue)
    {
        MWMechanics::CreatureStats &stats = attacker.getClass().getCreatureStats(attacker);
        const MWMechanics::MagicEffects &mageffects = stats.getMagicEffects();
        float hitchance = skillValue +
                          (stats.getAttribute(ESM::Attribute::Agility).getModified() / 5.0f) +
                          (stats.getAttribute(ESM::Attribute::Luck).getModified() / 10.0f);
        hitchance *= stats.getFatigueTerm();
        hitchance += mageffects.get(ESM::MagicEffect::FortifyAttack).mMagnitude -
                     mageffects.get(ESM::MagicEffect::Blind).mMagnitude;
        hitchance -= victim.getClass().getCreatureStats(victim).getEvasion();
        return hitchance;
    }

    void applyElementalShields(const MWWorld::Ptr &attacker, const MWWorld::Ptr &victim)
    {
        for (int i=0; i<3; ++i)
        {
            float magnitude = victim.getClass().getCreatureStats(victim).getMagicEffects().get(ESM::MagicEffect::FireShield+i).mMagnitude;

            if (!magnitude)
                continue;

            CreatureStats& attackerStats = attacker.getClass().getCreatureStats(attacker);
            float saveTerm = attacker.getClass().getSkill(attacker, ESM::Skill::Destruction)
                    + 0.2f * attackerStats.getAttribute(ESM::Attribute::Willpower).getModified()
                    + 0.1f * attackerStats.getAttribute(ESM::Attribute::Luck).getModified();

            int fatigueMax = attackerStats.getFatigue().getModified();
            int fatigueCurrent = attackerStats.getFatigue().getCurrent();

            float normalisedFatigue = fatigueMax==0 ? 1 : std::max (0.0f, static_cast<float> (fatigueCurrent)/fatigueMax);

            saveTerm *= 1.25f * normalisedFatigue;

            float roll = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 100; // [0, 99]
            float x = std::max(0.f, saveTerm - roll);

            int element = ESM::MagicEffect::FireDamage;
            if (i == 1)
                element = ESM::MagicEffect::ShockDamage;
            if (i == 2)
                element = ESM::MagicEffect::FrostDamage;

            float elementResistance = MWMechanics::getEffectResistanceAttribute(element, &attackerStats.getMagicEffects());

            x = std::min(100.f, x + elementResistance);

            static const float fElementalShieldMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fElementalShieldMult")->getFloat();
            x = fElementalShieldMult * magnitude * (1.f - 0.01f * x);

            // Note swapped victim and attacker, since the attacker takes the damage here.
            x = scaleDamage(x, victim, attacker);

            MWMechanics::DynamicStat<float> health = attackerStats.getHealth();
            health.setCurrent(health.getCurrent() - x);
            attackerStats.setHealth(health);
        }
    }

}
