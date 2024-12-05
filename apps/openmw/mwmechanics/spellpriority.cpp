#include "spellpriority.hpp"
#include "weaponpriority.hpp"

#include <cmath>

#include <components/esm3/loadench.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadrace.hpp>
#include <components/esm3/loadspel.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "creaturestats.hpp"
#include "spellresistance.hpp"
#include "spellutil.hpp"
#include "summoning.hpp"
#include "weapontype.hpp"

namespace
{
    int numEffectsToDispel(const MWWorld::Ptr& actor, int effectFilter = -1, bool negative = true)
    {
        int toCure = 0;
        const MWMechanics::ActiveSpells& activeSpells = actor.getClass().getCreatureStats(actor).getActiveSpells();
        for (MWMechanics::ActiveSpells::TIterator it = activeSpells.begin(); it != activeSpells.end(); ++it)
        {
            // if the effect filter is not specified, take in account only spells effects. Leave potions, enchanted
            // items etc.
            if (effectFilter == -1)
            {
                const ESM::Spell* spell
                    = MWBase::Environment::get().getESMStore()->get<ESM::Spell>().search(it->getSourceSpellId());
                if (!spell || spell->mData.mType != ESM::Spell::ST_Spell)
                    continue;
            }

            const MWMechanics::ActiveSpells::ActiveSpellParams& params = *it;
            for (const auto& effect : params.getEffects())
            {
                int effectId = effect.mEffectId;
                if (effectFilter != -1 && effectId != effectFilter)
                    continue;
                const ESM::MagicEffect* magicEffect
                    = MWBase::Environment::get().getESMStore()->get<ESM::MagicEffect>().find(effectId);

                if (effect.mDuration <= 3) // Don't attempt to dispel if effect runs out shortly anyway
                    continue;

                if (negative && magicEffect->mData.mFlags & ESM::MagicEffect::Harmful)
                    ++toCure;

                if (!negative && !(magicEffect->mData.mFlags & ESM::MagicEffect::Harmful))
                    ++toCure;
            }
        }
        return toCure;
    }

    float getSpellDuration(const MWWorld::Ptr& actor, const ESM::RefId& spellId)
    {
        float duration = 0;
        const MWMechanics::ActiveSpells& activeSpells = actor.getClass().getCreatureStats(actor).getActiveSpells();
        for (MWMechanics::ActiveSpells::TIterator it = activeSpells.begin(); it != activeSpells.end(); ++it)
        {
            if (it->getSourceSpellId() != spellId)
                continue;

            const MWMechanics::ActiveSpells::ActiveSpellParams& params = *it;
            for (const auto& effect : params.getEffects())
            {
                if (effect.mDuration > duration)
                    duration = effect.mDuration;
            }
        }
        return duration;
    }

    bool isSpellActive(const MWWorld::Ptr& caster, const MWWorld::Ptr& target, const ESM::RefId& id)
    {
        int actorId = caster.getClass().getCreatureStats(caster).getActorId();
        const auto& active = target.getClass().getCreatureStats(target).getActiveSpells();
        return std::find_if(active.begin(), active.end(), [&](const auto& spell) {
            return spell.getCasterActorId() == actorId && spell.getSourceSpellId() == id;
        }) != active.end();
    }

    float getRestoreMagickaPriority(const MWWorld::Ptr& actor)
    {
        const MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
        const MWMechanics::DynamicStat<float>& current = stats.getMagicka();
        for (const ESM::Spell* spell : stats.getSpells())
        {
            if (spell->mData.mType != ESM::Spell::ST_Spell)
                continue;
            int cost = MWMechanics::calcSpellCost(*spell);
            if (cost > current.getCurrent() && cost < current.getModified())
                return 2.f;
        }
        return 0.f;
    }
}

namespace MWMechanics
{
    int getRangeTypes(const ESM::EffectList& effects)
    {
        int types = 0;
        for (const ESM::IndexedENAMstruct& effect : effects.mList)
        {
            if (effect.mData.mRange == ESM::RT_Self)
                types |= RangeTypes::Self;
            else if (effect.mData.mRange == ESM::RT_Touch)
                types |= RangeTypes::Touch;
            else if (effect.mData.mRange == ESM::RT_Target)
                types |= RangeTypes::Target;
        }
        return types;
    }

