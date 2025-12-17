#include "spelleffects.hpp"

#include <algorithm>
#include <array>

#include <components/esm3/loadcrea.hpp>
#include <components/esm3/loadench.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadstat.hpp>
#include <components/misc/resourcehelpers.hpp>
#include <components/misc/rng.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/aifollow.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/spellresistance.hpp"
#include "../mwmechanics/spellutil.hpp"
#include "../mwmechanics/summoning.hpp"

#include "../mwrender/animation.hpp"

#include "../mwworld/actionequip.hpp"
#include "../mwworld/actionteleport.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/worldmodel.hpp"

namespace
{
    enum Stats
    {
        Health = 0,
        Magicka = 1,
        Fatigue = 2
    };

    float roll(const ESM::ActiveEffect& effect)
    {
        if (effect.mMinMagnitude == effect.mMaxMagnitude)
            return effect.mMinMagnitude;
        auto& prng = MWBase::Environment::get().getWorld()->getPrng();
        return effect.mMinMagnitude
            + Misc::Rng::rollDice(static_cast<int>(effect.mMaxMagnitude - effect.mMinMagnitude + 1), prng);
    }

    ESM::ActiveEffect::Flags modifyAiSetting(const MWWorld::Ptr& target, const ESM::ActiveEffect& effect,
        const ESM::RefId& creatureEffect, MWMechanics::AiSetting setting, float magnitude)
    {
        if (target == MWMechanics::getPlayer() || (effect.mEffectId == creatureEffect) == target.getClass().isNpc())
            return ESM::ActiveEffect::Flag_Invalid;
        auto& creatureStats = target.getClass().getCreatureStats(target);
        auto stat = creatureStats.getAiSetting(setting);
        stat.setModifier(static_cast<int>(stat.getModifier() + magnitude));
        creatureStats.setAiSetting(setting, stat);
        return ESM::ActiveEffect::Flag_Applied;
    }

    void adjustDynamicStat(const MWWorld::Ptr& target, int index, float magnitude, bool allowDecreaseBelowZero = false,
        bool allowIncreaseAboveModified = false)
    {
        auto& creatureStats = target.getClass().getCreatureStats(target);
        auto stat = creatureStats.getDynamic(index);
        stat.setCurrent(stat.getCurrent() + magnitude, allowDecreaseBelowZero, allowIncreaseAboveModified);
        creatureStats.setDynamic(index, stat);
    }

    void modDynamicStat(const MWWorld::Ptr& target, int index, float magnitude)
    {
        auto& creatureStats = target.getClass().getCreatureStats(target);
        auto stat = creatureStats.getDynamic(index);
        float current = stat.getCurrent();
        stat.setBase(std::max(0.f, stat.getBase() + magnitude));
        stat.setCurrent(current + magnitude);
        creatureStats.setDynamic(index, stat);
    }

    void damageAttribute(const MWWorld::Ptr& target, const ESM::ActiveEffect& effect, float magnitude)
    {
        auto& creatureStats = target.getClass().getCreatureStats(target);
        auto attribute = effect.getSkillOrAttribute();
        auto attr = creatureStats.getAttribute(attribute);
        if (effect.mEffectId == ESM::MagicEffect::DamageAttribute)
            magnitude = std::min(attr.getModified(), magnitude);
        attr.damage(magnitude);
        creatureStats.setAttribute(attribute, attr);
    }

    void restoreAttribute(const MWWorld::Ptr& target, const ESM::ActiveEffect& effect, float magnitude)
    {
        auto& creatureStats = target.getClass().getCreatureStats(target);
        auto attribute = effect.getSkillOrAttribute();
        auto attr = creatureStats.getAttribute(attribute);
        attr.restore(magnitude);
        creatureStats.setAttribute(attribute, attr);
    }

    void fortifyAttribute(const MWWorld::Ptr& target, const ESM::ActiveEffect& effect, float magnitude)
    {
        auto& creatureStats = target.getClass().getCreatureStats(target);
        auto attribute = effect.getSkillOrAttribute();
        auto attr = creatureStats.getAttribute(attribute);
        attr.setModifier(attr.getModifier() + magnitude);
        creatureStats.setAttribute(attribute, attr);
    }

    void damageSkill(const MWWorld::Ptr& target, const ESM::ActiveEffect& effect, float magnitude)
    {
        auto& npcStats = target.getClass().getNpcStats(target);
        auto& skill = npcStats.getSkill(effect.getSkillOrAttribute());
        if (effect.mEffectId == ESM::MagicEffect::DamageSkill)
            magnitude = std::min(skill.getModified(), magnitude);
        skill.damage(magnitude);
    }

    void restoreSkill(const MWWorld::Ptr& target, const ESM::ActiveEffect& effect, float magnitude)
    {
        auto& npcStats = target.getClass().getNpcStats(target);
        auto& skill = npcStats.getSkill(effect.getSkillOrAttribute());
        skill.restore(magnitude);
    }

    void fortifySkill(const MWWorld::Ptr& target, const ESM::ActiveEffect& effect, float magnitude)
    {
        auto& npcStats = target.getClass().getNpcStats(target);
        auto& skill = npcStats.getSkill(effect.getSkillOrAttribute());
        skill.setModifier(skill.getModifier() + magnitude);
    }

    bool disintegrateSlot(const MWWorld::Ptr& ptr, int slot, float disintegrate)
    {
        MWWorld::InventoryStore& inv = ptr.getClass().getInventoryStore(ptr);
        MWWorld::ContainerStoreIterator item = inv.getSlot(slot);

        if (item != inv.end()
            && (item.getType() == MWWorld::ContainerStore::Type_Armor
                || item.getType() == MWWorld::ContainerStore::Type_Weapon))
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
                if (ptr != MWMechanics::getPlayer())
                    inv.autoEquip();
                else
                    inv.unequipItem(*item);
            }

