#include "aicombataction.hpp"

#include <components/esm/loadench.hpp>
#include <components/esm/loadmgef.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/actionequip.hpp"
#include "../mwworld/cellstore.hpp"

#include "npcstats.hpp"
#include "spellcasting.hpp"
#include "combat.hpp"
#include "weaponpriority.hpp"
#include "spellpriority.hpp"
#include "weapontype.hpp"

namespace MWMechanics
{
    float suggestCombatRange(int rangeTypes)
    {
        static const float fCombatDistance = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fCombatDistance")->mValue.getFloat();
        static float fHandToHandReach = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fHandToHandReach")->mValue.getFloat();

        // This distance is a possible distance of melee attack
        static float distance = fCombatDistance * std::max(2.f, fHandToHandReach);

        if (rangeTypes & RangeTypes::Touch)
        {
            return fCombatDistance;
        }

        return distance * 4;
    }

    void ActionSpell::prepare(const MWWorld::Ptr &actor)
    {
        actor.getClass().getCreatureStats(actor).getSpells().setSelectedSpell(mSpellId);
        actor.getClass().getCreatureStats(actor).setDrawState(DrawState_Spell);
        if (actor.getClass().hasInventoryStore(actor))
        {
            MWWorld::InventoryStore& inv = actor.getClass().getInventoryStore(actor);
            inv.setSelectedEnchantItem(inv.end());
        }

        const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(mSpellId);
        MWBase::Environment::get().getWorld()->preloadEffects(&spell->mEffects);
    }

    float ActionSpell::getCombatRange (bool& isRanged) const
    {
        const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(mSpellId);
        int types = getRangeTypes(spell->mEffects);

        isRanged = (types & RangeTypes::Target) | (types & RangeTypes::Self);
        return suggestCombatRange(types);
    }

    void ActionEnchantedItem::prepare(const MWWorld::Ptr &actor)
    {
        actor.getClass().getCreatureStats(actor).getSpells().setSelectedSpell(std::string());
        actor.getClass().getInventoryStore(actor).setSelectedEnchantItem(mItem);
        actor.getClass().getCreatureStats(actor).setDrawState(DrawState_Spell);
    }

    float ActionEnchantedItem::getCombatRange(bool& isRanged) const
    {
        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(mItem->getClass().getEnchantment(*mItem));
        int types = getRangeTypes(enchantment->mEffects);

        isRanged = (types & RangeTypes::Target) | (types & RangeTypes::Self);
        return suggestCombatRange(types);
    }

    float ActionPotion::getCombatRange(bool& isRanged) const
    {
        // Distance doesn't matter since this action has no animation
        // If we want to back away slightly to avoid enemy hits, we should set isRanged to "true"
        return 600.f;
    }

    void ActionPotion::prepare(const MWWorld::Ptr &actor)
    {
        actor.getClass().apply(actor, mPotion.getCellRef().getRefId(), actor);
        actor.getClass().getContainerStore(actor).remove(mPotion, 1, actor);
    }

    void ActionWeapon::prepare(const MWWorld::Ptr &actor)
    {
        if (actor.getClass().hasInventoryStore(actor))
        {
            if (mWeapon.isEmpty())
                actor.getClass().getInventoryStore(actor).unequipSlot(MWWorld::InventoryStore::Slot_CarriedRight, actor);
            else
            {
                MWWorld::ActionEquip equip(mWeapon);
                equip.execute(actor);
            }

            if (!mAmmunition.isEmpty())
            {
                MWWorld::ActionEquip equip(mAmmunition);
                equip.execute(actor);
            }
        }
        actor.getClass().getCreatureStats(actor).setDrawState(DrawState_Weapon);
    }

    float ActionWeapon::getCombatRange(bool& isRanged) const
    {
        isRanged = false;

        static const float fCombatDistance = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fCombatDistance")->mValue.getFloat();
        static const float fProjectileMaxSpeed = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fProjectileMaxSpeed")->mValue.getFloat();

        if (mWeapon.isEmpty())
        {
            static float fHandToHandReach =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fHandToHandReach")->mValue.getFloat();
            return fHandToHandReach * fCombatDistance;
        }

        const ESM::Weapon* weapon = mWeapon.get<ESM::Weapon>()->mBase;
        if (MWMechanics::getWeaponType(weapon->mData.mType)->mWeaponClass != ESM::WeaponType::Melee)
        {
            isRanged = true;
            return fProjectileMaxSpeed;
        }
        else
            return weapon->mData.mReach * fCombatDistance;
    }

