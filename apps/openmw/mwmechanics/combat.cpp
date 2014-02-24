#include "combat.hpp"

#include <OgreSceneNode.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/movement.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwbase/windowmanager.hpp"

namespace
{

Ogre::Radian signedAngle(Ogre::Vector3 v1, Ogre::Vector3 v2, Ogre::Vector3 n)
{
    return Ogre::Math::ATan2(
                n.dotProduct( v1.crossProduct(v2) ),
                v1.dotProduct(v2)
                );
}

}

namespace MWMechanics
{

    bool blockMeleeAttack(const MWWorld::Ptr &attacker, const MWWorld::Ptr &blocker, const MWWorld::Ptr &weapon, float damage)
    {
        if (!blocker.getClass().hasInventoryStore(blocker))
            return false;

        if (blocker.getClass().getCreatureStats(blocker).getKnockedDown()
                || blocker.getClass().getCreatureStats(blocker).getHitRecovery())
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

        MWMechanics::CreatureStats& blockerStats = blocker.getClass().getCreatureStats(blocker);
        if (blockerStats.getDrawState() == DrawState_Spell)
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
            if (shield->getCellRef().mCharge == -1)
                shield->getCellRef().mCharge = shield->getClass().getItemMaxHealth(*shield);
            shield->getCellRef().mCharge -= std::min(shield->getCellRef().mCharge, int(damage));
            if (!shield->getCellRef().mCharge)
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

        float multiplier = 0;
        if (resistance >= 0)
            multiplier = 1 - resistance / 100.f;
        else
            multiplier = -(resistance-100) / 100.f;

        if (!(weapon.get<ESM::Weapon>()->mBase->mData.mFlags & ESM::Weapon::Silver
              || weapon.get<ESM::Weapon>()->mBase->mData.mFlags & ESM::Weapon::Magical))
            damage *= multiplier;

        if (weapon.get<ESM::Weapon>()->mBase->mData.mFlags & ESM::Weapon::Silver
                & actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
            damage *= MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fWereWolfSilverWeaponDamageMult")->getFloat();

        if (damage == 0 && attacker.getRefData().getHandle() == "player")
            MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicTargetResistsWeapons}");
    }

}