            return true;
        }

        return false;
    }

    int getBoundItemSlot(const MWWorld::Ptr& boundPtr)
    {
        const auto [slots, _] = boundPtr.getClass().getEquipmentSlots(boundPtr);
        if (!slots.empty())
            return slots[0];
        return -1;
    }

    bool addBoundItem(const ESM::RefId& itemId, const MWWorld::Ptr& actor)
    {
        MWWorld::InventoryStore& store = actor.getClass().getInventoryStore(actor);
        MWWorld::Ptr boundPtr = *store.MWWorld::ContainerStore::add(itemId, 1);

        int slot = getBoundItemSlot(boundPtr);
        auto prevItem = slot >= 0 ? store.getSlot(slot) : store.end();

        MWWorld::ActionEquip action(boundPtr);
        action.execute(actor);

        MWWorld::Ptr newItem;
        auto it = slot >= 0 ? store.getSlot(slot) : store.end();
        // Equip can fail because beast races cannot equip boots/helmets
        if (it != store.end())
            newItem = *it;

        if (newItem.isEmpty() || boundPtr != newItem)
            return false;

        if (actor == MWMechanics::getPlayer())
        {
            MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();

            // change draw state only if the item is in player's right hand
            if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
                player.setDrawState(MWMechanics::DrawState::Weapon);

            if (prevItem != store.end())
                player.setPreviousItem(itemId, prevItem->getCellRef().getRefId());
        }

        return true;
    }

    void removeBoundItem(const ESM::RefId& itemId, const MWWorld::Ptr& actor)
    {
        MWWorld::InventoryStore& store = actor.getClass().getInventoryStore(actor);
        auto item = std::find_if(
            store.begin(), store.end(), [&](const auto& it) { return it.getCellRef().getRefId() == itemId; });
        if (item == store.end())
            return;
        int slot = getBoundItemSlot(*item);

        auto currentItem = store.getSlot(slot);

        bool wasEquipped = currentItem != store.end() && currentItem->getCellRef().getRefId() == itemId;

        if (wasEquipped)
            store.remove(*currentItem, 1);
        else
            store.remove(itemId, 1);

        if (actor != MWMechanics::getPlayer())
        {
            // Equip a replacement
            if (!wasEquipped)
                return;

            auto type = currentItem->getType();
            if (type != ESM::Weapon::sRecordId && type != ESM::Armor::sRecordId && type != ESM::Clothing::sRecordId)
                return;

            if (actor.getClass().getCreatureStats(actor).isDead())
                return;

            if (!actor.getClass().hasInventoryStore(actor))
                return;

            if (actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf())
                return;

            actor.getClass().getInventoryStore(actor).autoEquip();

            return;
        }

        MWWorld::Player& player = MWBase::Environment::get().getWorld()->getPlayer();
        ESM::RefId prevItemId = player.getPreviousItem(itemId);
        player.erasePreviousItem(itemId);

        if (!prevItemId.empty() && wasEquipped)
        {
            // Find previous item (or its replacement) by id.
            // we should equip previous item only if expired bound item was equipped.
            MWWorld::Ptr prevItem = store.findReplacement(prevItemId);
            if (!prevItem.isEmpty())
            {
                MWWorld::ActionEquip action(prevItem);
                action.execute(actor);
            }
        }
    }

    bool isCorprusEffect(const MWMechanics::ActiveSpells::ActiveEffect& effect, bool harmfulOnly = false)
    {
        if (effect.mFlags & ESM::ActiveEffect::Flag_Applied && effect.mEffectId != ESM::MagicEffect::Corprus)
        {
            const auto* magicEffect
                = MWBase::Environment::get().getESMStore()->get<ESM::MagicEffect>().find(effect.mEffectId);
            if (magicEffect->mData.mFlags & ESM::MagicEffect::Flags::AppliedOnce
                && (!harmfulOnly || magicEffect->mData.mFlags & ESM::MagicEffect::Flags::Harmful))
                return true;
        }
        return false;
    }

    void absorbSpell(const MWMechanics::ActiveSpells::ActiveSpellParams& spellParams, const MWWorld::Ptr& caster,
        const MWWorld::Ptr& target)
    {
        const auto& esmStore = *MWBase::Environment::get().getESMStore();
        const ESM::Static* absorbStatic = esmStore.get<ESM::Static>().find(ESM::RefId::stringRefId("VFX_Absorb"));
        MWRender::Animation* animation = MWBase::Environment::get().getWorld()->getAnimation(target);
        if (animation && !absorbStatic->mModel.empty())
        {
            const VFS::Path::Normalized absorbStaticModel
                = Misc::ResourceHelpers::correctMeshPath(VFS::Path::Normalized(absorbStatic->mModel));
            animation->addEffect(
                absorbStaticModel.value(), ESM::MagicEffect::refIdToName(ESM::MagicEffect::SpellAbsorption), false);
        }

        int spellCost = 0;
        if (const ESM::Spell* spell = esmStore.get<ESM::Spell>().search(spellParams.getSourceSpellId()))
        {
            spellCost = MWMechanics::calcSpellCost(*spell);
        }
        else
        {
            const ESM::Enchantment* enchantment = esmStore.get<ESM::Enchantment>().search(spellParams.getEnchantment());
            if (enchantment)
                spellCost = MWMechanics::getEffectiveEnchantmentCastCost(*enchantment, caster);
        }

        // Magicka is increased by the cost of the spell
        auto& stats = target.getClass().getCreatureStats(target);
        auto magicka = stats.getMagicka();
        magicka.setCurrent(magicka.getCurrent() + spellCost);
        stats.setMagicka(magicka);
    }

    MWMechanics::MagicApplicationResult::Type applyProtections(const MWWorld::Ptr& target, const MWWorld::Ptr& caster,
        const MWMechanics::ActiveSpells::ActiveSpellParams& spellParams, ESM::ActiveEffect& effect,
        const ESM::MagicEffect* magicEffect)
    {
        auto& stats = target.getClass().getCreatureStats(target);
        auto& magnitudes = stats.getMagicEffects();
        // Apply reflect and spell absorption
        if (target != caster && spellParams.hasFlag(ESM::ActiveSpells::Flag_Temporary))
        {
            bool canReflect = !(magicEffect->mData.mFlags & ESM::MagicEffect::Unreflectable)
                && !(effect.mFlags & ESM::ActiveEffect::Flag_Ignore_Reflect)
                && magnitudes.getOrDefault(ESM::MagicEffect::Reflect).getMagnitude() > 0.f && !caster.isEmpty();
            bool canAbsorb = !(effect.mFlags & ESM::ActiveEffect::Flag_Ignore_SpellAbsorption)
                && magnitudes.getOrDefault(ESM::MagicEffect::SpellAbsorption).getMagnitude() > 0.f;
            if (canReflect || canAbsorb)
            {
                auto& prng = MWBase::Environment::get().getWorld()->getPrng();
                for (const auto& activeParam : stats.getActiveSpells())
                {
                    for (const auto& activeEffect : activeParam.getEffects())
                    {
                        if (!(activeEffect.mFlags & ESM::ActiveEffect::Flag_Applied))
                            continue;
                        if (activeEffect.mEffectId == ESM::MagicEffect::Reflect)
                        {
                            if (canReflect && Misc::Rng::roll0to99(prng) < activeEffect.mMagnitude)
                            {
                                return MWMechanics::MagicApplicationResult::Type::REFLECTED;
                            }
                        }
                        else if (activeEffect.mEffectId == ESM::MagicEffect::SpellAbsorption)
                        {
                            if (canAbsorb && Misc::Rng::roll0to99(prng) < activeEffect.mMagnitude)
                            {
                                absorbSpell(spellParams, caster, target);
                                return MWMechanics::MagicApplicationResult::Type::REMOVED;
                            }
                        }
                    }
                }
            }
        }
        // Notify the target actor they've been hit
        bool isHarmful = magicEffect->mData.mFlags & ESM::MagicEffect::Harmful;
        if (target.getClass().isActor() && target != caster && !caster.isEmpty() && isHarmful)
            target.getClass().onHit(target, {}, {}, caster, true, MWMechanics::DamageSourceType::Magical);
        // Apply resistances
        if (!(effect.mFlags & ESM::ActiveEffect::Flag_Ignore_Resistances))
        {
            const ESM::Spell* spell
                = spellParams.hasFlag(ESM::ActiveSpells::Flag_Temporary) ? spellParams.getSpell() : nullptr;
            float magnitudeMult = MWMechanics::getEffectMultiplier(effect.mEffectId, target, caster, spell, &magnitudes);
            if (magnitudeMult == 0)
            {
                // Fully resisted, show message
                if (target == MWMechanics::getPlayer())
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicPCResisted}");
                else if (caster == MWMechanics::getPlayer())
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicTargetResisted}");
                return MWMechanics::MagicApplicationResult::Type::REMOVED;
            }
            else if (!(magicEffect->mData.mFlags & ESM::MagicEffect::NoMagnitude))
            {
                effect.mMinMagnitude *= magnitudeMult;
                effect.mMaxMagnitude *= magnitudeMult;
            }
        }
        return MWMechanics::MagicApplicationResult::Type::APPLIED;
    }

    const std::map<ESM::RefId, std::string>& getBoundItemsMap() {
        static const std::map<ESM::RefId, std::string> sBoundItemsMap{
            { ESM::MagicEffect::BoundBattleAxe, "sMagicBoundBattleAxeID" },
            { ESM::MagicEffect::BoundBoots, "sMagicBoundBootsID" },
            { ESM::MagicEffect::BoundCuirass, "sMagicBoundCuirassID" },
            { ESM::MagicEffect::BoundDagger, "sMagicBoundDaggerID" },
            { ESM::MagicEffect::BoundGloves, "sMagicBoundLeftGauntletID" },
            { ESM::MagicEffect::BoundHelm, "sMagicBoundHelmID" },
            { ESM::MagicEffect::BoundLongbow, "sMagicBoundLongbowID" },
            { ESM::MagicEffect::BoundLongsword, "sMagicBoundLongswordID" },
            { ESM::MagicEffect::BoundMace, "sMagicBoundMaceID" },
            { ESM::MagicEffect::BoundShield, "sMagicBoundShieldID" },
            { ESM::MagicEffect::BoundSpear, "sMagicBoundSpearID" },
        };
        return sBoundItemsMap;
    }

    using SpellsPurge = void (MWMechanics::Spells::*)();
    void purgePermanent(const MWWorld::Ptr& target, SpellsPurge method, ESM::Spell::SpellType type)
    {
        MWMechanics::CreatureStats& stats = target.getClass().getCreatureStats(target);
        (stats.getSpells().*method)();
        stats.getActiveSpells().purge(
            [type](const MWMechanics::ActiveSpells::ActiveSpellParams& params) {
                if (params.hasFlag(ESM::ActiveSpells::Flag_SpellStore))
                {
                    const ESM::Spell* spell = params.getSpell();
                    return spell && spell->mData.mType == type;
                }
                return false;
            },
            target);
    }
}