    const ESM::Weapon* ActionWeapon::getWeapon() const
    {
        if (mWeapon.isEmpty())
            return nullptr;
        return mWeapon.get<ESM::Weapon>()->mBase;
    }

    std::shared_ptr<Action> prepareNextAction(const MWWorld::Ptr &actor, const MWWorld::Ptr &enemy)
    {
        Spells& spells = actor.getClass().getCreatureStats(actor).getSpells();

        float bestActionRating = 0.f;
        float antiFleeRating = 0.f;
        // Default to hand-to-hand combat
        std::shared_ptr<Action> bestAction (new ActionWeapon(MWWorld::Ptr()));
        if (actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
        {
            bestAction->prepare(actor);
            return bestAction;
        }

        if (actor.getClass().hasInventoryStore(actor))
        {
            MWWorld::InventoryStore& store = actor.getClass().getInventoryStore(actor);

            for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
            {
                float rating = ratePotion(*it, actor);
                if (rating > bestActionRating)
                {
                    bestActionRating = rating;
                    bestAction.reset(new ActionPotion(*it));
                    antiFleeRating = std::numeric_limits<float>::max();
                }
            }

            for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
            {
                float rating = rateMagicItem(*it, actor, enemy);
                if (rating > bestActionRating)
                {
                    bestActionRating = rating;
                    bestAction.reset(new ActionEnchantedItem(it));
                    antiFleeRating = std::numeric_limits<float>::max();
                }
            }

            MWWorld::Ptr bestArrow;
            float bestArrowRating = rateAmmo(actor, enemy, bestArrow, ESM::Weapon::Arrow);

            MWWorld::Ptr bestBolt;
            float bestBoltRating = rateAmmo(actor, enemy, bestBolt, ESM::Weapon::Bolt);

            for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
            {
                float rating = rateWeapon(*it, actor, enemy, -1, bestArrowRating, bestBoltRating);
                if (rating > bestActionRating)
                {
                    const ESM::Weapon* weapon = it->get<ESM::Weapon>()->mBase;
                    int ammotype = getWeaponType(weapon->mData.mType)->mAmmoType;

                    MWWorld::Ptr ammo;
                    if (ammotype == ESM::Weapon::Arrow)
                        ammo = bestArrow;
                    else if (ammotype == ESM::Weapon::Bolt)
                        ammo = bestBolt;

                    bestActionRating = rating;
                    bestAction.reset(new ActionWeapon(*it, ammo));
                    antiFleeRating = vanillaRateWeaponAndAmmo(*it, ammo, actor, enemy);
                }
            }
        }

        for (Spells::TIterator it = spells.begin(); it != spells.end(); ++it)
        {
            float rating = rateSpell(it->first, actor, enemy);
            if (rating > bestActionRating)
            {
                bestActionRating = rating;
                bestAction.reset(new ActionSpell(it->first->mId));
                antiFleeRating = vanillaRateSpell(it->first, actor, enemy);
            }
        }

        if (makeFleeDecision(actor, enemy, antiFleeRating))
            bestAction.reset(new ActionFlee());

        if (bestAction.get())
            bestAction->prepare(actor);

        return bestAction;
    }

    float getBestActionRating(const MWWorld::Ptr &actor, const MWWorld::Ptr &enemy)
    {
        Spells& spells = actor.getClass().getCreatureStats(actor).getSpells();

        float bestActionRating = 0.f;
        // Default to hand-to-hand combat
        if (actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
        {
            return bestActionRating;
        }

        if (actor.getClass().hasInventoryStore(actor))
        {
            MWWorld::InventoryStore& store = actor.getClass().getInventoryStore(actor);

            for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
            {
                float rating = rateMagicItem(*it, actor, enemy);
                if (rating > bestActionRating)
                {
                    bestActionRating = rating;
                }
            }

            float bestArrowRating = rateAmmo(actor, enemy, ESM::Weapon::Arrow);

            float bestBoltRating = rateAmmo(actor, enemy, ESM::Weapon::Bolt);

            for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
            {
                float rating = rateWeapon(*it, actor, enemy, -1, bestArrowRating, bestBoltRating);
                if (rating > bestActionRating)
                {
                    bestActionRating = rating;
                }
            }
        }

        for (Spells::TIterator it = spells.begin(); it != spells.end(); ++it)
        {
            float rating = rateSpell(it->first, actor, enemy);
            if (rating > bestActionRating)
            {
                bestActionRating = rating;
            }
        }

        return bestActionRating;
    }


    float getDistanceMinusHalfExtents(const MWWorld::Ptr& actor1, const MWWorld::Ptr& actor2, bool minusZDist)
    {
        osg::Vec3f actor1Pos = actor1.getRefData().getPosition().asVec3();
        osg::Vec3f actor2Pos = actor2.getRefData().getPosition().asVec3();

        float dist = (actor1Pos - actor2Pos).length();

        if (minusZDist)
            dist -= std::abs(actor1Pos.z() - actor2Pos.z());

        return (dist
                - MWBase::Environment::get().getWorld()->getHalfExtents(actor1).y()
                - MWBase::Environment::get().getWorld()->getHalfExtents(actor2).y());
    }

    float getMaxAttackDistance(const MWWorld::Ptr& actor)
    {
        const CreatureStats& stats = actor.getClass().getCreatureStats(actor);
        const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        std::string selectedSpellId = stats.getSpells().getSelectedSpell();
        MWWorld::Ptr selectedEnchItem;

        MWWorld::Ptr activeWeapon, activeAmmo;
        if (actor.getClass().hasInventoryStore(actor))
        {
            MWWorld::InventoryStore& invStore = actor.getClass().getInventoryStore(actor);

            MWWorld::ContainerStoreIterator item = invStore.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
            if (item != invStore.end() && item.getType() == MWWorld::ContainerStore::Type_Weapon)
                activeWeapon = *item;

            item = invStore.getSlot(MWWorld::InventoryStore::Slot_Ammunition);
            if (item != invStore.end() && item.getType() == MWWorld::ContainerStore::Type_Weapon)
                activeAmmo = *item;

            if (invStore.getSelectedEnchantItem() != invStore.end())
                selectedEnchItem = *invStore.getSelectedEnchantItem();
        }

        float dist = 1.0f;
        if (activeWeapon.isEmpty() && !selectedSpellId.empty() && !selectedEnchItem.isEmpty())
        {
            static const float fHandToHandReach = gmst.find("fHandToHandReach")->mValue.getFloat();
            dist = fHandToHandReach;
        }
        else if (stats.getDrawState() == MWMechanics::DrawState_Spell)
        {
            dist = 1.0f;
            if (!selectedSpellId.empty())
            {
                const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(selectedSpellId);
                for (std::vector<ESM::ENAMstruct>::const_iterator effectIt =
                     spell->mEffects.mList.begin(); effectIt != spell->mEffects.mList.end(); ++effectIt)
                {
                    if (effectIt->mRange == ESM::RT_Target)
                    {
                        const ESM::MagicEffect* effect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effectIt->mEffectID);
                        dist = effect->mData.mSpeed;
                        break;
                    }
                }
            }
            else if (!selectedEnchItem.isEmpty())
            {
                std::string enchId = selectedEnchItem.getClass().getEnchantment(selectedEnchItem);
                if (!enchId.empty())
                {
                    const ESM::Enchantment* ench = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(enchId);
                    for (std::vector<ESM::ENAMstruct>::const_iterator effectIt =
                         ench->mEffects.mList.begin(); effectIt != ench->mEffects.mList.end(); ++effectIt)
                    {
                        if (effectIt->mRange == ESM::RT_Target)
                        {
                            const ESM::MagicEffect* effect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effectIt->mEffectID);
                            dist = effect->mData.mSpeed;
                            break;
                        }
                    }
                }
            }

            static const float fTargetSpellMaxSpeed = gmst.find("fTargetSpellMaxSpeed")->mValue.getFloat();
            dist *= std::max(1000.0f, fTargetSpellMaxSpeed);
        }
        else if (!activeWeapon.isEmpty())
        {
            const ESM::Weapon* esmWeap = activeWeapon.get<ESM::Weapon>()->mBase;
            if (MWMechanics::getWeaponType(esmWeap->mData.mType)->mWeaponClass != ESM::WeaponType::Melee)
            {
                static const float fTargetSpellMaxSpeed = gmst.find("fProjectileMaxSpeed")->mValue.getFloat();
                dist = fTargetSpellMaxSpeed;
                if (!activeAmmo.isEmpty())
                {
                    const ESM::Weapon* esmAmmo = activeAmmo.get<ESM::Weapon>()->mBase;
                    dist *= esmAmmo->mData.mSpeed;
                }
            }
            else if (esmWeap->mData.mReach > 1)
            {
                dist = esmWeap->mData.mReach;
            }
        }

        dist = (dist > 0.f) ? dist : 1.0f;

        static const float fCombatDistance = gmst.find("fCombatDistance")->mValue.getFloat();
        static const float fCombatDistanceWerewolfMod = gmst.find("fCombatDistanceWerewolfMod")->mValue.getFloat();

        float combatDistance = fCombatDistance;
        if (actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
            combatDistance *= (fCombatDistanceWerewolfMod + 1.0f);

        if (dist < combatDistance)
            dist *= combatDistance;

        return dist;
    }

    bool canFight(const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy)
    {
        ESM::Position actorPos = actor.getRefData().getPosition();
        ESM::Position enemyPos = enemy.getRefData().getPosition();

        if (isTargetMagicallyHidden(enemy) && !MWBase::Environment::get().getMechanicsManager()->awarenessCheck(enemy, actor))
        {
            return false;
        }

        if (actor.getClass().isPureWaterCreature(actor))
        {
            if (!MWBase::Environment::get().getWorld()->isWading(enemy))
                return false;
        }

        float atDist = getMaxAttackDistance(actor);
        if (atDist > getDistanceMinusHalfExtents(actor, enemy)
                && atDist > std::abs(actorPos.pos[2] - enemyPos.pos[2]))
        {
            if (MWBase::Environment::get().getWorld()->getLOS(actor, enemy))
                return true;
        }

        if (actor.getClass().isPureLandCreature(actor) && MWBase::Environment::get().getWorld()->isWalkingOnWater(enemy))
        {
            return false;
        }

        if (actor.getClass().isPureFlyingCreature(actor) || actor.getClass().isPureLandCreature(actor))
        {
            if (MWBase::Environment::get().getWorld()->isSwimming(enemy))
                return false;
        }

        if (actor.getClass().isBipedal(actor) || !actor.getClass().canFly(actor))
        {
            if (enemy.getClass().getCreatureStats(enemy).getMagicEffects().get(ESM::MagicEffect::Levitate).getMagnitude() > 0)
            {
                float attackDistance = getMaxAttackDistance(actor);
                if ((attackDistance + actorPos.pos[2]) < enemyPos.pos[2])
                {
                    if (enemy.getCell()->isExterior())
                    {
                        if (attackDistance < (enemyPos.pos[2] - MWBase::Environment::get().getWorld()->getTerrainHeightAt(enemyPos.asVec3())))
                            return false;
                    }
                }
            }
        }

        if (!actor.getClass().canWalk(actor) && !actor.getClass().isBipedal(actor))
            return true;

        if (actor.getClass().getCreatureStats(actor).getMagicEffects().get(ESM::MagicEffect::Levitate).getMagnitude() > 0)
            return true;

        if (MWBase::Environment::get().getWorld()->isSwimming(actor))
            return true;

        if (getDistanceMinusHalfExtents(actor, enemy, true) <= 0.0f)
            return false;

        return true;
    }

    float vanillaRateFlee(const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy)
    {
        const CreatureStats& stats = actor.getClass().getCreatureStats(actor);
        const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        int flee = stats.getAiSetting(CreatureStats::AI_Flee).getModified();
        if (flee >= 100)
            return flee;

        static const float fAIFleeHealthMult = gmst.find("fAIFleeHealthMult")->mValue.getFloat();
        static const float fAIFleeFleeMult = gmst.find("fAIFleeFleeMult")->mValue.getFloat();

        float healthPercentage = (stats.getHealth().getModified() == 0.0f)
                                    ? 1.0f : stats.getHealth().getCurrent() / stats.getHealth().getModified();
        float rating = (1.0f - healthPercentage) * fAIFleeHealthMult + flee * fAIFleeFleeMult;

        static const int iWereWolfLevelToAttack = gmst.find("iWereWolfLevelToAttack")->mValue.getInteger();

        if (actor.getClass().isNpc() && enemy.getClass().isNpc())
        {
            if (enemy.getClass().getNpcStats(enemy).isWerewolf() && stats.getLevel() < iWereWolfLevelToAttack)
            {
                static const int iWereWolfFleeMod = gmst.find("iWereWolfFleeMod")->mValue.getInteger();
                rating = iWereWolfFleeMod;
            }
        }

        if (rating != 0.0f)
            rating += getFightDistanceBias(actor, enemy);

        return rating;
    }

    bool makeFleeDecision(const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy, float antiFleeRating)
    {
        float fleeRating = vanillaRateFlee(actor, enemy);
        if (fleeRating < 100.0f)
            fleeRating = 0.0f;

        if (fleeRating > antiFleeRating)
            return true;

        // Run away after summoning a creature if we have nothing to use but fists.
        if (antiFleeRating == 0.0f && !actor.getClass().getCreatureStats(actor).getSummonedCreatureMap().empty())
            return true;

        return false;
    }
}
