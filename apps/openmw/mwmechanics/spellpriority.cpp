#include "spellpriority.hpp"
#include "weaponpriority.hpp"

#include <components/esm/loadench.hpp>
#include <components/esm/loadmgef.hpp>
#include <components/esm/loadspel.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/cellstore.hpp"

#include "creaturestats.hpp"
#include "spellcasting.hpp"
#include "weapontype.hpp"
#include "combat.hpp"

namespace
{
    int numEffectsToDispel (const MWWorld::Ptr& actor, int effectFilter=-1, bool negative = true)
    {
        int toCure=0;
        const MWMechanics::ActiveSpells& activeSpells = actor.getClass().getCreatureStats(actor).getActiveSpells();
        for (MWMechanics::ActiveSpells::TIterator it = activeSpells.begin(); it != activeSpells.end(); ++it)
        {
            // if the effect filter is not specified, take in account only spells effects. Leave potions, enchanted items etc.
            if (effectFilter == -1)
            {
                const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(it->first);
                if (!spell || spell->mData.mType != ESM::Spell::ST_Spell)
                    continue;
            }

            const MWMechanics::ActiveSpells::ActiveSpellParams& params = it->second;
            for (std::vector<MWMechanics::ActiveSpells::ActiveEffect>::const_iterator effectIt = params.mEffects.begin();
                effectIt != params.mEffects.end(); ++effectIt)
            {
                int effectId = effectIt->mEffectId;
                if (effectFilter != -1 && effectId != effectFilter)
                    continue;
                const ESM::MagicEffect* magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effectId);

                if (effectIt->mDuration <= 3) // Don't attempt to dispel if effect runs out shortly anyway
                    continue;

                if (negative && magicEffect->mData.mFlags & ESM::MagicEffect::Harmful)
                    ++toCure;

                if (!negative && !(magicEffect->mData.mFlags & ESM::MagicEffect::Harmful))
                    ++toCure;
            }
        }
        return toCure;
    }

    float getSpellDuration (const MWWorld::Ptr& actor, const std::string& spellId)
    {
        float duration = 0;
        const MWMechanics::ActiveSpells& activeSpells = actor.getClass().getCreatureStats(actor).getActiveSpells();
        for (MWMechanics::ActiveSpells::TIterator it = activeSpells.begin(); it != activeSpells.end(); ++it)
        {
            if (it->first != spellId)
                continue;

            const MWMechanics::ActiveSpells::ActiveSpellParams& params = it->second;
            for (std::vector<MWMechanics::ActiveSpells::ActiveEffect>::const_iterator effectIt = params.mEffects.begin();
                effectIt != params.mEffects.end(); ++effectIt)
            {
                if (effectIt->mDuration > duration)
                    duration = effectIt->mDuration;
            }
        }
        return duration;
    }
}

namespace MWMechanics
{
    int getRangeTypes (const ESM::EffectList& effects)
    {
        int types = 0;
        for (std::vector<ESM::ENAMstruct>::const_iterator it = effects.mList.begin(); it != effects.mList.end(); ++it)
        {
            if (it->mRange == ESM::RT_Self)
                types |= RangeTypes::Self;
            else if (it->mRange == ESM::RT_Touch)
                types |= RangeTypes::Touch;
            else if (it->mRange == ESM::RT_Target)
                types |= RangeTypes::Target;
        }
        return types;
    }

    float ratePotion (const MWWorld::Ptr &item, const MWWorld::Ptr& actor)
    {
        if (item.getTypeName() != typeid(ESM::Potion).name())
            return 0.f;

        const ESM::Potion* potion = item.get<ESM::Potion>()->mBase;
        return rateEffects(potion->mEffects, actor, MWWorld::Ptr());
    }

