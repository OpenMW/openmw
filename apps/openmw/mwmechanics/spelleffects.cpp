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

namespace
{
    float roll(const ESM::ActiveEffect& effect)
    {
        if (effect.mMinMagnitude == effect.mMaxMagnitude)
            return effect.mMinMagnitude;
        auto& prng = MWBase::Environment::get().getWorld()->getPrng();
        return effect.mMinMagnitude + Misc::Rng::rollDice(effect.mMaxMagnitude - effect.mMinMagnitude + 1, prng);
    }

    void modifyAiSetting(const MWWorld::Ptr& target, const ESM::ActiveEffect& effect,
        ESM::MagicEffect::Effects creatureEffect, MWMechanics::AiSetting setting, float magnitude, bool& invalid)
    {
        if (target == MWMechanics::getPlayer() || (effect.mEffectId == creatureEffect) == target.getClass().isNpc())
            invalid = true;
        else
        {
            auto& creatureStats = target.getClass().getCreatureStats(target);
            auto stat = creatureStats.getAiSetting(setting);
            stat.setModifier(static_cast<int>(stat.getModifier() + magnitude));
            creatureStats.setAiSetting(setting, stat);
        }
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
                absorbStaticModel.value(), ESM::MagicEffect::indexToName(ESM::MagicEffect::SpellAbsorption), false);
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
            target.getClass().onHit(
                target, 0.0f, true, MWWorld::Ptr(), caster, osg::Vec3f(), true, MWMechanics::DamageSourceType::Magical);
        // Apply resistances
        if (!(effect.mFlags & ESM::ActiveEffect::Flag_Ignore_Resistances))
        {
            const ESM::Spell* spell
                = spellParams.hasFlag(ESM::ActiveSpells::Flag_Temporary) ? spellParams.getSpell() : nullptr;
            float magnitudeMult
                = MWMechanics::getEffectMultiplier(effect.mEffectId, target, caster, spell, &magnitudes);
            if (magnitudeMult == 0)
            {
                // Fully resisted, show message
                if (target == MWMechanics::getPlayer())
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicPCResisted}");
                else if (caster == MWMechanics::getPlayer())
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicTargetResisted}");
                return MWMechanics::MagicApplicationResult::Type::REMOVED;
            }
            effect.mMinMagnitude *= magnitudeMult;
            effect.mMaxMagnitude *= magnitudeMult;
        }
        return MWMechanics::MagicApplicationResult::Type::APPLIED;
    }

    static const std::map<int, std::string> sBoundItemsMap{
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
}

namespace MWMechanics
{