    float ratePotion(const MWWorld::Ptr& item, const MWWorld::Ptr& actor)
    {
        if (item.getType() != ESM::Potion::sRecordId)
            return 0.f;

        const ESM::Potion* potion = item.get<ESM::Potion>()->mBase;
        return rateEffects(potion->mEffects, actor, MWWorld::Ptr());
    }

    float rateSpell(const ESM::Spell* spell, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy, bool checkMagicka)
    {
        float successChance = MWMechanics::getSpellSuccessChance(spell, actor, nullptr, true, checkMagicka);
        if (successChance == 0.f)
            return 0.f;

        if (spell->mData.mType != ESM::Spell::ST_Spell)
            return 0.f;

        // Don't make use of racial bonus spells, like MW. Can be made optional later
        if (actor.getClass().isNpc())
        {
            const ESM::RefId& raceid = actor.get<ESM::NPC>()->mBase->mRace;
            const ESM::Race* race = MWBase::Environment::get().getESMStore()->get<ESM::Race>().find(raceid);
            if (race->mPowers.exists(spell->mId))
                return 0.f;
        }

        // Spells don't stack, so early out if the spell is still active on the target
        int types = getRangeTypes(spell->mEffects);
        if ((types & Self) && isSpellActive(actor, actor, spell->mId))
            return 0.f;
        if (((types & Touch) || (types & Target)) && !enemy.isEmpty() && isSpellActive(actor, enemy, spell->mId))
            return 0.f;

        return rateEffects(spell->mEffects, actor, enemy) * (successChance / 100.f);
    }

    float rateMagicItem(const MWWorld::Ptr& ptr, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy)
    {
        if (ptr.getClass().getEnchantment(ptr).empty())
            return 0.f;

        const ESM::Enchantment* enchantment = MWBase::Environment::get().getESMStore()->get<ESM::Enchantment>().find(
            ptr.getClass().getEnchantment(ptr));

        // Spells don't stack, so early out if the spell is still active on the target
        int types = getRangeTypes(enchantment->mEffects);
        if ((types & Self)
            && actor.getClass().getCreatureStats(actor).getActiveSpells().isSpellActive(ptr.getCellRef().getRefId()))
            return 0.f;

        if (types & (Touch | Target) && getSpellDuration(enemy, ptr.getCellRef().getRefId()) > 3)
            return 0.f;

        if (enchantment->mData.mType == ESM::Enchantment::CastOnce)
        {
            return rateEffects(enchantment->mEffects, actor, enemy);
        }
        else if (enchantment->mData.mType == ESM::Enchantment::WhenUsed)
        {
            MWWorld::InventoryStore& store = actor.getClass().getInventoryStore(actor);

            // Creatures can not wear armor/clothing, so allow creatures to use non-equipped items,
            if (actor.getClass().isNpc() && !store.isEquipped(ptr))
                return 0.f;

            int castCost = getEffectiveEnchantmentCastCost(*enchantment, actor);

            if (ptr.getCellRef().getEnchantmentCharge() != -1 && ptr.getCellRef().getEnchantmentCharge() < castCost)
                return 0.f;

            float rating = rateEffects(enchantment->mEffects, actor, enemy);

            rating *= 1.25f; // prefer rechargeable magic items over spells
            return rating;
        }

        return 0.f;
    }