    float rateSpell(const ESM::Spell *spell, const MWWorld::Ptr &actor, const MWWorld::Ptr& enemy)
    {
        const CreatureStats& stats = actor.getClass().getCreatureStats(actor);

        float successChance = MWMechanics::getSpellSuccessChance(spell, actor);
        if (successChance == 0.f)
            return 0.f;

        if (spell->mData.mType != ESM::Spell::ST_Spell)
            return 0.f;

        // Don't make use of racial bonus spells, like MW. Can be made optional later
        if (actor.getClass().isNpc())
        {
            std::string raceid = actor.get<ESM::NPC>()->mBase->mRace;
            const ESM::Race* race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(raceid);
            if (race->mPowers.exists(spell->mId))
                return 0.f;
        }

        // Spells don't stack, so early out if the spell is still active on the target
        int types = getRangeTypes(spell->mEffects);
        if ((types & Self) && stats.getActiveSpells().isSpellActive(spell->mId))
            return 0.f;
        if ( ((types & Touch) || (types & Target)) && enemy.getClass().getCreatureStats(enemy).getActiveSpells().isSpellActive(spell->mId))
            return 0.f;

        return rateEffects(spell->mEffects, actor, enemy) * (successChance / 100.f);
    }

    float rateMagicItem(const MWWorld::Ptr &ptr, const MWWorld::Ptr &actor, const MWWorld::Ptr& enemy)
    {
        if (ptr.getClass().getEnchantment(ptr).empty())
            return 0.f;

        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(ptr.getClass().getEnchantment(ptr));

        // Spells don't stack, so early out if the spell is still active on the target
        int types = getRangeTypes(enchantment->mEffects);
        if ((types & Self) && actor.getClass().getCreatureStats(actor).getActiveSpells().isSpellActive(ptr.getCellRef().getRefId()))
            return 0.f;

        if (types & (Touch|Target) && getSpellDuration(enemy, ptr.getCellRef().getRefId()) > 3)
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

            int castCost = getEffectiveEnchantmentCastCost(static_cast<float>(enchantment->mData.mCost), actor);

            if (ptr.getCellRef().getEnchantmentCharge() != -1
               && ptr.getCellRef().getEnchantmentCharge() < castCost)
                return 0.f;

            float rating = rateEffects(enchantment->mEffects, actor, enemy);

            rating *= 1.25f; // prefer rechargable magic items over spells
            return rating;
        }