namespace MWMechanics
{

    ESM::ActiveEffect::Flags applyMagicEffect(const MWWorld::Ptr& target, const MWWorld::Ptr& caster,
        const ActiveSpells::ActiveSpellParams& spellParams, ESM::ActiveEffect& effect, bool& receivedMagicDamage,
        bool& affectedHealth, bool& recalculateMagicka)
    {
        const auto world = MWBase::Environment::get().getWorld();
        const bool godmode = target == getPlayer() && world->getGodModeState();
        if (effect.mEffectId == ESM::MagicEffect::CureCommonDisease)
            purgePermanent(target, &Spells::purgeCommonDisease, ESM::Spell::ST_Disease);
        else if (effect.mEffectId == ESM::MagicEffect::CureBlightDisease)
            purgePermanent(target, &Spells::purgeBlightDisease, ESM::Spell::ST_Blight);
        else if (effect.mEffectId == ESM::MagicEffect::RemoveCurse)
            purgePermanent(target, &Spells::purgeCurses, ESM::Spell::ST_Curse);
        else if (effect.mEffectId == ESM::MagicEffect::CureCorprusDisease)
            target.getClass().getCreatureStats(target).getActiveSpells().purgeEffect(
                target, ESM::MagicEffect::Corprus);
        else if (effect.mEffectId == ESM::MagicEffect::CurePoison)
            target.getClass().getCreatureStats(target).getActiveSpells().purgeEffect(
                target, ESM::MagicEffect::Poison);
        else if (effect.mEffectId == ESM::MagicEffect::CureParalyzation)
            target.getClass().getCreatureStats(target).getActiveSpells().purgeEffect(
                target, ESM::MagicEffect::Paralyze);
        else if (effect.mEffectId == ESM::MagicEffect::Dispel)
            // Dispel removes entire spells at once
            target.getClass().getCreatureStats(target).getActiveSpells().purge(
                [magnitude = effect.mMagnitude](const ActiveSpells::ActiveSpellParams& params) {
                    if (params.hasFlag(ESM::ActiveSpells::Flag_Temporary))
                    {
                        const ESM::Spell* spell = params.getSpell();
                        if (spell && spell->mData.mType == ESM::Spell::ST_Spell)
                        {
                            auto& prng = MWBase::Environment::get().getWorld()->getPrng();
                            return Misc::Rng::roll0to99(prng) < magnitude;
                        }
                    }
                    return false;
                },
                target);
        else if (effect.mEffectId == ESM::MagicEffect::AlmsiviIntervention
            || effect.mEffectId == ESM::MagicEffect::DivineIntervention)
        {
            if (target != getPlayer())
                return ESM::ActiveEffect::Flag_Invalid;
            else if (world->isTeleportingEnabled())
            {
                std::string_view marker
                    = (effect.mEffectId == ESM::MagicEffect::DivineIntervention) ? "divinemarker" : "templemarker";
                world->teleportToClosestMarker(target, ESM::RefId::stringRefId(marker));
                if (!caster.isEmpty())
                {
                    MWRender::Animation* anim = world->getAnimation(caster);
                    anim->removeEffect(ESM::MagicEffect::refIdToName(effect.mEffectId));
                    const ESM::Static* fx
                        = world->getStore().get<ESM::Static>().search(ESM::RefId::stringRefId("VFX_Summon_end"));
                    if (fx != nullptr)
                    {
                        const VFS::Path::Normalized fxModel
                            = Misc::ResourceHelpers::correctMeshPath(VFS::Path::Normalized(fx->mModel));
                        anim->addEffect(fxModel.value(), "");
                    }
                }
            }
            else if (caster == getPlayer())
                MWBase::Environment::get().getWindowManager()->messageBox("#{sTeleportDisabled}");
        }
        else if (effect.mEffectId == ESM::MagicEffect::Mark)
        {
            if (target != getPlayer())
                return ESM::ActiveEffect::Flag_Invalid;
            else if (world->isTeleportingEnabled())
                world->getPlayer().markPosition(target.getCell(), target.getRefData().getPosition());
            else if (caster == getPlayer())
                MWBase::Environment::get().getWindowManager()->messageBox("#{sTeleportDisabled}");
        }
        else if (effect.mEffectId == ESM::MagicEffect::Recall)
        {
            if (target != getPlayer())
                return ESM::ActiveEffect::Flag_Invalid;
            else if (world->isTeleportingEnabled())
            {
                MWWorld::CellStore* markedCell = nullptr;
                ESM::Position markedPosition;

                world->getPlayer().getMarkedPosition(markedCell, markedPosition);
                if (markedCell)
                {
                    ESM::RefId dest = markedCell->getCell()->getId();
                    MWWorld::ActionTeleport action(dest, markedPosition, false);
                    action.execute(target);
                    if (!caster.isEmpty())
                    {
                        MWRender::Animation* anim = world->getAnimation(caster);
                        anim->removeEffect(ESM::MagicEffect::refIdToName(effect.mEffectId));
                    }
                }
            }
            else if (caster == getPlayer())
                MWBase::Environment::get().getWindowManager()->messageBox("#{sTeleportDisabled}");
        }
        else if (effect.mEffectId == ESM::MagicEffect::CommandCreature
            || effect.mEffectId == ESM::MagicEffect::CommandHumanoid)
        {
            if (caster.isEmpty() || !caster.getClass().isActor() || target == getPlayer()
                || (effect.mEffectId == ESM::MagicEffect::CommandCreature) == target.getClass().isNpc())
                return ESM::ActiveEffect::Flag_Invalid;
            else if (effect.mMagnitude >= target.getClass().getCreatureStats(target).getLevel())
            {
                MWMechanics::AiFollow package(caster, true);
                target.getClass().getCreatureStats(target).getAiSequence().stack(package, target);
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::ExtraSpell)
        {
            if (!target.getClass().hasInventoryStore(target))
                return ESM::ActiveEffect::Flag_Invalid;
            if (target != getPlayer())
            {
                auto& store = target.getClass().getInventoryStore(target);
                for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
                {
                    // Unequip everything except weapons, torches, and pants
                    switch (slot)
                    {
                        case MWWorld::InventoryStore::Slot_Ammunition:
                        case MWWorld::InventoryStore::Slot_CarriedRight:
                        case MWWorld::InventoryStore::Slot_Pants:
                            continue;
                        case MWWorld::InventoryStore::Slot_CarriedLeft:
                        {
                            auto carried = store.getSlot(slot);
                            if (carried == store.end() || carried.getType() != MWWorld::ContainerStore::Type_Armor)
                                continue;
                            [[fallthrough]];
                        }
                        default:
                            store.unequipSlot(slot);
                    }
                }
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::TurnUndead)
        {
            if (target.getClass().isNpc() || target.get<ESM::Creature>()->mBase->mData.mType != ESM::Creature::Undead)
                return ESM::ActiveEffect::Flag_Invalid;
            else
            {
                auto& creatureStats = target.getClass().getCreatureStats(target);
                Stat<int> stat = creatureStats.getAiSetting(AiSetting::Flee);
                stat.setModifier(static_cast<int>(stat.getModifier() + effect.mMagnitude));
                creatureStats.setAiSetting(AiSetting::Flee, stat);
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::FrenzyCreature || effect.mEffectId == ESM::MagicEffect::FrenzyHumanoid)
            return modifyAiSetting(
                target, effect, ESM::MagicEffect::FrenzyCreature, AiSetting::Fight, effect.mMagnitude);
        else if (effect.mEffectId == ESM::MagicEffect::CalmCreature || effect.mEffectId == ESM::MagicEffect::CalmHumanoid)
        {
            ESM::ActiveEffect::Flags applied = modifyAiSetting(
                target, effect, ESM::MagicEffect::CalmCreature, AiSetting::Fight, -effect.mMagnitude);
            if (applied != ESM::ActiveEffect::Flag_Applied)
                return applied;
            if (effect.mMagnitude > 0)
            {
                auto& creatureStats = target.getClass().getCreatureStats(target);
                creatureStats.getAiSequence().stopCombat();
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::DemoralizeCreature || effect.mEffectId == ESM::MagicEffect::DemoralizeHumanoid)
            return modifyAiSetting(
                target, effect, ESM::MagicEffect::DemoralizeCreature, AiSetting::Flee, effect.mMagnitude);
        else if (effect.mEffectId == ESM::MagicEffect::RallyCreature || effect.mEffectId == ESM::MagicEffect::RallyHumanoid)
            return modifyAiSetting(
                target, effect, ESM::MagicEffect::RallyCreature, AiSetting::Flee, -effect.mMagnitude);
        else if (effect.mEffectId == ESM::MagicEffect::Charm)
        {
            if (!target.getClass().isNpc())
                return ESM::ActiveEffect::Flag_Invalid;
        }
        else if (effect.mEffectId == ESM::MagicEffect::Sound)
        {
            if (target == getPlayer())
            {
                const auto& magnitudes = target.getClass().getCreatureStats(target).getMagicEffects();
                float volume = std::clamp(
                    (magnitudes.getOrDefault(effect.mEffectId).getModifier() + effect.mMagnitude) / 100.f, 0.f, 1.f);
                MWBase::Environment::get().getSoundManager()->playSound3D(target,
                    ESM::RefId::stringRefId("magic sound"), volume, 1.f, MWSound::Type::Sfx,
                    MWSound::PlayMode::LoopNoEnv);
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::SummonScamp ||
            effect.mEffectId == ESM::MagicEffect::SummonClannfear ||
            effect.mEffectId == ESM::MagicEffect::SummonDaedroth ||
            effect.mEffectId == ESM::MagicEffect::SummonDremora ||
            effect.mEffectId == ESM::MagicEffect::SummonAncestralGhost ||
            effect.mEffectId == ESM::MagicEffect::SummonSkeletalMinion ||
            effect.mEffectId == ESM::MagicEffect::SummonBonewalker ||
            effect.mEffectId == ESM::MagicEffect::SummonGreaterBonewalker ||
            effect.mEffectId == ESM::MagicEffect::SummonBonelord ||
            effect.mEffectId == ESM::MagicEffect::SummonWingedTwilight ||
            effect.mEffectId == ESM::MagicEffect::SummonHunger ||
            effect.mEffectId == ESM::MagicEffect::SummonGoldenSaint ||
            effect.mEffectId == ESM::MagicEffect::SummonFlameAtronach ||
            effect.mEffectId == ESM::MagicEffect::SummonFrostAtronach ||
            effect.mEffectId == ESM::MagicEffect::SummonStormAtronach ||
            effect.mEffectId == ESM::MagicEffect::SummonCenturionSphere ||
            effect.mEffectId == ESM::MagicEffect::SummonFabricant ||
            effect.mEffectId == ESM::MagicEffect::SummonWolf ||
            effect.mEffectId == ESM::MagicEffect::SummonBear ||
            effect.mEffectId == ESM::MagicEffect::SummonBonewolf ||
            effect.mEffectId == ESM::MagicEffect::SummonCreature04 ||
            effect.mEffectId == ESM::MagicEffect::SummonCreature05)
        {
            if (!target.isInCell())
                return ESM::ActiveEffect::Flag_Invalid;
            effect.mArg = summonCreature(effect.mEffectId, target);
        }
        else if (effect.mEffectId == ESM::MagicEffect::BoundGloves)
        {
            if (!target.getClass().hasInventoryStore(target))
                return ESM::ActiveEffect::Flag_Invalid;
            addBoundItem(
                ESM::RefId::stringRefId(
                    world->getStore().get<ESM::GameSetting>().find("sMagicBoundRightGauntletID")->mValue.getString()),
                target);
            addBoundItem(
                ESM::RefId::stringRefId(
                    world->getStore().get<ESM::GameSetting>().find("sMagicBoundLeftGauntletID")->mValue.getString()),
                target);
        }
        else if (effect.mEffectId == ESM::MagicEffect::BoundDagger ||
        effect.mEffectId == ESM::MagicEffect::BoundLongsword ||
        effect.mEffectId == ESM::MagicEffect::BoundMace ||
        effect.mEffectId == ESM::MagicEffect::BoundBattleAxe ||
        effect.mEffectId == ESM::MagicEffect::BoundSpear ||
        effect.mEffectId == ESM::MagicEffect::BoundLongbow ||
        effect.mEffectId == ESM::MagicEffect::BoundCuirass ||
        effect.mEffectId == ESM::MagicEffect::BoundHelm ||
        effect.mEffectId == ESM::MagicEffect::BoundBoots ||
        effect.mEffectId == ESM::MagicEffect::BoundShield)
        {
            if (!target.getClass().hasInventoryStore(target))
                return ESM::ActiveEffect::Flag_Invalid;
            const std::string& item = getBoundItemsMap().at(effect.mEffectId);
            const MWWorld::Store<ESM::GameSetting>& gmst = world->getStore().get<ESM::GameSetting>();
            const ESM::RefId itemId = ESM::RefId::stringRefId(gmst.find(item)->mValue.getString());
            if (!addBoundItem(itemId, target))
                effect.mTimeLeft = 0.f;
        }
        else if (effect.mEffectId == ESM::MagicEffect::FireDamage || effect.mEffectId == ESM::MagicEffect::ShockDamage
            || effect.mEffectId == ESM::MagicEffect::FrostDamage || effect.mEffectId == ESM::MagicEffect::DamageHealth
            || effect.mEffectId == ESM::MagicEffect::Poison || effect.mEffectId == ESM::MagicEffect::DamageMagicka
            || effect.mEffectId == ESM::MagicEffect::DamageFatigue)
        {
            if (!godmode)
            {
                int index = 0;
                if (effect.mEffectId == ESM::MagicEffect::DamageMagicka)
                    index = 1;
                else if (effect.mEffectId == ESM::MagicEffect::DamageFatigue)
                    index = 2;
                // Damage "Dynamic" abilities reduce the base value
                if (spellParams.hasFlag(ESM::ActiveSpells::Flag_AffectsBaseValues))
                    modDynamicStat(target, index, -effect.mMagnitude);
                else
                {
                    adjustDynamicStat(
                        target, index, -effect.mMagnitude, index == 2 && Settings::game().mUncappedDamageFatigue);
                    if (index == 0)
                        receivedMagicDamage = affectedHealth = true;
                }
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::DamageAttribute)
        {
            if (!godmode)
                damageAttribute(target, effect, effect.mMagnitude);
        }
        else if (effect.mEffectId == ESM::MagicEffect::DamageSkill)
        {
            if (!godmode && target.getClass().isNpc())
            {
                // Damage Skill abilities reduce base skill :todd:
                if (spellParams.hasFlag(ESM::ActiveSpells::Flag_AffectsBaseValues))
                {
                    auto& npcStats = target.getClass().getNpcStats(target);
                    SkillValue& skill = npcStats.getSkill(effect.getSkillOrAttribute());
                    // Damage Skill abilities reduce base skill :todd:
                    skill.setBase(std::max(skill.getBase() - effect.mMagnitude, 0.f));
                }
                else
                    damageSkill(target, effect, effect.mMagnitude);
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::RestoreAttribute)
            restoreAttribute(target, effect, effect.mMagnitude);
        else if (effect.mEffectId == ESM::MagicEffect::RestoreSkill)
        {
            if (target.getClass().isNpc())
                restoreSkill(target, effect, effect.mMagnitude);
        }
        else if (effect.mEffectId == ESM::MagicEffect::RestoreHealth)
        {
            affectedHealth = true;
            adjustDynamicStat(target, Stats::Health, effect.mMagnitude);
        }
        else if (effect.mEffectId == ESM::MagicEffect::RestoreMagicka)
            adjustDynamicStat(target, Stats::Magicka, effect.mMagnitude);
        else if (effect.mEffectId == ESM::MagicEffect::RestoreFatigue)
            adjustDynamicStat(target, Stats::Fatigue, effect.mMagnitude);
        else if (effect.mEffectId == ESM::MagicEffect::SunDamage)
        {
            //// isInCell shouldn't be needed, but updateActor called during game start
            if (!godmode && target.isInCell() && target.getCell()->isExterior()
                && !(target.getCell()->isQuasiExterior()))
            {
                const float sunRisen = world->getSunPercentage();
                static float fMagicSunBlockedMult
                    = world->getStore().get<ESM::GameSetting>().find("fMagicSunBlockedMult")->mValue.getFloat();
                const float damageScale = std::clamp(
                    std::max(world->getSunVisibility() * sunRisen, fMagicSunBlockedMult * sunRisen), 0.f, 1.f);
                float damage = effect.mMagnitude * damageScale;
                adjustDynamicStat(target, 0, -damage);
                if (damage > 0.f)
                    receivedMagicDamage = affectedHealth = true;
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::DrainHealth)
        {
            if (godmode)
                return ESM::ActiveEffect::Flag_Remove;
            else
            {
                // Unlike Absorb and Damage effects Drain effects can bring stats below zero
                adjustDynamicStat(target, Stats::Health, -effect.mMagnitude, true);
                receivedMagicDamage = affectedHealth = true;
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::DrainMagicka)
        {
            if (godmode)
                return ESM::ActiveEffect::Flag_Remove;
            else
            {
                // Unlike Absorb and Damage effects Drain effects can bring stats below zero
                adjustDynamicStat(target, Stats::Magicka, -effect.mMagnitude, true);
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::DrainFatigue)
        {
            if (godmode)
                return ESM::ActiveEffect::Flag_Remove;
            else
            {
                // Unlike Absorb and Damage effects Drain effects can bring stats below zero
                adjustDynamicStat(target, Stats::Fatigue, -effect.mMagnitude, true);
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::FortifyHealth)
        {
            if (spellParams.hasFlag(ESM::ActiveSpells::Flag_AffectsBaseValues))
                modDynamicStat(target, Stats::Health, effect.mMagnitude);
            else
                adjustDynamicStat(target, Stats::Health, effect.mMagnitude, false, true);
        }
        else if (effect.mEffectId == ESM::MagicEffect::FortifyMagicka)
        {
            if (spellParams.hasFlag(ESM::ActiveSpells::Flag_AffectsBaseValues))
                modDynamicStat(target, Stats::Magicka, effect.mMagnitude);
            else
                adjustDynamicStat(target, Stats::Magicka, effect.mMagnitude, false, true);
        }
        else if (effect.mEffectId == ESM::MagicEffect::FortifyFatigue)
        {
            if (spellParams.hasFlag(ESM::ActiveSpells::Flag_AffectsBaseValues))
                modDynamicStat(target, Stats::Fatigue, effect.mMagnitude);
            else
                adjustDynamicStat(target, Stats::Fatigue, effect.mMagnitude, false, true);
        }
        else if (effect.mEffectId == ESM::MagicEffect::DrainAttribute)
        {
            if (godmode)
                return ESM::ActiveEffect::Flag_Remove;
            damageAttribute(target, effect, effect.mMagnitude);
        }
        else if (effect.mEffectId == ESM::MagicEffect::FortifyAttribute)
        {
            // Abilities affect base stats, but not for drain
            if (spellParams.hasFlag(ESM::ActiveSpells::Flag_AffectsBaseValues))
            {
                auto& creatureStats = target.getClass().getCreatureStats(target);
                auto attribute = effect.getSkillOrAttribute();
                AttributeValue attr = creatureStats.getAttribute(attribute);
                attr.setBase(attr.getBase() + effect.mMagnitude);
                creatureStats.setAttribute(attribute, attr);
            }
            else
                fortifyAttribute(target, effect, effect.mMagnitude);
        }
        else if (effect.mEffectId == ESM::MagicEffect::DrainSkill)
        {
            if (godmode || !target.getClass().isNpc())
                return ESM::ActiveEffect::Flag_Remove;
            damageSkill(target, effect, effect.mMagnitude);
        }
        else if (effect.mEffectId == ESM::MagicEffect::FortifySkill)
        {
            if (target.getClass().isNpc())
            {
                if (spellParams.hasFlag(ESM::ActiveSpells::Flag_AffectsBaseValues))
                {
                    // Abilities affect base stats, but not for drain
                    auto& npcStats = target.getClass().getNpcStats(target);
                    auto& skill = npcStats.getSkill(effect.getSkillOrAttribute());
                    skill.setBase(skill.getBase() + effect.mMagnitude);
                }
                else
                    fortifySkill(target, effect, effect.mMagnitude);
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::FortifyMaximumMagicka)
            recalculateMagicka = true;
        else if (effect.mEffectId == ESM::MagicEffect::AbsorbHealth)
        {
            if (godmode)
                return ESM::ActiveEffect::Flag_Remove;
            else
            {
                adjustDynamicStat(target, Stats::Health, -effect.mMagnitude);
                if (!caster.isEmpty())
                {
                    adjustDynamicStat(caster, Stats::Health, effect.mMagnitude);
                }
                receivedMagicDamage = affectedHealth = true;
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::AbsorbMagicka)
        {
            if (godmode)
                return ESM::ActiveEffect::Flag_Remove;
            else
            {
                adjustDynamicStat(target, Stats::Magicka, -effect.mMagnitude);
                if (!caster.isEmpty())
                    adjustDynamicStat(caster, Stats::Magicka, effect.mMagnitude);
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::AbsorbFatigue)
        {
            if (godmode)
                return ESM::ActiveEffect::Flag_Remove;
            else
            {
                adjustDynamicStat(target, Stats::Fatigue, -effect.mMagnitude);
                if (!caster.isEmpty())
                    adjustDynamicStat(caster, Stats::Fatigue, effect.mMagnitude);
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::AbsorbAttribute)
        {
            if (godmode)
                return ESM::ActiveEffect::Flag_Remove;
            else
            {
                damageAttribute(target, effect, effect.mMagnitude);
                if (!caster.isEmpty())
                    fortifyAttribute(caster, effect, effect.mMagnitude);
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::AbsorbSkill)
        {
            if (godmode)
                return ESM::ActiveEffect::Flag_Remove;
            else
            {
                if (target.getClass().isNpc())
                    damageSkill(target, effect, effect.mMagnitude);
                if (!caster.isEmpty() && caster.getClass().isNpc())
                    fortifySkill(caster, effect, effect.mMagnitude);
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::DisintegrateArmor)
        {
            if (!target.getClass().hasInventoryStore(target))
                return ESM::ActiveEffect::Flag_Invalid;
            if (!godmode)
            {
                static const std::array<int, 9> priorities{
                    MWWorld::InventoryStore::Slot_CarriedLeft,
                    MWWorld::InventoryStore::Slot_Cuirass,
                    MWWorld::InventoryStore::Slot_LeftPauldron,
                    MWWorld::InventoryStore::Slot_RightPauldron,
                    MWWorld::InventoryStore::Slot_LeftGauntlet,
                    MWWorld::InventoryStore::Slot_RightGauntlet,
                    MWWorld::InventoryStore::Slot_Helmet,
                    MWWorld::InventoryStore::Slot_Greaves,
                    MWWorld::InventoryStore::Slot_Boots,
                };
                for (const int priority : priorities)
                {
                    if (disintegrateSlot(target, priority, effect.mMagnitude))
                        break;
                }
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::DisintegrateWeapon)
        {
            if (!target.getClass().hasInventoryStore(target))
                return ESM::ActiveEffect::Flag_Invalid;
            if (!godmode)
                disintegrateSlot(target, MWWorld::InventoryStore::Slot_CarriedRight, effect.mMagnitude);
        }
        return ESM::ActiveEffect::Flag_Applied;
    }

    bool shouldRemoveEffect(const MWWorld::Ptr& target, const ESM::ActiveEffect& effect)
    {
        if (effect.mFlags & ESM::ActiveEffect::Flag_Invalid)
            return true;
        const auto world = MWBase::Environment::get().getWorld();
        if (effect.mEffectId == ESM::MagicEffect::Levitate)
        {
            if (!world->isLevitationEnabled())
            {
                if (target == getPlayer())
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sLevitateDisabled}");
                return true;
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::DivineIntervention ||
        effect.mEffectId == ESM::MagicEffect::Recall ||
        effect.mEffectId == ESM::MagicEffect::AlmsiviIntervention)
        {
            return effect.mFlags & ESM::ActiveEffect::Flag_Applied;
        }
        else if (effect.mEffectId == ESM::MagicEffect::WaterWalking)
        {
            if (target.getClass().isPureWaterCreature(target) && world->isSwimming(target))
                return true;
            if (effect.mFlags & ESM::ActiveEffect::Flag_Applied)
                return false;
            if (!world->isWaterWalkingCastableOnTarget(target))
            {
                if (target == getPlayer())
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicInvalidEffect}");
                return true;
            }
        }
        return false;
    }

    MagicApplicationResult applyMagicEffect(const MWWorld::Ptr& target, const MWWorld::Ptr& caster,
        ActiveSpells::ActiveSpellParams& spellParams, ESM::ActiveEffect& effect, float dt, bool playNonLooping)
    {
        const auto world = MWBase::Environment::get().getWorld();
        int32_t applied = ESM::ActiveEffect::Flag_Remove;
        bool receivedMagicDamage = false;
        bool recalculateMagicka = false;
        bool affectedHealth = false;
        if (effect.mEffectId == ESM::MagicEffect::Corprus && spellParams.shouldWorsen())
        {
            spellParams.worsen();
            for (auto& otherEffect : spellParams.getEffects())
            {
                if (isCorprusEffect(otherEffect))
                    applyMagicEffect(target, caster, spellParams, otherEffect, receivedMagicDamage, affectedHealth,
                        recalculateMagicka);
            }
            if (target == getPlayer())
                MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicCorprusWorsens}");
            return { MagicApplicationResult::Type::APPLIED, receivedMagicDamage, affectedHealth };
        }
        else if (shouldRemoveEffect(target, effect))
        {
            onMagicEffectRemoved(target, spellParams, effect);
            return { MagicApplicationResult::Type::REMOVED, receivedMagicDamage, affectedHealth };
        }
        const auto* magicEffect = world->getStore().get<ESM::MagicEffect>().find(effect.mEffectId);
        if (effect.mFlags & ESM::ActiveEffect::Flag_Applied)
        {
            if (magicEffect->mData.mFlags & ESM::MagicEffect::Flags::AppliedOnce)
            {
                effect.mTimeLeft -= dt;
                return { MagicApplicationResult::Type::APPLIED, receivedMagicDamage, affectedHealth };
            }
            else if (!dt)
                return { MagicApplicationResult::Type::APPLIED, receivedMagicDamage, affectedHealth };
        }
        if (effect.mEffectId == ESM::MagicEffect::Lock)
        {
            if (target.getClass().canLock(target))
            {
                MWRender::Animation* animation = world->getAnimation(target);
                if (animation)
                    animation->addSpellCastGlow(magicEffect->getColor());
                int magnitude = static_cast<int>(roll(effect));
                if (target.getCellRef().getLockLevel()
                    < magnitude) // If the door is not already locked to a higher value, lock it to spell magnitude
                {
                    MWBase::Environment::get().getSoundManager()->playSound3D(
                        target, ESM::RefId::stringRefId("Open Lock"), 1.f, 1.f);

                    if (caster == getPlayer())
                        MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicLockSuccess}");
                    target.getCellRef().lock(magnitude);
                }
            }
            else
                applied |= ESM::ActiveEffect::Flag_Invalid;
        }
        else if (effect.mEffectId == ESM::MagicEffect::Open)
        {
            if (target.getClass().canLock(target))
            {
                // Use the player instead of the caster for vanilla crime compatibility
                MWBase::Environment::get().getMechanicsManager()->unlockAttempted(getPlayer(), target);

                MWRender::Animation* animation = world->getAnimation(target);
                if (animation)
                    animation->addSpellCastGlow(magicEffect->getColor());
                int magnitude = static_cast<int>(roll(effect));
                if (target.getCellRef().getLockLevel() <= magnitude)
                {
                    if (target.getCellRef().isLocked())
                    {
                        MWBase::Environment::get().getSoundManager()->playSound3D(
                            target, ESM::RefId::stringRefId("Open Lock"), 1.f, 1.f);

                        if (caster == getPlayer())
                            MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicOpenSuccess}");
                        target.getCellRef().unlock();
                    }
                }
                else
                {
                    MWBase::Environment::get().getSoundManager()->playSound3D(
                        target, ESM::RefId::stringRefId("Open Lock Fail"), 1.f, 1.f);
                }
            }
            else
                applied |= ESM::ActiveEffect::Flag_Invalid;
        }
        else if (!target.getClass().isActor())
        {
            applied |= ESM::ActiveEffect::Flag_Invalid;
        }
        else
        {
            // Morrowind.exe doesn't apply magic effects while the menu is open, we do because we like to see stats
            // updated instantly. We don't want to teleport instantly though. Nor do we want to force players to drink
            // invisibility potions in the "right" order
            if (!dt
                && (effect.mEffectId == ESM::MagicEffect::Recall
                    || effect.mEffectId == ESM::MagicEffect::DivineIntervention
                    || effect.mEffectId == ESM::MagicEffect::AlmsiviIntervention
                    || effect.mEffectId == ESM::MagicEffect::Invisibility))
                return { MagicApplicationResult::Type::APPLIED, receivedMagicDamage, affectedHealth };
            auto& stats = target.getClass().getCreatureStats(target);
            auto& magnitudes = stats.getMagicEffects();
            if (!spellParams.hasFlag(ESM::ActiveSpells::Flag_AffectsBaseValues)
                && !(effect.mFlags & ESM::ActiveEffect::Flag_Remove))
            {
                MagicApplicationResult::Type result
                    = applyProtections(target, caster, spellParams, effect, magicEffect);
                if (result != MagicApplicationResult::Type::APPLIED)
                    return { result, receivedMagicDamage, affectedHealth };
            }
            float oldMagnitude = 0.f;
            if (effect.mFlags & ESM::ActiveEffect::Flag_Applied)
                oldMagnitude = effect.mMagnitude;
            else if (!(effect.mFlags & ESM::ActiveEffect::Flag_Remove))
            {
                bool isTemporary = spellParams.hasFlag(ESM::ActiveSpells::Flag_Temporary);
                bool isEquipment = spellParams.hasFlag(ESM::ActiveSpells::Flag_Equipment);

                if (!spellParams.hasFlag(ESM::ActiveSpells::Flag_Lua))
                    playEffects(target, *magicEffect, (isTemporary || (isEquipment && playNonLooping)));

                if (effect.mEffectId == ESM::MagicEffect::Soultrap && !target.getClass().isNpc()
                    && target.getType() == ESM::Creature::sRecordId
                    && target.get<ESM::Creature>()->mBase->mData.mSoul == 0 && caster == getPlayer())
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicInvalidTarget}");
            }
            float magnitude = roll(effect);
            // Note that there's an early out for Flag_Applied AppliedOnce effects so we don't have to exclude them here
            effect.mMagnitude = magnitude;
            if (!(magicEffect->mData.mFlags
                    & (ESM::MagicEffect::Flags::NoMagnitude | ESM::MagicEffect::Flags::AppliedOnce)))
            {
                if (effect.mDuration != 0)
                {
                    float mult = dt;
                    if (spellParams.hasFlag(ESM::ActiveSpells::Flag_Temporary))
                        mult = std::min(effect.mTimeLeft, dt);
                    effect.mMagnitude *= mult;
                }
                if (effect.mMagnitude == 0)
                {
                    effect.mMagnitude = oldMagnitude;
                    effect.mFlags |= ESM::ActiveEffect::Flag_Applied | ESM::ActiveEffect::Flag_Remove;
                    effect.mTimeLeft -= dt;
                    return { MagicApplicationResult::Type::APPLIED, receivedMagicDamage, affectedHealth };
                }
            }
            if (effect.mEffectId == ESM::MagicEffect::Corprus)
            {
                spellParams.worsen();
                applied |= ESM::ActiveEffect::Flag_Applied;
            }
            else
                applied |= applyMagicEffect(
                    target, caster, spellParams, effect, receivedMagicDamage, affectedHealth, recalculateMagicka);
            effect.mMagnitude = magnitude;
            magnitudes.add(EffectKey(effect.mEffectId, effect.getSkillOrAttribute()),
                EffectParam(effect.mMagnitude - oldMagnitude));
        }
        effect.mTimeLeft -= dt;
        if (applied & ESM::ActiveEffect::Flag_Invalid)
        {
            effect.mTimeLeft = 0;
            auto anim = world->getAnimation(target);
            if (anim)
                anim->removeEffect(ESM::MagicEffect::refIdToName(effect.mEffectId));
            // Note that we can't return REMOVED here because the effect still needs to be detectable
        }
        effect.mFlags |= applied;
        if (recalculateMagicka)
            target.getClass().getCreatureStats(target).recalculateMagicka();
        return { MagicApplicationResult::Type::APPLIED, receivedMagicDamage, affectedHealth };
    }

    void removeMagicEffect(
        const MWWorld::Ptr& target, ActiveSpells::ActiveSpellParams& spellParams, const ESM::ActiveEffect& effect)
    {
        const auto world = MWBase::Environment::get().getWorld();
        const auto worldModel = MWBase::Environment::get().getWorldModel();
        auto& magnitudes = target.getClass().getCreatureStats(target).getMagicEffects();
        if (effect.mEffectId == ESM::MagicEffect::CommandCreature
            || effect.mEffectId == ESM::MagicEffect::CommandHumanoid)
        {
            if (magnitudes.getOrDefault(effect.mEffectId).getMagnitude() <= 0.f)
            {
                auto& seq = target.getClass().getCreatureStats(target).getAiSequence();
                seq.erasePackageIf([&](const auto& package) {
                    return package->getTypeId() == MWMechanics::AiPackageTypeId::Follow
                        && static_cast<const MWMechanics::AiFollow*>(package.get())->isCommanded();
                });
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::ExtraSpell)
        {
            if (magnitudes.getOrDefault(effect.mEffectId).getMagnitude() <= 0.f && target != getPlayer())
                target.getClass().getInventoryStore(target).autoEquip();
        }
        else if (effect.mEffectId == ESM::MagicEffect::TurnUndead)
        {
            auto& creatureStats = target.getClass().getCreatureStats(target);
            Stat<int> stat = creatureStats.getAiSetting(AiSetting::Flee);
            stat.setModifier(static_cast<int>(stat.getModifier() - effect.mMagnitude));
            creatureStats.setAiSetting(AiSetting::Flee, stat);
        }
        else if (effect.mEffectId == ESM::MagicEffect::FrenzyCreature ||
        effect.mEffectId == ESM::MagicEffect::FrenzyHumanoid)
            modifyAiSetting(target, effect, ESM::MagicEffect::FrenzyCreature, AiSetting::Fight, -effect.mMagnitude);
        else if (effect.mEffectId == ESM::MagicEffect::CalmCreature ||
        effect.mEffectId == ESM::MagicEffect::CalmHumanoid)
            modifyAiSetting(target, effect, ESM::MagicEffect::CalmCreature, AiSetting::Fight, effect.mMagnitude);
        else if (effect.mEffectId == ESM::MagicEffect::DemoralizeCreature ||
        effect.mEffectId == ESM::MagicEffect::DemoralizeHumanoid)
            modifyAiSetting(
                target, effect, ESM::MagicEffect::DemoralizeCreature, AiSetting::Flee, -effect.mMagnitude);
        else if (effect.mEffectId == ESM::MagicEffect::NightEye)
        {
            const MWMechanics::EffectParam nightEye = magnitudes.getOrDefault(effect.mEffectId);
            if (nightEye.getMagnitude() < 0.f && nightEye.getBase() < 0)
            {
                // The PCVisionBonus functions are different from every other magic effect function in that they
                // clamp the value to [0, 1]. Morrowind.exe applies the same clamping to the night-eye effect, which
                // can create situations where an effect is still active (i.e. shown in the menu) but the screen is
                // no longer bright. Modifying the base value here should prevent that while preserving their
                // function.
                float delta = std::clamp(-nightEye.getMagnitude(), 0.f, -static_cast<float>(nightEye.getBase()));
                magnitudes.modifyBase(effect.mEffectId, static_cast<int>(delta));
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::RallyCreature ||
                effect.mEffectId == ESM::MagicEffect::RallyHumanoid)
            modifyAiSetting(target, effect, ESM::MagicEffect::RallyCreature, AiSetting::Flee, effect.mMagnitude);
        else if (effect.mEffectId == ESM::MagicEffect::Sound)
        {
            if (magnitudes.getOrDefault(effect.mEffectId).getModifier() <= 0.f && target == getPlayer())
                MWBase::Environment::get().getSoundManager()->stopSound3D(
                    target, ESM::RefId::stringRefId("magic sound"));
        }
        else if (effect.mEffectId == ESM::MagicEffect::SummonScamp ||
        effect.mEffectId == ESM::MagicEffect::SummonClannfear ||
        effect.mEffectId == ESM::MagicEffect::SummonDaedroth ||
        effect.mEffectId == ESM::MagicEffect::SummonDremora ||
        effect.mEffectId == ESM::MagicEffect::SummonAncestralGhost ||
        effect.mEffectId == ESM::MagicEffect::SummonSkeletalMinion ||
        effect.mEffectId == ESM::MagicEffect::SummonBonewalker ||
        effect.mEffectId == ESM::MagicEffect::SummonGreaterBonewalker ||
        effect.mEffectId == ESM::MagicEffect::SummonBonelord ||
        effect.mEffectId == ESM::MagicEffect::SummonWingedTwilight ||
        effect.mEffectId == ESM::MagicEffect::SummonHunger ||
        effect.mEffectId == ESM::MagicEffect::SummonGoldenSaint ||
        effect.mEffectId == ESM::MagicEffect::SummonFlameAtronach ||
        effect.mEffectId == ESM::MagicEffect::SummonFrostAtronach ||
        effect.mEffectId == ESM::MagicEffect::SummonStormAtronach ||
        effect.mEffectId == ESM::MagicEffect::SummonCenturionSphere ||
        effect.mEffectId == ESM::MagicEffect::SummonFabricant ||
        effect.mEffectId == ESM::MagicEffect::SummonWolf ||
        effect.mEffectId == ESM::MagicEffect::SummonBear ||
        effect.mEffectId == ESM::MagicEffect::SummonBonewolf ||
        effect.mEffectId == ESM::MagicEffect::SummonCreature04 ||
        effect.mEffectId == ESM::MagicEffect::SummonCreature05)
        {
            ESM::RefNum actor = effect.getActor();
            if (actor.isSet())
                MWBase::Environment::get().getMechanicsManager()->cleanupSummonedCreature(actor);
            auto& summons = target.getClass().getCreatureStats(target).getSummonedCreatureMap();
            auto [begin, end] = summons.equal_range(effect.mEffectId);
            for (auto it = begin; it != end; ++it)
            {
                if (it->second == actor)
                {
                    summons.erase(it);
                    break;
                }
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::BoundGloves)
        {
            removeBoundItem(
                ESM::RefId::stringRefId(
                    world->getStore().get<ESM::GameSetting>().find("sMagicBoundRightGauntletID")->mValue.getString()),
                target);
            removeBoundItem(
                ESM::RefId::stringRefId(
                    world->getStore().get<ESM::GameSetting>().find("sMagicBoundLeftGauntletID")->mValue.getString()),
                target);
        }
        else if (effect.mEffectId == ESM::MagicEffect::BoundDagger ||
        effect.mEffectId == ESM::MagicEffect::BoundLongsword ||
        effect.mEffectId == ESM::MagicEffect::BoundMace ||
        effect.mEffectId == ESM::MagicEffect::BoundBattleAxe ||
        effect.mEffectId == ESM::MagicEffect::BoundSpear ||
        effect.mEffectId == ESM::MagicEffect::BoundLongbow ||
        effect.mEffectId == ESM::MagicEffect::BoundCuirass ||
        effect.mEffectId == ESM::MagicEffect::BoundHelm ||
        effect.mEffectId == ESM::MagicEffect::BoundBoots ||
        effect.mEffectId == ESM::MagicEffect::BoundShield)
        {
            const std::string& item = getBoundItemsMap().at(effect.mEffectId);
            removeBoundItem(
                ESM::RefId::stringRefId(world->getStore().get<ESM::GameSetting>().find(item)->mValue.getString()),
                target);
        }
        else if (effect.mEffectId == ESM::MagicEffect::DrainHealth)
            adjustDynamicStat(target, Stats::Health, effect.mMagnitude);
        else if (effect.mEffectId == ESM::MagicEffect::DrainMagicka)
            adjustDynamicStat(target, Stats::Magicka, effect.mMagnitude);
        else if (effect.mEffectId == ESM::MagicEffect::DrainFatigue)
            adjustDynamicStat(target, Stats::Fatigue, effect.mMagnitude);
        else if (effect.mEffectId == ESM::MagicEffect::FortifyHealth)
        {
            if (spellParams.hasFlag(ESM::ActiveSpells::Flag_AffectsBaseValues))
                modDynamicStat(target, Stats::Health, -effect.mMagnitude);
            else
                adjustDynamicStat(target, Stats::Health, -effect.mMagnitude, true);
        }
        else if (effect.mEffectId == ESM::MagicEffect::FortifyMagicka)
        {
            if (spellParams.hasFlag(ESM::ActiveSpells::Flag_AffectsBaseValues))
                modDynamicStat(target, Stats::Magicka, -effect.mMagnitude);
            else
                adjustDynamicStat(target, Stats::Magicka, -effect.mMagnitude, true);
        }
        else if (effect.mEffectId == ESM::MagicEffect::FortifyFatigue)
        {
            if (spellParams.hasFlag(ESM::ActiveSpells::Flag_AffectsBaseValues))
                modDynamicStat(target, Stats::Fatigue, -effect.mMagnitude);
            else
                adjustDynamicStat(target, Stats::Fatigue, -effect.mMagnitude, true);
        }
        else if (effect.mEffectId == ESM::MagicEffect::DrainAttribute)
            restoreAttribute(target, effect, effect.mMagnitude);
        else if (effect.mEffectId == ESM::MagicEffect::FortifyAttribute)
        {
            // Abilities affect base stats, but not for drain
            if (spellParams.hasFlag(ESM::ActiveSpells::Flag_AffectsBaseValues))
            {
                auto& creatureStats = target.getClass().getCreatureStats(target);
                auto attribute = effect.getSkillOrAttribute();
                AttributeValue attr = creatureStats.getAttribute(attribute);
                attr.setBase(attr.getBase() - effect.mMagnitude);
                creatureStats.setAttribute(attribute, attr);
            }
            else
                fortifyAttribute(target, effect, -effect.mMagnitude);
        }
        else if (effect.mEffectId == ESM::MagicEffect::DrainSkill)
        {
            if (target.getClass().isNpc())
                restoreSkill(target, effect, effect.mMagnitude);
        }
        else if (effect.mEffectId == ESM::MagicEffect::FortifySkill)
        {
            if (target.getClass().isNpc())
            {
                // Abilities affect base stats, but not for drain
                if (spellParams.hasFlag(ESM::ActiveSpells::Flag_AffectsBaseValues))
                {
                    auto& npcStats = target.getClass().getNpcStats(target);
                    auto& skill = npcStats.getSkill(effect.getSkillOrAttribute());
                    skill.setBase(skill.getBase() - effect.mMagnitude);
                }
                else
                    fortifySkill(target, effect, -effect.mMagnitude);
            }
        }
        else if (effect.mEffectId == ESM::MagicEffect::FortifyMaximumMagicka)
            target.getClass().getCreatureStats(target).recalculateMagicka();
        else if (effect.mEffectId == ESM::MagicEffect::AbsorbAttribute)
        {
            const auto caster = worldModel->getPtr(spellParams.getCaster());
            restoreAttribute(target, effect, effect.mMagnitude);
            if (!caster.isEmpty())
                fortifyAttribute(caster, effect, -effect.mMagnitude);
        }
        else if (effect.mEffectId == ESM::MagicEffect::AbsorbSkill)
        {
            if (target.getClass().isNpc())
                restoreSkill(target, effect, effect.mMagnitude);
            const auto caster = worldModel->getPtr(spellParams.getCaster());
            if (!caster.isEmpty() && caster.getClass().isNpc())
                fortifySkill(caster, effect, -effect.mMagnitude);
        }
        else if (effect.mEffectId == ESM::MagicEffect::Corprus)
        {
            int worsenings = spellParams.getWorsenings();
            spellParams.resetWorsenings();
            if (worsenings > 0)
            {
                for (const auto& otherEffect : spellParams.getEffects())
                {
                    if (isCorprusEffect(otherEffect, true))
                    {
                        for (int i = 0; i < worsenings; i++)
                            removeMagicEffect(target, spellParams, otherEffect);
                    }
                }
            }
            // Note that we remove the effects, but keep the params
            target.getClass().getCreatureStats(target).getActiveSpells().purge(
                [&spellParams](
                    const ActiveSpells::ActiveSpellParams& params, const auto&) { return &spellParams == &params; },
                target);
        }
    }

    void onMagicEffectRemoved(
        const MWWorld::Ptr& target, ActiveSpells::ActiveSpellParams& spellParams, const ESM::ActiveEffect& effect)
    {
        if (!(effect.mFlags & ESM::ActiveEffect::Flag_Applied))
            return;
        auto& magnitudes = target.getClass().getCreatureStats(target).getMagicEffects();
        magnitudes.add(EffectKey(effect.mEffectId, effect.getSkillOrAttribute()), EffectParam(-effect.mMagnitude));
        removeMagicEffect(target, spellParams, effect);
        if (magnitudes.getOrDefault(effect.mEffectId).getMagnitude() <= 0.f)
        {
            auto anim = MWBase::Environment::get().getWorld()->getAnimation(target);
            if (anim)
                anim->removeEffect(ESM::MagicEffect::refIdToName(effect.mEffectId));
        }
    }

}