    float rateEffect(const ESM::ENAMstruct& effect, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy)
    {
        // NOTE: enemy may be empty

        float rating = 1;
        switch (effect.mEffectID)
        {
            case ESM::MagicEffect::Soultrap:
            case ESM::MagicEffect::AlmsiviIntervention:
            case ESM::MagicEffect::DivineIntervention:
            case ESM::MagicEffect::CalmHumanoid:
            case ESM::MagicEffect::CalmCreature:
            case ESM::MagicEffect::FrenzyHumanoid:
            case ESM::MagicEffect::FrenzyCreature:
            case ESM::MagicEffect::DemoralizeHumanoid:
            case ESM::MagicEffect::DemoralizeCreature:
            case ESM::MagicEffect::RallyHumanoid:
            case ESM::MagicEffect::RallyCreature:
            case ESM::MagicEffect::Charm:
            case ESM::MagicEffect::DetectAnimal:
            case ESM::MagicEffect::DetectEnchantment:
            case ESM::MagicEffect::DetectKey:
            case ESM::MagicEffect::Telekinesis:
            case ESM::MagicEffect::Mark:
            case ESM::MagicEffect::Recall:
            case ESM::MagicEffect::Jump:
            case ESM::MagicEffect::WaterBreathing:
            case ESM::MagicEffect::SwiftSwim:
            case ESM::MagicEffect::WaterWalking:
            case ESM::MagicEffect::SlowFall:
            case ESM::MagicEffect::Light:
            case ESM::MagicEffect::Lock:
            case ESM::MagicEffect::Open:
            case ESM::MagicEffect::TurnUndead:
            case ESM::MagicEffect::WeaknessToCommonDisease:
            case ESM::MagicEffect::WeaknessToBlightDisease:
            case ESM::MagicEffect::WeaknessToCorprusDisease:
            case ESM::MagicEffect::CureCommonDisease:
            case ESM::MagicEffect::CureBlightDisease:
            case ESM::MagicEffect::CureCorprusDisease:
            case ESM::MagicEffect::ResistBlightDisease:
            case ESM::MagicEffect::ResistCommonDisease:
            case ESM::MagicEffect::ResistCorprusDisease:
            case ESM::MagicEffect::Invisibility:
            case ESM::MagicEffect::Chameleon:
            case ESM::MagicEffect::NightEye:
            case ESM::MagicEffect::Vampirism:
            case ESM::MagicEffect::StuntedMagicka:
            case ESM::MagicEffect::ExtraSpell:
            case ESM::MagicEffect::RemoveCurse:
            case ESM::MagicEffect::CommandCreature:
            case ESM::MagicEffect::CommandHumanoid:
                return 0.f;

            case ESM::MagicEffect::Blind:
            {
                if (enemy.isEmpty())
                    return 0.f;

                const CreatureStats& stats = enemy.getClass().getCreatureStats(enemy);

                // Enemy can't attack
                if (stats.isParalyzed() || stats.getKnockedDown())
                    return 0.f;

                // Enemy doesn't attack
                if (stats.getDrawState() != MWMechanics::DrawState::Weapon)
                    return 0.f;

                break;
            }

            case ESM::MagicEffect::Sound:
            {
                if (enemy.isEmpty())
                    return 0.f;

                const CreatureStats& stats = enemy.getClass().getCreatureStats(enemy);

                // Enemy can't cast spells
                if (stats.getMagicEffects().getOrDefault(ESM::MagicEffect::Silence).getMagnitude() > 0)
                    return 0.f;

                if (stats.isParalyzed() || stats.getKnockedDown())
                    return 0.f;

                // Enemy doesn't cast spells
                if (stats.getDrawState() != MWMechanics::DrawState::Spell)
                    return 0.f;

                break;
            }

            case ESM::MagicEffect::Silence:
            {
                if (enemy.isEmpty())
                    return 0.f;

                const CreatureStats& stats = enemy.getClass().getCreatureStats(enemy);

                // Enemy can't cast spells
                if (stats.isParalyzed() || stats.getKnockedDown())
                    return 0.f;

                // Enemy doesn't cast spells
                if (stats.getDrawState() != MWMechanics::DrawState::Spell)
                    return 0.f;
                break;
            }

            case ESM::MagicEffect::RestoreAttribute:
                return 0.f; // TODO: implement based on attribute damage
            case ESM::MagicEffect::RestoreSkill:
                return 0.f; // TODO: implement based on skill damage

            case ESM::MagicEffect::ResistFire:
            case ESM::MagicEffect::ResistFrost:
            case ESM::MagicEffect::ResistMagicka:
            case ESM::MagicEffect::ResistNormalWeapons:
            case ESM::MagicEffect::ResistParalysis:
            case ESM::MagicEffect::ResistPoison:
            case ESM::MagicEffect::ResistShock:
            case ESM::MagicEffect::SpellAbsorption:
            case ESM::MagicEffect::Reflect:
                return 0.f; // probably useless since we don't know in advance what the enemy will cast

            // don't cast these for now as they would make the NPC cast the same effect over and over again, especially
            // when they have potions
            case ESM::MagicEffect::FortifyAttribute:
            case ESM::MagicEffect::FortifyHealth:
            case ESM::MagicEffect::FortifyMagicka:
            case ESM::MagicEffect::FortifyFatigue:
            case ESM::MagicEffect::FortifySkill:
            case ESM::MagicEffect::FortifyMaximumMagicka:
            case ESM::MagicEffect::FortifyAttack:
                return 0.f;

            case ESM::MagicEffect::Burden:
            {
                if (enemy.isEmpty())
                    return 0.f;

                // Ignore enemy without inventory
                if (!enemy.getClass().hasInventoryStore(enemy))
                    return 0.f;

                // burden makes sense only to overburden an enemy
                float burden = enemy.getClass().getEncumbrance(enemy) - enemy.getClass().getCapacity(enemy);
                if (burden > 0)
                    return 0.f;

                if ((effect.mMagnMin + effect.mMagnMax) / 2.f > -burden)
                    rating *= 3;
                else
                    return 0.f;

                break;
            }

            case ESM::MagicEffect::Feather:
            {
                // Ignore actors without inventory
                if (!actor.getClass().hasInventoryStore(actor))
                    return 0.f;

                // feather makes sense only for overburden actors
                float burden = actor.getClass().getEncumbrance(actor) - actor.getClass().getCapacity(actor);
                if (burden <= 0)
                    return 0.f;

                if ((effect.mMagnMin + effect.mMagnMax) / 2.f >= burden)
                    rating *= 3;
                else
                    return 0.f;

                break;
            }

            case ESM::MagicEffect::Levitate:
                return 0.f; // AI isn't designed to take advantage of this, and could be perceived as unfair anyway
            case ESM::MagicEffect::BoundBoots:
            case ESM::MagicEffect::BoundHelm:
                if (actor.getClass().isNpc())
                {
                    // Beast races can't wear helmets or boots
                    const ESM::RefId& raceid = actor.get<ESM::NPC>()->mBase->mRace;
                    const ESM::Race* race = MWBase::Environment::get().getESMStore()->get<ESM::Race>().find(raceid);
                    if (race->mData.mFlags & ESM::Race::Beast)
                        return 0.f;
                }
                else
                    return 0.f;

                break;
            case ESM::MagicEffect::BoundShield:
                if (!actor.getClass().hasInventoryStore(actor))
                    return 0.f;
                else if (!actor.getClass().isNpc())
                {
                    // If the actor is an NPC they can benefit from the armor rating, otherwise check if we've got a
                    // one-handed weapon to use with the shield
                    const auto& store = actor.getClass().getInventoryStore(actor);
                    auto oneHanded = std::find_if(store.cbegin(MWWorld::ContainerStore::Type_Weapon), store.cend(),
                        [](const MWWorld::ConstPtr& weapon) {
                            if (weapon.getClass().getItemHealth(weapon) <= 0.f)
                                return false;
                            short type = weapon.get<ESM::Weapon>()->mBase->mData.mType;
                            return !(MWMechanics::getWeaponType(type)->mFlags & ESM::WeaponType::TwoHanded);
                        });
                    if (oneHanded == store.cend())
                        return 0.f;
                }
                break;
            // Creatures can not wear armor
            case ESM::MagicEffect::BoundCuirass:
            case ESM::MagicEffect::BoundGloves:
                if (!actor.getClass().isNpc())
                    return 0.f;
                break;

            case ESM::MagicEffect::AbsorbMagicka:
                if (!enemy.isEmpty() && enemy.getClass().getCreatureStats(enemy).getMagicka().getCurrent() <= 0.f)
                {
                    rating = 0.5f;
                    rating *= getRestoreMagickaPriority(actor);
                }
                break;
            case ESM::MagicEffect::RestoreHealth:
            case ESM::MagicEffect::RestoreMagicka:
            case ESM::MagicEffect::RestoreFatigue:
                if (effect.mRange == ESM::RT_Self)
                {
                    const MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
                    const DynamicStat<float>& current
                        = stats.getDynamic(effect.mEffectID - ESM::MagicEffect::RestoreHealth);
                    // NB: this currently assumes the hardcoded magic effect flags are used
                    const float magnitude = (effect.mMagnMin + effect.mMagnMax) / 2.f;
                    const float toHeal = magnitude * std::max(1, effect.mDuration);
                    const float damage = std::max(current.getModified() - current.getCurrent(), 0.f);
                    float priority = 0.f;
                    if (effect.mEffectID == ESM::MagicEffect::RestoreHealth)
                        priority = 4.f;
                    else if (effect.mEffectID == ESM::MagicEffect::RestoreMagicka)
                        priority = getRestoreMagickaPriority(actor);
                    else if (effect.mEffectID == ESM::MagicEffect::RestoreFatigue)
                        priority = 2.f;
                    float overheal = 0.f;
                    float heal = toHeal;
                    if (damage < toHeal && current.getCurrent() > current.getModified() * 0.5)
                    {
                        overheal = toHeal - damage;
                        heal = damage;
                    }

                    priority = (priority - 1.f) / 2.f * std::pow((damage / current.getModified() + 0.6f), priority * 2)
                        + priority * (heal - 2.f * overheal) / current.getModified() - 0.5f;
                    rating = priority;
                }
                break;

            case ESM::MagicEffect::Dispel:
            {
                int numPositive = 0;
                int numNegative = 0;
                int diff = 0;

                if (effect.mRange == ESM::RT_Self)
                {
                    numPositive = numEffectsToDispel(actor, -1, false);
                    numNegative = numEffectsToDispel(actor);

                    diff = numNegative - numPositive;
                }
                else
                {
                    if (enemy.isEmpty())
                        return 0.f;

                    numPositive = numEffectsToDispel(enemy, -1, false);
                    numNegative = numEffectsToDispel(enemy);

                    diff = numPositive - numNegative;

                    // if rating < 0 here, the spell will be considered as negative later
                    rating *= -1;
                }

                if (diff <= 0)
                    return 0.f;

                rating *= (diff) / 5.f;

                break;
            }

            // Prefer Cure effects over Dispel, because Dispel also removes positive effects
            case ESM::MagicEffect::CureParalyzation:
                return 1001.f * numEffectsToDispel(actor, ESM::MagicEffect::Paralyze);

            case ESM::MagicEffect::CurePoison:
                return 1001.f * numEffectsToDispel(actor, ESM::MagicEffect::Poison);
            case ESM::MagicEffect::DisintegrateArmor:
            {
                if (enemy.isEmpty())
                    return 0.f;

                // Ignore enemy without inventory
                if (!enemy.getClass().hasInventoryStore(enemy))
                    return 0.f;

                MWWorld::InventoryStore& inv = enemy.getClass().getInventoryStore(enemy);

                // According to UESP
                static const int armorSlots[] = {
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

                bool enemyHasArmor = false;

                // Ignore enemy without armor
                for (unsigned int i = 0; i < sizeof(armorSlots) / sizeof(int); ++i)
                {
                    MWWorld::ContainerStoreIterator item = inv.getSlot(armorSlots[i]);

                    if (item != inv.end() && (item.getType() == MWWorld::ContainerStore::Type_Armor))
                    {
                        enemyHasArmor = true;
                        break;
                    }
                }

                if (!enemyHasArmor)
                    return 0.f;

                break;
            }

            case ESM::MagicEffect::DisintegrateWeapon:
            {
                if (enemy.isEmpty())
                    return 0.f;

                // Ignore enemy without inventory
                if (!enemy.getClass().hasInventoryStore(enemy))
                    return 0.f;

                MWWorld::InventoryStore& inv = enemy.getClass().getInventoryStore(enemy);
                MWWorld::ContainerStoreIterator item = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);

                // Ignore enemy without weapons
                if (item == inv.end() || (item.getType() != MWWorld::ContainerStore::Type_Weapon))
                    return 0.f;

                break;
            }

            case ESM::MagicEffect::AbsorbAttribute:
            case ESM::MagicEffect::DamageAttribute:
            case ESM::MagicEffect::DrainAttribute:
                if (!enemy.isEmpty()
                    && enemy.getClass()
                            .getCreatureStats(enemy)
                            .getAttribute(ESM::Attribute::indexToRefId(effect.mAttribute))
                            .getModified()
                        <= 0)
                    return 0.f;
                {
                    if (effect.mAttribute >= 0 && effect.mAttribute < ESM::Attribute::Length)
                    {
                        const float attributePriorities[ESM::Attribute::Length] = {
                            1.0f, // Strength
                            0.5f, // Intelligence
                            0.6f, // Willpower
                            0.7f, // Agility
                            0.5f, // Speed
                            0.8f, // Endurance
                            0.7f, // Personality
                            0.3f // Luck
                        };
                        rating *= attributePriorities[effect.mAttribute];
                    }
                }
                break;

            case ESM::MagicEffect::AbsorbSkill:
            case ESM::MagicEffect::DamageSkill:
            case ESM::MagicEffect::DrainSkill:
                if (enemy.isEmpty() || !enemy.getClass().isNpc())
                    return 0.f;
                if (enemy.getClass().getSkill(enemy, ESM::Skill::indexToRefId(effect.mSkill)) <= 0)
                    return 0.f;
                break;

            default:
                break;
        }

        // Allow only one summoned creature at time
        if (isSummoningEffect(effect.mEffectID))
        {
            MWMechanics::CreatureStats& creatureStats = actor.getClass().getCreatureStats(actor);

            if (!creatureStats.getSummonedCreatureMap().empty())
                return 0.f;
            // But rate summons higher than other effects
            rating = 3.f;
        }
        if (effect.mEffectID >= ESM::MagicEffect::BoundDagger && effect.mEffectID <= ESM::MagicEffect::BoundGloves)
        {
            // Prefer casting bound items over other spells
            rating = 2.f;
            // While rateSpell prevents actors from recasting the same spell, it doesn't prevent them from casting
            // different spells with the same effect. Multiple instances of the same bound item don't stack so if the
            // effect is already active, rate it as useless. Likewise, if the actor already has a bound weapon, don't
            // summon another of a different kind unless what we have is a bow and the actor is out of ammo.
            // FIXME: This code assumes the summoned item is of the usual type (i.e. a mod hasn't changed Bound Bow to
            // summon an Axe instead)
            if (effect.mEffectID <= ESM::MagicEffect::BoundLongbow)
            {
                for (int e = ESM::MagicEffect::BoundDagger; e <= ESM::MagicEffect::BoundLongbow; ++e)
                    if (actor.getClass().getCreatureStats(actor).getMagicEffects().getOrDefault(e).getMagnitude() > 0.f
                        && (e != ESM::MagicEffect::BoundLongbow || effect.mEffectID == e
                            || rateAmmo(actor, enemy, getWeaponType(ESM::Weapon::MarksmanBow)->mAmmoType) <= 0.f))
                        return 0.f;
                ESM::RefId skill = ESM::Skill::ShortBlade;
                if (effect.mEffectID == ESM::MagicEffect::BoundLongsword)
                    skill = ESM::Skill::LongBlade;
                else if (effect.mEffectID == ESM::MagicEffect::BoundMace)
                    skill = ESM::Skill::BluntWeapon;
                else if (effect.mEffectID == ESM::MagicEffect::BoundBattleAxe)
                    skill = ESM::Skill::Axe;
                else if (effect.mEffectID == ESM::MagicEffect::BoundSpear)
                    skill = ESM::Skill::Spear;
                else if (effect.mEffectID == ESM::MagicEffect::BoundLongbow)
                {
                    // AI should not summon the bow if there is no suitable ammo.
                    if (rateAmmo(actor, enemy, getWeaponType(ESM::Weapon::MarksmanBow)->mAmmoType) <= 0.f)
                        return 0.f;
                    skill = ESM::Skill::Marksman;
                }
                // Prefer summoning items we know how to use
                rating *= (50.f + actor.getClass().getSkill(actor, skill)) / 100.f;
            }
            else if (actor.getClass()
                         .getCreatureStats(actor)
                         .getMagicEffects()
                         .getOrDefault(effect.mEffectID)
                         .getMagnitude()
                > 0.f)
                return 0.f;
        }

        // Underwater casting not possible
        if (effect.mRange == ESM::RT_Target)
        {
            if (MWBase::Environment::get().getWorld()->isUnderwater(MWWorld::ConstPtr(actor), 0.75f))
                return 0.f;

            if (enemy.isEmpty())
                return 0.f;

            if (MWBase::Environment::get().getWorld()->isUnderwater(MWWorld::ConstPtr(enemy), 0.75f))
                return 0.f;
        }

        const ESM::MagicEffect* magicEffect
            = MWBase::Environment::get().getESMStore()->get<ESM::MagicEffect>().find(effect.mEffectID);
        if (magicEffect->mData.mFlags & ESM::MagicEffect::Harmful)
        {
            rating *= -1.f;

            if (enemy.isEmpty())
                return 0.f;

            // Check resistance for harmful effects
            CreatureStats& stats = enemy.getClass().getCreatureStats(enemy);

            float resistance = MWMechanics::getEffectResistanceAttribute(effect.mEffectID, &stats.getMagicEffects());

            rating *= (1.f - std::min(resistance, 100.f) / 100.f);
        }

        // for harmful no-magnitude effects (e.g. silence) check if enemy is already has them
        // for non-harmful no-magnitude effects (e.g. bound items) check if actor is already has them
        if (magicEffect->mData.mFlags & ESM::MagicEffect::NoMagnitude)
        {
            if (magicEffect->mData.mFlags & ESM::MagicEffect::Harmful)
            {
                CreatureStats& stats = enemy.getClass().getCreatureStats(enemy);

                if (stats.getMagicEffects().getOrDefault(effect.mEffectID).getMagnitude() > 0)
                    return 0.f;
            }
            else
            {
                CreatureStats& stats = actor.getClass().getCreatureStats(actor);

                if (stats.getMagicEffects().getOrDefault(effect.mEffectID).getMagnitude() > 0)
                    return 0.f;
            }
        }

        rating *= calcEffectCost(effect, magicEffect);

        // Currently treating all "on target" or "on touch" effects to target the enemy actor.
        // Combat AI is egoistic, so doesn't consider applying positive effects to friendly actors.
        if (effect.mRange != ESM::RT_Self)
            rating *= -1.f;

        return rating;
    }

