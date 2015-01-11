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
                || blockerStats.getMagicEffects().get(ESM::MagicEffect::Paralyze).getMagnitude() > 0)
            return false;

        if (!MWBase::Environment::get().getMechanicsManager()->isReadyToBlock(blocker))
            return false;

        MWWorld::InventoryStore& inv = blocker.getClass().getInventoryStore(blocker);
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

        float attackerSkill = 0.f;
        if (weapon.isEmpty())
            attackerSkill = attacker.getClass().getSkill(attacker, ESM::Skill::HandToHand);
        else
            attackerSkill = attacker.getClass().getSkill(attacker, weapon.getClass().getEquipmentSkill(weapon));
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
            float normalizedEncumbrance = blocker.getClass().getNormalizedEncumbrance(blocker);
            normalizedEncumbrance = std::min(1.f, normalizedEncumbrance);
            float fatigueLoss = fFatigueBlockBase + normalizedEncumbrance * fFatigueBlockMult;
            if (!weapon.isEmpty())
                fatigueLoss += weapon.getClass().getWeight(weapon) * attackerStats.getAttackStrength() * fWeaponFatigueBlockMult;
            fatigue.setCurrent(fatigue.getCurrent() - fatigueLoss);
            blockerStats.setFatigue(fatigue);

            blockerStats.setBlock(true);

            if (blocker.getCellRef().getRefId() == "player")
                blocker.getClass().skillUsageSucceeded(blocker, ESM::Skill::Block, 0);

            return true;
        }
        return false;
    }

    void resistNormalWeapon(const MWWorld::Ptr &actor, const MWWorld::Ptr& attacker, const MWWorld::Ptr &weapon, float &damage)
    {
        MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
        float resistance = std::min(100.f, stats.getMagicEffects().get(ESM::MagicEffect::ResistNormalWeapons).getMagnitude()
                - stats.getMagicEffects().get(ESM::MagicEffect::WeaknessToNormalWeapons).getMagnitude());

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

        if(victim.isEmpty() || !victim.getClass().isActor() || victim.getClass().getCreatureStats(victim).isDead())
            // Can't hit non-actors or dead actors
        {
            reduceWeaponCondition(0.f, false, weapon, attacker);
            return;
        }

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
            MWMechanics::reduceWeaponCondition(0.f, false, weapon, attacker);
            return;
        }

        float fDamageStrengthBase = gmst.find("fDamageStrengthBase")->getFloat();
        float fDamageStrengthMult = gmst.find("fDamageStrengthMult")->getFloat();

        const unsigned char* attack = weapon.get<ESM::Weapon>()->mBase->mData.mChop;
        float damage = attack[0] + ((attack[1]-attack[0])*attackerStats.getAttackStrength()); // Bow/crossbow damage
        if (weapon != projectile)
        {
            // Arrow/bolt damage
            attack = projectile.get<ESM::Weapon>()->mBase->mData.mChop;
            damage += attack[0] + ((attack[1]-attack[0])*attackerStats.getAttackStrength());
        }

        damage *= fDamageStrengthBase +
                (attackerStats.getAttribute(ESM::Attribute::Strength).getModified() * fDamageStrengthMult * 0.1);

        adjustWeaponDamage(damage, weapon);
        reduceWeaponCondition(damage, true, weapon, attacker);

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
        hitchance += mageffects.get(ESM::MagicEffect::FortifyAttack).getMagnitude() -
                     mageffects.get(ESM::MagicEffect::Blind).getMagnitude();
        hitchance -= victim.getClass().getCreatureStats(victim).getEvasion();
        return hitchance;
    }

    void applyElementalShields(const MWWorld::Ptr &attacker, const MWWorld::Ptr &victim)
    {
        for (int i=0; i<3; ++i)
        {
            float magnitude = victim.getClass().getCreatureStats(victim).getMagicEffects().get(ESM::MagicEffect::FireShield+i).getMagnitude();

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

    void reduceWeaponCondition(float damage, bool hit, MWWorld::Ptr &weapon, const MWWorld::Ptr &attacker)
    {
        if (weapon.isEmpty())
            return;

        if (!hit)
            damage = 0.f;

        const bool weaphashealth = weapon.getClass().hasItemHealth(weapon);
        if(weaphashealth)
        {
            int weaphealth = weapon.getClass().getItemHealth(weapon);

            const float fWeaponDamageMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fWeaponDamageMult")->getFloat();
            float x = std::max(1.f, fWeaponDamageMult * damage);

            weaphealth -= std::min(int(x), weaphealth);
            weapon.getCellRef().setCharge(weaphealth);

            // Weapon broken? unequip it
            if (weaphealth == 0)
                weapon = *attacker.getClass().getInventoryStore(attacker).unequipItem(weapon, attacker);
        }
    }

    void adjustWeaponDamage(float &damage, const MWWorld::Ptr &weapon)
    {
        if (weapon.isEmpty())
            return;

        const bool weaphashealth = weapon.getClass().hasItemHealth(weapon);
        if(weaphashealth)
        {
            int weaphealth = weapon.getClass().getItemHealth(weapon);
            int weapmaxhealth = weapon.getClass().getItemMaxHealth(weapon);
            damage *= (float(weaphealth) / weapmaxhealth);
        }
    }

    void getHandToHandDamage(const MWWorld::Ptr &attacker, const MWWorld::Ptr &victim, float &damage, bool &healthdmg)
    {
        // Note: MCP contains an option to include Strength in hand-to-hand damage
        // calculations. Some mods recommend using it, so we may want to include an
        // option for it.
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        float minstrike = store.get<ESM::GameSetting>().find("fMinHandToHandMult")->getFloat();
        float maxstrike = store.get<ESM::GameSetting>().find("fMaxHandToHandMult")->getFloat();
        damage  = attacker.getClass().getSkill(attacker, ESM::Skill::HandToHand);
        damage *= minstrike + ((maxstrike-minstrike)*attacker.getClass().getCreatureStats(attacker).getAttackStrength());

        MWMechanics::CreatureStats& otherstats = victim.getClass().getCreatureStats(victim);
        healthdmg = (otherstats.getMagicEffects().get(ESM::MagicEffect::Paralyze).getMagnitude() > 0)
                || otherstats.getKnockedDown();
        bool isWerewolf = (attacker.getClass().isNpc() && attacker.getClass().getNpcStats(attacker).isWerewolf());
        if(isWerewolf)
        {
            healthdmg = true;
            // GLOB instead of GMST because it gets updated during a quest
            damage *= MWBase::Environment::get().getWorld()->getGlobalFloat("werewolfclawmult");
        }
        if(healthdmg)
            damage *= store.get<ESM::GameSetting>().find("fHandtoHandHealthPer")->getFloat();

        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        if(isWerewolf)
        {
            const ESM::Sound *sound = store.get<ESM::Sound>().searchRandom("WolfHit");
            if(sound)
                sndMgr->playSound3D(victim, sound->mId, 1.0f, 1.0f);
        }
        else
            sndMgr->playSound3D(victim, "Hand To Hand Hit", 1.0f, 1.0f);
    }

    bool isEnvironmentCompatible(const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim)
    {
        const MWWorld::Class& attackerClass = attacker.getClass();
        MWBase::World* world = MWBase::Environment::get().getWorld();

        // If attacker is fish, victim must be in water
        if (attackerClass.isPureWaterCreature(attacker))
        {
            return world->isWading(victim);
        }
        
        // If attacker can't swim, victim must not be in water
        if (!attackerClass.canSwim(attacker))
        {
            return !world->isSwimming(victim);
        }

        return true;
    }
}
