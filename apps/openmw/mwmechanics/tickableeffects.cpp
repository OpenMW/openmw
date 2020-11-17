#include "tickableeffects.hpp"

#include <components/settings/settings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "actorutil.hpp"
#include "npcstats.hpp"

namespace MWMechanics
{
    void adjustDynamicStat(CreatureStats& creatureStats, int index, float magnitude, bool allowDecreaseBelowZero = false)
    {
        DynamicStat<float> stat = creatureStats.getDynamic(index);
        stat.setCurrent(stat.getCurrent() + magnitude, allowDecreaseBelowZero);
        creatureStats.setDynamic(index, stat);
    }

    bool disintegrateSlot (const MWWorld::Ptr& ptr, int slot, float disintegrate)
    {
        if (!ptr.getClass().hasInventoryStore(ptr))
            return false;

        MWWorld::InventoryStore& inv = ptr.getClass().getInventoryStore(ptr);
        MWWorld::ContainerStoreIterator item = inv.getSlot(slot);

        if (item != inv.end() && (item.getType() == MWWorld::ContainerStore::Type_Armor || item.getType() == MWWorld::ContainerStore::Type_Weapon))
        {
            if (!item->getClass().hasItemHealth(*item))
                return false;
            int charge = item->getClass().getItemHealth(*item);
            if (charge == 0)
                return false;

            // Store remainder of disintegrate amount (automatically subtracted if > 1)
            item->getCellRef().applyChargeRemainderToBeSubtracted(disintegrate - std::floor(disintegrate));

            charge = item->getClass().getItemHealth(*item);
            charge -= std::min(static_cast<int>(disintegrate), charge);
            item->getCellRef().setCharge(charge);

            if (charge == 0)
            {
                // Will unequip the broken item and try to find a replacement
                if (ptr != getPlayer())
                    inv.autoEquip(ptr);
                else
                    inv.unequipItem(*item, ptr);
            }

            return true;
        }

        return false;
    }

    bool effectTick(CreatureStats& creatureStats, const MWWorld::Ptr& actor, const EffectKey &effectKey, float magnitude)
    {
        if (magnitude == 0.f)
            return false;

        bool receivedMagicDamage = false;
        bool godmode = actor == getPlayer() && MWBase::Environment::get().getWorld()->getGodModeState();

        switch (effectKey.mId)
        {
        case ESM::MagicEffect::DamageAttribute:
        {
            if (godmode)
                break;
            AttributeValue attr = creatureStats.getAttribute(effectKey.mArg);
            attr.damage(magnitude);
            creatureStats.setAttribute(effectKey.mArg, attr);
            break;
        }
        case ESM::MagicEffect::RestoreAttribute:
        {
            AttributeValue attr = creatureStats.getAttribute(effectKey.mArg);
            attr.restore(magnitude);
            creatureStats.setAttribute(effectKey.mArg, attr);
            break;
        }
        case ESM::MagicEffect::RestoreHealth:
        case ESM::MagicEffect::RestoreMagicka:
        case ESM::MagicEffect::RestoreFatigue:
            adjustDynamicStat(creatureStats, effectKey.mId-ESM::MagicEffect::RestoreHealth, magnitude);
            break;
        case ESM::MagicEffect::DamageHealth:
            if (godmode)
                break;
            receivedMagicDamage = true;
            adjustDynamicStat(creatureStats, effectKey.mId-ESM::MagicEffect::DamageHealth, -magnitude);
            break;

        case ESM::MagicEffect::DamageMagicka:
        case ESM::MagicEffect::DamageFatigue:
        {
            if (godmode)
                break;
            int index = effectKey.mId-ESM::MagicEffect::DamageHealth;
            static const bool uncappedDamageFatigue = Settings::Manager::getBool("uncapped damage fatigue", "Game");
            adjustDynamicStat(creatureStats, index, -magnitude, index == 2 && uncappedDamageFatigue);
            break;
        }
        case ESM::MagicEffect::AbsorbHealth:
            if (!godmode || magnitude <= 0)
            {
                if (magnitude > 0.f)
                    receivedMagicDamage = true;
                adjustDynamicStat(creatureStats, effectKey.mId-ESM::MagicEffect::AbsorbHealth, -magnitude);
            }
            break;

        case ESM::MagicEffect::AbsorbMagicka:
        case ESM::MagicEffect::AbsorbFatigue:
            if (!godmode || magnitude <= 0)
                adjustDynamicStat(creatureStats, effectKey.mId-ESM::MagicEffect::AbsorbHealth, -magnitude);
            break;

        case ESM::MagicEffect::DisintegrateArmor:
        {
            if (godmode)
                break;
            static const std::array<int, 9>  priorities
            {
                MWWorld::InventoryStore::Slot_CarriedLeft,
                MWWorld::InventoryStore::Slot_Cuirass,
                MWWorld::InventoryStore::Slot_LeftPauldron,
                MWWorld::InventoryStore::Slot_RightPauldron,
                MWWorld::InventoryStore::Slot_LeftGauntlet,
                MWWorld::InventoryStore::Slot_RightGauntlet,
                MWWorld::InventoryStore::Slot_Helmet,
                MWWorld::InventoryStore::Slot_Greaves,
                MWWorld::InventoryStore::Slot_Boots
            };
            for (const int priority : priorities)
            {
                if (disintegrateSlot(actor, priority, magnitude))
                    break;
            }

            break;
        }
        case ESM::MagicEffect::DisintegrateWeapon:
            if (!godmode)
                disintegrateSlot(actor, MWWorld::InventoryStore::Slot_CarriedRight, magnitude);
            break;

        case ESM::MagicEffect::SunDamage:
        {
            // isInCell shouldn't be needed, but updateActor called during game start
            if (!actor.isInCell() || !actor.getCell()->isExterior() || godmode)
                break;
            float time = MWBase::Environment::get().getWorld()->getTimeStamp().getHour();
            float timeDiff = std::min(7.f, std::max(0.f, std::abs(time - 13)));
            float damageScale = 1.f - timeDiff / 7.f;
            // When cloudy, the sun damage effect is halved
            static float fMagicSunBlockedMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                        "fMagicSunBlockedMult")->mValue.getFloat();

            int weather = MWBase::Environment::get().getWorld()->getCurrentWeather();
            if (weather > 1)
                damageScale *= fMagicSunBlockedMult;

            adjustDynamicStat(creatureStats, 0, -magnitude * damageScale);
            if (magnitude * damageScale > 0.f)
                receivedMagicDamage = true;

            break;
        }

        case ESM::MagicEffect::FireDamage:
        case ESM::MagicEffect::ShockDamage:
        case ESM::MagicEffect::FrostDamage:
        case ESM::MagicEffect::Poison:
        {
            if (godmode)
                break;
            adjustDynamicStat(creatureStats, 0, -magnitude);
            receivedMagicDamage = true;
            break;
        }

        case ESM::MagicEffect::DamageSkill:
        case ESM::MagicEffect::RestoreSkill:
        {
            if (!actor.getClass().isNpc())
                break;
            if (godmode && effectKey.mId == ESM::MagicEffect::DamageSkill)
                break;
            NpcStats &npcStats = actor.getClass().getNpcStats(actor);
            SkillValue& skill = npcStats.getSkill(effectKey.mArg);
            if (effectKey.mId == ESM::MagicEffect::RestoreSkill)
                skill.restore(magnitude);
            else
                skill.damage(magnitude);
            break;
        }

        default:
            return false;
        }

        if (receivedMagicDamage && actor == getPlayer())
            MWBase::Environment::get().getWindowManager()->activateHitOverlay(false);
        return true;
    }
}