    float rateEffects(
        const ESM::EffectList& list, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy, bool useSpellMult)
    {
        // NOTE: enemy may be empty

        float rating = 0.f;

        const MWWorld::Store<ESM::GameSetting>& gmst
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();
        static const float fAIMagicSpellMult = gmst.find("fAIMagicSpellMult")->mValue.getFloat();
        static const float fAIRangeMagicSpellMult = gmst.find("fAIRangeMagicSpellMult")->mValue.getFloat();

        for (const ESM::IndexedENAMstruct& effect : list.mList)
        {
            float effectRating = rateEffect(effect.mData, actor, enemy);
            if (useSpellMult)
            {
                if (effect.mData.mRange == ESM::RT_Target)
                    effectRating *= fAIRangeMagicSpellMult;
                else
                    effectRating *= fAIMagicSpellMult;
            }
            rating += effectRating;
        }
        return rating;
    }

    float vanillaRateSpell(const ESM::Spell* spell, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy)
    {
        const MWWorld::Store<ESM::GameSetting>& gmst
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();

        static const float fAIMagicSpellMult = gmst.find("fAIMagicSpellMult")->mValue.getFloat();
        static const float fAIRangeMagicSpellMult = gmst.find("fAIRangeMagicSpellMult")->mValue.getFloat();

        float mult = fAIMagicSpellMult;

        for (std::vector<ESM::IndexedENAMstruct>::const_iterator effectIt = spell->mEffects.mList.begin();
             effectIt != spell->mEffects.mList.end(); ++effectIt)
        {
            if (effectIt->mData.mRange == ESM::RT_Target)
            {
                if (!MWBase::Environment::get().getWorld()->isSwimming(enemy))
                    mult = fAIRangeMagicSpellMult;
                else
                    mult = 0.0f;
                break;
            }
        }

        return MWMechanics::getSpellSuccessChance(spell, actor) * mult;
    }
}