        return 0.f;
    }

    float rateEffect(const ESM::ENAMstruct &effect, const MWWorld::Ptr &actor, const MWWorld::Ptr &enemy)
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
                if (stats.getDrawState() != MWMechanics::DrawState_Weapon)
                    return 0.f;

                break;
            }

        case ESM::MagicEffect::Sound:
            {
                if (enemy.isEmpty())
                    return 0.f;

                const CreatureStats& stats = enemy.getClass().getCreatureStats(enemy);

                // Enemy can't cast spells
                if (stats.getMagicEffects().get(ESM::MagicEffect::Silence).getMagnitude() > 0)
                    return 0.f;

                if (stats.isParalyzed() || stats.getKnockedDown())
                    return 0.f;

                // Enemy doesn't cast spells
                if (stats.getDrawState() != MWMechanics::DrawState_Spell)
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
                if (stats.getDrawState() != MWMechanics::DrawState_Spell)
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

        // don't cast these for now as they would make the NPC cast the same effect over and over again, especially when they have potions
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

                if ((effect.mMagnMin + effect.mMagnMax)/2.f > -burden)
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

                if ((effect.mMagnMin + effect.mMagnMax)/2.f >= burden)
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
                std::string raceid = actor.get<ESM::NPC>()->mBase->mRace;
                const ESM::Race* race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(raceid);
                if (race->mData.mFlags & ESM::Race::Beast)
                    return 0.f;
            }
            else
                return 0.f;

            break;
        // Creatures can not wear armor
        case ESM::MagicEffect::BoundCuirass:
        case ESM::MagicEffect::BoundGloves:
            if (!actor.getClass().isNpc())
                return 0.f;
            break;

        case ESM::MagicEffect::BoundLongbow:
            // AI should not summon the bow if there is no suitable ammo.
            if (rateAmmo(actor, enemy, getWeaponType(ESM::Weapon::MarksmanBow)->mAmmoType) <= 0.f)
                return 0.f;
            break;

        case ESM::MagicEffect::RestoreHealth:
        case ESM::MagicEffect::RestoreMagicka:
        case ESM::MagicEffect::RestoreFatigue:
            if (effect.mRange == ESM::RT_Self)
            {
                int priority = 1;
                if (effect.mEffectID == ESM::MagicEffect::RestoreHealth)
                    priority = 10;
                const MWMechanics::CreatureStats& stats = actor.getClass().getCreatureStats(actor);
                const DynamicStat<float>& current = stats.getDynamic(effect.mEffectID - ESM::MagicEffect::RestoreHealth);
                const float magnitude = (effect.mMagnMin + effect.mMagnMax)/2.f;
                const float toHeal = magnitude * effect.mDuration;
                // Effect doesn't heal more than we need, *or* we are below 1/2 health
                if (current.getModified() - current.getCurrent() > toHeal
                        || current.getCurrent() < current.getModified()*0.5)
                {
                    return 10000.f * priority
                            - (toHeal - (current.getModified()-current.getCurrent())); // prefer the most fitting potion
                }
                else
                    return -10000.f * priority; // Save for later
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
                    MWWorld::InventoryStore::Slot_Boots
                };

                bool enemyHasArmor = false;

                // Ignore enemy without armor
                for (unsigned int i=0; i<sizeof(armorSlots)/sizeof(int); ++i)
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

        case ESM::MagicEffect::DamageAttribute:
        case ESM::MagicEffect::DrainAttribute:
            if (!enemy.isEmpty() && enemy.getClass().getCreatureStats(enemy).getAttribute(effect.mAttribute).getModified() <= 0)
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

        case ESM::MagicEffect::DamageSkill:
        case ESM::MagicEffect::DrainSkill:
            if (enemy.isEmpty() || !enemy.getClass().isNpc())
                return 0.f;
            if (enemy.getClass().getSkill(enemy, effect.mSkill) <= 0)
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

        const ESM::MagicEffect* magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effect.mEffectID);
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

                if (stats.getMagicEffects().get(effect.mEffectID).getMagnitude() > 0)
                    return 0.f;
            }
            else
            {
                CreatureStats& stats = actor.getClass().getCreatureStats(actor);

                if (stats.getMagicEffects().get(effect.mEffectID).getMagnitude() > 0)
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

    float rateEffects(const ESM::EffectList &list, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy)
    {
        // NOTE: enemy may be empty

        float rating = 0.f;
        float ratingMult = 1.f; // NB: this multiplier is applied to the effect rating, not the final rating

        const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();
        static const float fAIMagicSpellMult = gmst.find("fAIMagicSpellMult")->mValue.getFloat();
        static const float fAIRangeMagicSpellMult = gmst.find("fAIRangeMagicSpellMult")->mValue.getFloat();

        for (std::vector<ESM::ENAMstruct>::const_iterator it = list.mList.begin(); it != list.mList.end(); ++it)
        {
            ratingMult = (it->mRange == ESM::RT_Target) ? fAIRangeMagicSpellMult : fAIMagicSpellMult;

            rating += rateEffect(*it, actor, enemy) * ratingMult;
        }
        return rating;
    }

    float vanillaRateSpell(const ESM::Spell* spell, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy)
    {
        const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        static const float fAIMagicSpellMult = gmst.find("fAIMagicSpellMult")->mValue.getFloat();
        static const float fAIRangeMagicSpellMult = gmst.find("fAIRangeMagicSpellMult")->mValue.getFloat();

        float mult = fAIMagicSpellMult;

        for (std::vector<ESM::ENAMstruct>::const_iterator effectIt =
             spell->mEffects.mList.begin(); effectIt != spell->mEffects.mList.end(); ++effectIt)
        {
            if (effectIt->mRange == ESM::RT_Target)
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