    void applyMagicEffect(const MWWorld::Ptr& target, const MWWorld::Ptr& caster,
        const ActiveSpells::ActiveSpellParams& spellParams, ESM::ActiveEffect& effect, bool& invalid,
        bool& receivedMagicDamage, bool& affectedHealth, bool& recalculateMagicka)
    {
        const auto world = MWBase::Environment::get().getWorld();
        bool godmode = target == getPlayer() && world->getGodModeState();
        switch (effect.mEffectId)
        {
            case ESM::MagicEffect::CureCommonDisease:
                target.getClass().getCreatureStats(target).getSpells().purgeCommonDisease();
                break;
            case ESM::MagicEffect::CureBlightDisease:
                target.getClass().getCreatureStats(target).getSpells().purgeBlightDisease();
                break;
            case ESM::MagicEffect::RemoveCurse:
                target.getClass().getCreatureStats(target).getSpells().purgeCurses();
                break;
            case ESM::MagicEffect::CureCorprusDisease:
                target.getClass().getCreatureStats(target).getActiveSpells().purgeEffect(
                    target, ESM::MagicEffect::Corprus);
                break;
            case ESM::MagicEffect::CurePoison:
                target.getClass().getCreatureStats(target).getActiveSpells().purgeEffect(
                    target, ESM::MagicEffect::Poison);
                break;
            case ESM::MagicEffect::CureParalyzation:
                target.getClass().getCreatureStats(target).getActiveSpells().purgeEffect(
                    target, ESM::MagicEffect::Paralyze);
                break;
            case ESM::MagicEffect::Dispel:
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
                break;
            case ESM::MagicEffect::AlmsiviIntervention:
            case ESM::MagicEffect::DivineIntervention:
                if (target != getPlayer())
                    invalid = true;
                else if (world->isTeleportingEnabled())
                {
                    std::string_view marker
                        = (effect.mEffectId == ESM::MagicEffect::DivineIntervention) ? "divinemarker" : "templemarker";
                    world->teleportToClosestMarker(target, ESM::RefId::stringRefId(marker));
                    if (!caster.isEmpty())
                    {
                        MWRender::Animation* anim = world->getAnimation(caster);
                        anim->removeEffect(ESM::MagicEffect::indexToName(effect.mEffectId));
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
                break;
            case ESM::MagicEffect::Mark:
                if (target != getPlayer())
                    invalid = true;
                else if (world->isTeleportingEnabled())
                    world->getPlayer().markPosition(target.getCell(), target.getRefData().getPosition());
                else if (caster == getPlayer())
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sTeleportDisabled}");
                break;
            case ESM::MagicEffect::Recall:
                if (target != getPlayer())
                    invalid = true;
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
                            anim->removeEffect(ESM::MagicEffect::indexToName(effect.mEffectId));
                        }
                    }
                }
                else if (caster == getPlayer())
                    MWBase::Environment::get().getWindowManager()->messageBox("#{sTeleportDisabled}");
                break;
            case ESM::MagicEffect::CommandCreature:
            case ESM::MagicEffect::CommandHumanoid:
                if (caster.isEmpty() || !caster.getClass().isActor() || target == getPlayer()
                    || (effect.mEffectId == ESM::MagicEffect::CommandCreature) == target.getClass().isNpc())
                    invalid = true;
                else if (effect.mMagnitude >= target.getClass().getCreatureStats(target).getLevel())
                {
                    MWMechanics::AiFollow package(caster, true);
                    target.getClass().getCreatureStats(target).getAiSequence().stack(package, target);
                }
                break;
            case ESM::MagicEffect::ExtraSpell:
                if (target.getClass().hasInventoryStore(target))
                {
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
                                    if (carried == store.end()
                                        || carried.getType() != MWWorld::ContainerStore::Type_Armor)
                                        continue;
                                    [[fallthrough]];
                                }
                                default:
                                    store.unequipSlot(slot);
                            }
                        }
                    }
                }
                else
                    invalid = true;
                break;
            case ESM::MagicEffect::TurnUndead:
                if (target.getClass().isNpc()
                    || target.get<ESM::Creature>()->mBase->mData.mType != ESM::Creature::Undead)
                    invalid = true;
                else
                {
                    auto& creatureStats = target.getClass().getCreatureStats(target);
                    Stat<int> stat = creatureStats.getAiSetting(AiSetting::Flee);
                    stat.setModifier(static_cast<int>(stat.getModifier() + effect.mMagnitude));
                    creatureStats.setAiSetting(AiSetting::Flee, stat);
                }
                break;
            case ESM::MagicEffect::FrenzyCreature:
            case ESM::MagicEffect::FrenzyHumanoid:
                modifyAiSetting(
                    target, effect, ESM::MagicEffect::FrenzyCreature, AiSetting::Fight, effect.mMagnitude, invalid);
                break;
            case ESM::MagicEffect::CalmCreature:
            case ESM::MagicEffect::CalmHumanoid:
                modifyAiSetting(
                    target, effect, ESM::MagicEffect::CalmCreature, AiSetting::Fight, -effect.mMagnitude, invalid);
                if (!invalid && effect.mMagnitude > 0)
                {
                    auto& creatureStats = target.getClass().getCreatureStats(target);
                    creatureStats.getAiSequence().stopCombat();
                }
                break;
            case ESM::MagicEffect::DemoralizeCreature:
            case ESM::MagicEffect::DemoralizeHumanoid:
                modifyAiSetting(
                    target, effect, ESM::MagicEffect::DemoralizeCreature, AiSetting::Flee, effect.mMagnitude, invalid);
                break;
            case ESM::MagicEffect::RallyCreature:
            case ESM::MagicEffect::RallyHumanoid:
                modifyAiSetting(
                    target, effect, ESM::MagicEffect::RallyCreature, AiSetting::Flee, -effect.mMagnitude, invalid);
                break;
            case ESM::MagicEffect::Charm:
                if (!target.getClass().isNpc())
                    invalid = true;
                break;
            case ESM::MagicEffect::Sound:
                if (target == getPlayer())
                {
                    const auto& magnitudes = target.getClass().getCreatureStats(target).getMagicEffects();
                    float volume = std::clamp(
                        (magnitudes.getOrDefault(effect.mEffectId).getModifier() + effect.mMagnitude) / 100.f, 0.f,
                        1.f);
                    MWBase::Environment::get().getSoundManager()->playSound3D(target,
                        ESM::RefId::stringRefId("magic sound"), volume, 1.f, MWSound::Type::Sfx,
                        MWSound::PlayMode::LoopNoEnv);
                }
                break;
            case ESM::MagicEffect::SummonScamp:
            case ESM::MagicEffect::SummonClannfear:
            case ESM::MagicEffect::SummonDaedroth:
            case ESM::MagicEffect::SummonDremora:
            case ESM::MagicEffect::SummonAncestralGhost:
            case ESM::MagicEffect::SummonSkeletalMinion:
            case ESM::MagicEffect::SummonBonewalker:
            case ESM::MagicEffect::SummonGreaterBonewalker:
            case ESM::MagicEffect::SummonBonelord:
            case ESM::MagicEffect::SummonWingedTwilight:
            case ESM::MagicEffect::SummonHunger:
            case ESM::MagicEffect::SummonGoldenSaint:
            case ESM::MagicEffect::SummonFlameAtronach:
            case ESM::MagicEffect::SummonFrostAtronach:
            case ESM::MagicEffect::SummonStormAtronach:
            case ESM::MagicEffect::SummonCenturionSphere:
            case ESM::MagicEffect::SummonFabricant:
            case ESM::MagicEffect::SummonWolf:
            case ESM::MagicEffect::SummonBear:
            case ESM::MagicEffect::SummonBonewolf:
            case ESM::MagicEffect::SummonCreature04:
            case ESM::MagicEffect::SummonCreature05:
                if (!target.isInCell())
                    invalid = true;
                else
                    effect.mArg = summonCreature(effect.mEffectId, target);
                break;
            case ESM::MagicEffect::BoundGloves:
                if (!target.getClass().hasInventoryStore(target))
                {
                    invalid = true;
                    break;
                }
                addBoundItem(ESM::RefId::stringRefId(world->getStore()
                                                         .get<ESM::GameSetting>()
                                                         .find("sMagicBoundRightGauntletID")
                                                         ->mValue.getString()),
                    target);
                // left gauntlet added below
                [[fallthrough]];
            case ESM::MagicEffect::BoundDagger:
            case ESM::MagicEffect::BoundLongsword:
            case ESM::MagicEffect::BoundMace:
            case ESM::MagicEffect::BoundBattleAxe:
            case ESM::MagicEffect::BoundSpear:
            case ESM::MagicEffect::BoundLongbow:
            case ESM::MagicEffect::BoundCuirass:
            case ESM::MagicEffect::BoundHelm:
            case ESM::MagicEffect::BoundBoots:
            case ESM::MagicEffect::BoundShield:
            {
                if (!target.getClass().hasInventoryStore(target))
                {
                    invalid = true;
                    break;
                }
                const std::string& item = sBoundItemsMap.at(effect.mEffectId);
                const MWWorld::Store<ESM::GameSetting>& gmst = world->getStore().get<ESM::GameSetting>();
                const ESM::RefId itemId = ESM::RefId::stringRefId(gmst.find(item)->mValue.getString());
                if (!addBoundItem(itemId, target))
                    effect.mTimeLeft = 0.f;
                break;
            }
            case ESM::MagicEffect::FireDamage:
            case ESM::MagicEffect::ShockDamage:
            case ESM::MagicEffect::FrostDamage:
            case ESM::MagicEffect::DamageHealth:
            case ESM::MagicEffect::Poison:
            case ESM::MagicEffect::DamageMagicka:
            case ESM::MagicEffect::DamageFatigue:
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
                break;
            case ESM::MagicEffect::DamageAttribute:
                if (!godmode)
                    damageAttribute(target, effect, effect.mMagnitude);
                break;
            case ESM::MagicEffect::DamageSkill:
                if (!target.getClass().isNpc())
                    invalid = true;
                else if (!godmode)
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
                break;
            case ESM::MagicEffect::RestoreAttribute:
                restoreAttribute(target, effect, effect.mMagnitude);
                break;
            case ESM::MagicEffect::RestoreSkill:
                if (!target.getClass().isNpc())
                    invalid = true;
                else
                    restoreSkill(target, effect, effect.mMagnitude);
                break;
            case ESM::MagicEffect::RestoreHealth:
                affectedHealth = true;
                [[fallthrough]];
            case ESM::MagicEffect::RestoreMagicka:
            case ESM::MagicEffect::RestoreFatigue:
                adjustDynamicStat(target, effect.mEffectId - ESM::MagicEffect::RestoreHealth, effect.mMagnitude);
                break;
            case ESM::MagicEffect::SunDamage:
            {
                // isInCell shouldn't be needed, but updateActor called during game start
                if (!target.isInCell() || !(target.getCell()->isExterior() || target.getCell()->isQuasiExterior())
                    || godmode)
                    break;
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
            break;
            case ESM::MagicEffect::DrainHealth:
            case ESM::MagicEffect::DrainMagicka:
            case ESM::MagicEffect::DrainFatigue:
                if (!godmode)
                {
                    int index = effect.mEffectId - ESM::MagicEffect::DrainHealth;
                    // Unlike Absorb and Damage effects Drain effects can bring stats below zero
                    adjustDynamicStat(target, index, -effect.mMagnitude, true);
                    if (index == 0)
                        receivedMagicDamage = affectedHealth = true;
                }
                break;
            case ESM::MagicEffect::FortifyHealth:
            case ESM::MagicEffect::FortifyMagicka:
            case ESM::MagicEffect::FortifyFatigue:
                if (spellParams.hasFlag(ESM::ActiveSpells::Flag_AffectsBaseValues))
                    modDynamicStat(target, effect.mEffectId - ESM::MagicEffect::FortifyHealth, effect.mMagnitude);
                else
                    adjustDynamicStat(
                        target, effect.mEffectId - ESM::MagicEffect::FortifyHealth, effect.mMagnitude, false, true);
                break;
            case ESM::MagicEffect::DrainAttribute:
                if (!godmode)
                    damageAttribute(target, effect, effect.mMagnitude);
                break;
            case ESM::MagicEffect::FortifyAttribute:
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
                break;
            case ESM::MagicEffect::DrainSkill:
                if (!target.getClass().isNpc())
                    invalid = true;
                else if (!godmode)
                    damageSkill(target, effect, effect.mMagnitude);
                break;
            case ESM::MagicEffect::FortifySkill:
                if (!target.getClass().isNpc())
                    invalid = true;
                else if (spellParams.hasFlag(ESM::ActiveSpells::Flag_AffectsBaseValues))
                {
                    // Abilities affect base stats, but not for drain
                    auto& npcStats = target.getClass().getNpcStats(target);
                    auto& skill = npcStats.getSkill(effect.getSkillOrAttribute());
                    skill.setBase(skill.getBase() + effect.mMagnitude);
                }
                else
                    fortifySkill(target, effect, effect.mMagnitude);
                break;
            case ESM::MagicEffect::FortifyMaximumMagicka:
                recalculateMagicka = true;
                break;
            case ESM::MagicEffect::AbsorbHealth:
            case ESM::MagicEffect::AbsorbMagicka:
            case ESM::MagicEffect::AbsorbFatigue:
                if (!godmode)
                {
                    int index = effect.mEffectId - ESM::MagicEffect::AbsorbHealth;
                    adjustDynamicStat(target, index, -effect.mMagnitude);
                    if (!caster.isEmpty())
                        adjustDynamicStat(caster, index, effect.mMagnitude);
                    if (index == 0)
                        receivedMagicDamage = affectedHealth = true;
                }
                break;
            case ESM::MagicEffect::AbsorbAttribute:
                if (!godmode)
                {
                    damageAttribute(target, effect, effect.mMagnitude);
                    if (!caster.isEmpty())
                        fortifyAttribute(caster, effect, effect.mMagnitude);
                }
                break;
            case ESM::MagicEffect::AbsorbSkill:
                if (!target.getClass().isNpc())
                    invalid = true;
                else if (!godmode)
                {
                    damageSkill(target, effect, effect.mMagnitude);
                    if (!caster.isEmpty())
                        fortifySkill(caster, effect, effect.mMagnitude);
                }
                break;
            case ESM::MagicEffect::DisintegrateArmor:
            {
                if (!target.getClass().hasInventoryStore(target))
                {
                    invalid = true;
                    break;
                }
                if (godmode)
                    break;
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
                break;
            }
            case ESM::MagicEffect::DisintegrateWeapon:
                if (!target.getClass().hasInventoryStore(target))
                {
                    invalid = true;
                    break;
                }
                if (!godmode)
                    disintegrateSlot(target, MWWorld::InventoryStore::Slot_CarriedRight, effect.mMagnitude);
                break;
        }
    }

    bool shouldRemoveEffect(const MWWorld::Ptr& target, const ESM::ActiveEffect& effect)
    {
        const auto world = MWBase::Environment::get().getWorld();
        switch (effect.mEffectId)
        {
            case ESM::MagicEffect::Levitate:
            {
                if (!world->isLevitationEnabled())
                {
                    if (target == getPlayer())
                        MWBase::Environment::get().getWindowManager()->messageBox("#{sLevitateDisabled}");
                    return true;
                }
                break;
            }
            case ESM::MagicEffect::Recall:
            case ESM::MagicEffect::DivineIntervention:
            case ESM::MagicEffect::AlmsiviIntervention:
            {
                return effect.mFlags & ESM::ActiveEffect::Flag_Applied;
            }
            case ESM::MagicEffect::WaterWalking:
            {
                if (target.getClass().isPureWaterCreature(target) && world->isSwimming(target))
                    return true;
                if (effect.mFlags & ESM::ActiveEffect::Flag_Applied)
                    break;
                if (!world->isWaterWalkingCastableOnTarget(target))
                {
                    if (target == getPlayer())
                        MWBase::Environment::get().getWindowManager()->messageBox("#{sMagicInvalidEffect}");
                    return true;
                }
                break;
            }
        }
        return false;
    }

    MagicApplicationResult applyMagicEffect(const MWWorld::Ptr& target, const MWWorld::Ptr& caster,
        ActiveSpells::ActiveSpellParams& spellParams, ESM::ActiveEffect& effect, float dt, bool playNonLooping)
    {
        const auto world = MWBase::Environment::get().getWorld();
        bool invalid = false;
        bool receivedMagicDamage = false;
        bool recalculateMagicka = false;
        bool affectedHealth = false;
        if (effect.mEffectId == ESM::MagicEffect::Corprus && spellParams.shouldWorsen())
        {
            spellParams.worsen();
            for (auto& otherEffect : spellParams.getEffects())
            {
                if (isCorprusEffect(otherEffect))
                    applyMagicEffect(target, caster, spellParams, otherEffect, invalid, receivedMagicDamage,
                        affectedHealth, recalculateMagicka);
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
                invalid = true;
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
                invalid = true;
        }
        else if (!target.getClass().isActor())
        {
            invalid = true;
        }
        else
        {
            // Morrowind.exe doesn't apply magic effects while the menu is open, we do because we like to see stats
            // updated instantly. We don't want to teleport instantly though
            if (!dt
                && (effect.mEffectId == ESM::MagicEffect::Recall
                    || effect.mEffectId == ESM::MagicEffect::DivineIntervention
                    || effect.mEffectId == ESM::MagicEffect::AlmsiviIntervention))
                return { MagicApplicationResult::Type::APPLIED, receivedMagicDamage, affectedHealth };
            auto& stats = target.getClass().getCreatureStats(target);
            auto& magnitudes = stats.getMagicEffects();
            if (!spellParams.hasFlag(ESM::ActiveSpells::Flag_AffectsBaseValues)
                && !(effect.mFlags & ESM::ActiveEffect::Flag_Applied))
            {
                MagicApplicationResult::Type result
                    = applyProtections(target, caster, spellParams, effect, magicEffect);
                if (result != MagicApplicationResult::Type::APPLIED)
                    return { result, receivedMagicDamage, affectedHealth };
            }
            float oldMagnitude = 0.f;
            if (effect.mFlags & ESM::ActiveEffect::Flag_Applied)
                oldMagnitude = effect.mMagnitude;
            else
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
                spellParams.worsen();
            else
                applyMagicEffect(target, caster, spellParams, effect, invalid, receivedMagicDamage, affectedHealth,
                    recalculateMagicka);
            effect.mMagnitude = magnitude;
            magnitudes.add(EffectKey(effect.mEffectId, effect.getSkillOrAttribute()),
                EffectParam(effect.mMagnitude - oldMagnitude));
        }
        effect.mTimeLeft -= dt;
        if (invalid)
        {
            effect.mTimeLeft = 0;
            effect.mFlags |= ESM::ActiveEffect::Flag_Remove;
            auto anim = world->getAnimation(target);
            if (anim)
                anim->removeEffect(ESM::MagicEffect::indexToName(effect.mEffectId));
        }
        else
            effect.mFlags |= ESM::ActiveEffect::Flag_Applied | ESM::ActiveEffect::Flag_Remove;
        if (recalculateMagicka)
            target.getClass().getCreatureStats(target).recalculateMagicka();
        return { MagicApplicationResult::Type::APPLIED, receivedMagicDamage, affectedHealth };
    }

    void removeMagicEffect(
        const MWWorld::Ptr& target, ActiveSpells::ActiveSpellParams& spellParams, const ESM::ActiveEffect& effect)
    {
        const auto world = MWBase::Environment::get().getWorld();
        auto& magnitudes = target.getClass().getCreatureStats(target).getMagicEffects();
        bool invalid;
        switch (effect.mEffectId)
        {
            case ESM::MagicEffect::CommandCreature:
            case ESM::MagicEffect::CommandHumanoid:
                if (magnitudes.getOrDefault(effect.mEffectId).getMagnitude() <= 0.f)
                {
                    auto& seq = target.getClass().getCreatureStats(target).getAiSequence();
                    seq.erasePackageIf([&](const auto& package) {
                        return package->getTypeId() == MWMechanics::AiPackageTypeId::Follow
                            && static_cast<const MWMechanics::AiFollow*>(package.get())->isCommanded();
                    });
                }
                break;
            case ESM::MagicEffect::ExtraSpell:
                if (magnitudes.getOrDefault(effect.mEffectId).getMagnitude() <= 0.f && target != getPlayer())
                    target.getClass().getInventoryStore(target).autoEquip();
                break;
            case ESM::MagicEffect::TurnUndead:
            {
                auto& creatureStats = target.getClass().getCreatureStats(target);
                Stat<int> stat = creatureStats.getAiSetting(AiSetting::Flee);
                stat.setModifier(static_cast<int>(stat.getModifier() - effect.mMagnitude));
                creatureStats.setAiSetting(AiSetting::Flee, stat);
            }
            break;
            case ESM::MagicEffect::FrenzyCreature:
            case ESM::MagicEffect::FrenzyHumanoid:
                modifyAiSetting(
                    target, effect, ESM::MagicEffect::FrenzyCreature, AiSetting::Fight, -effect.mMagnitude, invalid);
                break;
            case ESM::MagicEffect::CalmCreature:
            case ESM::MagicEffect::CalmHumanoid:
                modifyAiSetting(
                    target, effect, ESM::MagicEffect::CalmCreature, AiSetting::Fight, effect.mMagnitude, invalid);
                break;
            case ESM::MagicEffect::DemoralizeCreature:
            case ESM::MagicEffect::DemoralizeHumanoid:
                modifyAiSetting(
                    target, effect, ESM::MagicEffect::DemoralizeCreature, AiSetting::Flee, -effect.mMagnitude, invalid);
                break;
            case ESM::MagicEffect::NightEye:
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
            break;
            case ESM::MagicEffect::RallyCreature:
            case ESM::MagicEffect::RallyHumanoid:
                modifyAiSetting(
                    target, effect, ESM::MagicEffect::RallyCreature, AiSetting::Flee, effect.mMagnitude, invalid);
                break;
            case ESM::MagicEffect::Sound:
                if (magnitudes.getOrDefault(effect.mEffectId).getModifier() <= 0.f && target == getPlayer())
                    MWBase::Environment::get().getSoundManager()->stopSound3D(
                        target, ESM::RefId::stringRefId("magic sound"));
                break;
            case ESM::MagicEffect::SummonScamp:
            case ESM::MagicEffect::SummonClannfear:
            case ESM::MagicEffect::SummonDaedroth:
            case ESM::MagicEffect::SummonDremora:
            case ESM::MagicEffect::SummonAncestralGhost:
            case ESM::MagicEffect::SummonSkeletalMinion:
            case ESM::MagicEffect::SummonBonewalker:
            case ESM::MagicEffect::SummonGreaterBonewalker:
            case ESM::MagicEffect::SummonBonelord:
            case ESM::MagicEffect::SummonWingedTwilight:
            case ESM::MagicEffect::SummonHunger:
            case ESM::MagicEffect::SummonGoldenSaint:
            case ESM::MagicEffect::SummonFlameAtronach:
            case ESM::MagicEffect::SummonFrostAtronach:
            case ESM::MagicEffect::SummonStormAtronach:
            case ESM::MagicEffect::SummonCenturionSphere:
            case ESM::MagicEffect::SummonFabricant:
            case ESM::MagicEffect::SummonWolf:
            case ESM::MagicEffect::SummonBear:
            case ESM::MagicEffect::SummonBonewolf:
            case ESM::MagicEffect::SummonCreature04:
            case ESM::MagicEffect::SummonCreature05:
            {
                int actorId = effect.getActorId();
                if (actorId != -1)
                    MWBase::Environment::get().getMechanicsManager()->cleanupSummonedCreature(target, actorId);
                auto& summons = target.getClass().getCreatureStats(target).getSummonedCreatureMap();
                auto [begin, end] = summons.equal_range(effect.mEffectId);
                for (auto it = begin; it != end; ++it)
                {
                    if (it->second == actorId)
                    {
                        summons.erase(it);
                        break;
                    }
                }
            }
            break;
            case ESM::MagicEffect::BoundGloves:
                removeBoundItem(ESM::RefId::stringRefId(world->getStore()
                                                            .get<ESM::GameSetting>()
                                                            .find("sMagicBoundRightGauntletID")
                                                            ->mValue.getString()),
                    target);
                [[fallthrough]];
            case ESM::MagicEffect::BoundDagger:
            case ESM::MagicEffect::BoundLongsword:
            case ESM::MagicEffect::BoundMace:
            case ESM::MagicEffect::BoundBattleAxe:
            case ESM::MagicEffect::BoundSpear:
            case ESM::MagicEffect::BoundLongbow:
            case ESM::MagicEffect::BoundCuirass:
            case ESM::MagicEffect::BoundHelm:
            case ESM::MagicEffect::BoundBoots:
            case ESM::MagicEffect::BoundShield:
            {
                const std::string& item = sBoundItemsMap.at(effect.mEffectId);
                removeBoundItem(
                    ESM::RefId::stringRefId(world->getStore().get<ESM::GameSetting>().find(item)->mValue.getString()),
                    target);
            }
            break;
            case ESM::MagicEffect::DrainHealth:
            case ESM::MagicEffect::DrainMagicka:
            case ESM::MagicEffect::DrainFatigue:
                adjustDynamicStat(target, effect.mEffectId - ESM::MagicEffect::DrainHealth, effect.mMagnitude);
                break;
            case ESM::MagicEffect::FortifyHealth:
            case ESM::MagicEffect::FortifyMagicka:
            case ESM::MagicEffect::FortifyFatigue:
                if (spellParams.hasFlag(ESM::ActiveSpells::Flag_AffectsBaseValues))
                    modDynamicStat(target, effect.mEffectId - ESM::MagicEffect::FortifyHealth, -effect.mMagnitude);
                else
                    adjustDynamicStat(
                        target, effect.mEffectId - ESM::MagicEffect::FortifyHealth, -effect.mMagnitude, true);
                break;
            case ESM::MagicEffect::DrainAttribute:
                restoreAttribute(target, effect, effect.mMagnitude);
                break;
            case ESM::MagicEffect::FortifyAttribute:
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
                break;
            case ESM::MagicEffect::DrainSkill:
                restoreSkill(target, effect, effect.mMagnitude);
                break;
            case ESM::MagicEffect::FortifySkill:
                // Abilities affect base stats, but not for drain
                if (spellParams.hasFlag(ESM::ActiveSpells::Flag_AffectsBaseValues))
                {
                    auto& npcStats = target.getClass().getNpcStats(target);
                    auto& skill = npcStats.getSkill(effect.getSkillOrAttribute());
                    skill.setBase(skill.getBase() - effect.mMagnitude);
                }
                else
                    fortifySkill(target, effect, -effect.mMagnitude);
                break;
            case ESM::MagicEffect::FortifyMaximumMagicka:
                target.getClass().getCreatureStats(target).recalculateMagicka();
                break;
            case ESM::MagicEffect::AbsorbAttribute:
            {
                const auto caster = world->searchPtrViaActorId(spellParams.getCasterActorId());
                restoreAttribute(target, effect, effect.mMagnitude);
                if (!caster.isEmpty())
                    fortifyAttribute(caster, effect, -effect.mMagnitude);
            }
            break;
            case ESM::MagicEffect::AbsorbSkill:
            {
                const auto caster = world->searchPtrViaActorId(spellParams.getCasterActorId());
                restoreSkill(target, effect, effect.mMagnitude);
                if (!caster.isEmpty())
                    fortifySkill(caster, effect, -effect.mMagnitude);
            }
            break;
            case ESM::MagicEffect::Corprus:
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
            break;
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
                anim->removeEffect(ESM::MagicEffect::indexToName(effect.mEffectId));
        }
    }

}
