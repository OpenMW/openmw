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

namespace
{

// RangeTypes using bitflags to allow multiple range types, as can be the case with spells having multiple effects.
enum RangeTypes
{
    Self = 0x1,
    Touch = 0x10,
    Target = 0x100
};

int getRangeTypes (const ESM::EffectList& effects)
{
    int types = 0;
    for (std::vector<ESM::ENAMstruct>::const_iterator it = effects.mList.begin(); it != effects.mList.end(); ++it)
    {
        if (it->mRange == ESM::RT_Self)
            types |= Self;
        else if (it->mRange == ESM::RT_Touch)
            types |= Touch;
        else if (it->mRange == ESM::RT_Target)
            types |= Target;
    }
    return types;
}

float suggestCombatRange(int rangeTypes)
{
    if (rangeTypes & Touch)
    {
        static const float fCombatDistance = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fCombatDistance")->getFloat();
        return fCombatDistance;
    }
    else if (rangeTypes & Target)
    {
        return 1000.f;
    }
    else
    {
        // For Self spells, distance doesn't matter, so back away slightly to avoid enemy hits
        return 600.f;
    }
}

int numEffectsToDispel (const MWWorld::Ptr& actor, int effectFilter=-1, bool negative = true)
{
    int toCure=0;
    const MWMechanics::ActiveSpells& activeSpells = actor.getClass().getCreatureStats(actor).getActiveSpells();
    for (MWMechanics::ActiveSpells::TIterator it = activeSpells.begin(); it != activeSpells.end(); ++it)
    {
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

}

namespace MWMechanics
{

    float ratePotion (const MWWorld::Ptr &item, const MWWorld::Ptr& actor)
    {
        if (item.getTypeName() != typeid(ESM::Potion).name())
            return 0.f;

        const ESM::Potion* potion = item.get<ESM::Potion>()->mBase;
        return rateEffects(potion->mEffects, actor, MWWorld::Ptr());
    }

    float rateWeapon (const MWWorld::Ptr &item, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy, int type,
                      float arrowRating, float boltRating)
    {
        if (item.getTypeName() != typeid(ESM::Weapon).name())
            return 0.f;

        const ESM::Weapon* weapon = item.get<ESM::Weapon>()->mBase;

        if (type != -1 && weapon->mData.mType != type)
            return 0.f;

        float rating=0.f;
        float bonus=0.f;

        if (weapon->mData.mType >= ESM::Weapon::MarksmanBow && weapon->mData.mType <= ESM::Weapon::MarksmanThrown)
            bonus+=1.5f;

        if (weapon->mData.mType >= ESM::Weapon::MarksmanBow)
        {
            rating = (weapon->mData.mChop[0] + weapon->mData.mChop[1]) / 2.f;
        }
        else
        {
            for (int i=0; i<2; ++i)
            {
                rating += weapon->mData.mSlash[i];
                rating += weapon->mData.mThrust[i];
                rating += weapon->mData.mChop[i];
            }
            rating /= 6.f;
        }

        if (item.getClass().hasItemHealth(item))
        {
            if (item.getClass().getItemHealth(item) == 0)
                return 0.f;
            rating *= item.getClass().getItemHealth(item) / float(item.getClass().getItemMaxHealth(item));
        }

        if (weapon->mData.mType == ESM::Weapon::MarksmanBow)
        {
            if (arrowRating <= 0.f)
                rating = 0.f;
            else
                rating += arrowRating;
        }
        else if (weapon->mData.mType == ESM::Weapon::MarksmanCrossbow)
        {
            if (boltRating <= 0.f)
                rating = 0.f;
            else
                rating += boltRating;
        }

        if (!weapon->mEnchant.empty())
        {
            const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(weapon->mEnchant);
            if (enchantment->mData.mType == ESM::Enchantment::WhenStrikes
                    && (item.getCellRef().getEnchantmentCharge() == -1
                        || item.getCellRef().getEnchantmentCharge() >= enchantment->mData.mCost))
                rating += rateEffects(enchantment->mEffects, actor, enemy);
        }

        int skill = item.getClass().getEquipmentSkill(item);
        if (skill != -1)
            rating *= actor.getClass().getSkill(actor, skill) / 100.f;

        return rating + bonus;
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

        if (spell->mData.mCost > stats.getMagicka().getCurrent())
            return 0.f;

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

        if (enchantment->mData.mType == ESM::Enchantment::CastOnce)
        {
            return rateEffects(enchantment->mEffects, actor, enemy);
        }
        else
        {
            //if (!ptr.getClass().canBeEquipped(ptr, actor))
            return 0.f;
        }
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

        case ESM::MagicEffect::Sound:
            {
                if (enemy.isEmpty())
                    return 0.f;

                // there is no need to cast sound if enemy is not able to cast spells
                CreatureStats& stats = enemy.getClass().getCreatureStats(enemy);

                if (stats.getMagicEffects().get(ESM::MagicEffect::Silence).getMagnitude() > 0)
                    return 0.f;

                if (stats.getMagicEffects().get(ESM::MagicEffect::Paralyze).getMagnitude() > 0)
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
            // Intended fall-through
        // Creatures can not wear armor
        case ESM::MagicEffect::BoundCuirass:
        case ESM::MagicEffect::BoundGloves:
            if (!actor.getClass().isNpc())
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
                const DynamicStat<float>& current = actor.getClass().getCreatureStats(actor).
                        getDynamic(effect.mEffectID - ESM::MagicEffect::RestoreHealth);
                float toHeal = (effect.mMagnMin + effect.mMagnMax)/2.f * effect.mDuration;
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
            if (enemy.getClass().getNpcStats(enemy).getSkill(effect.mSkill).getModified() <= 0)
                return 0.f;
            break;

        default:
            break;
        }

        const ESM::MagicEffect* magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effect.mEffectID);

        rating *= magicEffect->mData.mBaseCost;

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
        else
        {
            rating *= (effect.mMagnMin + effect.mMagnMax)/2.f;
        }

        if (!(magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration))
            rating *= effect.mDuration;

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
        for (std::vector<ESM::ENAMstruct>::const_iterator it = list.mList.begin(); it != list.mList.end(); ++it)
        {
            rating += rateEffect(*it, actor, enemy);
        }
        return rating;
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

        isRanged = (types & Target);
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
        return suggestCombatRange(types);
    }

    float ActionPotion::getCombatRange(bool& isRanged) const
    {
        // distance doesn't matter, so back away slightly to avoid enemy hits
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

        static const float fCombatDistance = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fCombatDistance")->getFloat();

        if (mWeapon.isEmpty())
        {
            static float fHandToHandReach =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fHandToHandReach")->getFloat();
            return fHandToHandReach * fCombatDistance;
        }

        const ESM::Weapon* weapon = mWeapon.get<ESM::Weapon>()->mBase;

        if (weapon->mData.mType >= ESM::Weapon::MarksmanBow)
        {
            isRanged = true;
            return 1000.f;
        }
        else
            return weapon->mData.mReach * fCombatDistance;
    }

    const ESM::Weapon* ActionWeapon::getWeapon() const
    {
        if (mWeapon.isEmpty())
            return NULL;
        return mWeapon.get<ESM::Weapon>()->mBase;
    }

    boost::shared_ptr<Action> prepareNextAction(const MWWorld::Ptr &actor, const MWWorld::Ptr &enemy)
    {
        Spells& spells = actor.getClass().getCreatureStats(actor).getSpells();

        float bestActionRating = 0.f;
        float antiFleeRating = 0.f;
        // Default to hand-to-hand combat
        boost::shared_ptr<Action> bestAction (new ActionWeapon(MWWorld::Ptr()));
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

            float bestArrowRating = 0;
            MWWorld::Ptr bestArrow;
            for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
            {
                float rating = rateWeapon(*it, actor, enemy, ESM::Weapon::Arrow);
                if (rating > bestArrowRating)
                {
                    bestArrowRating = rating;
                    bestArrow = *it;
                }
            }

            float bestBoltRating = 0;
            MWWorld::Ptr bestBolt;
            for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
            {
                float rating = rateWeapon(*it, actor, enemy, ESM::Weapon::Bolt);
                if (rating > bestBoltRating)
                {
                    bestBoltRating = rating;
                    bestBolt = *it;
                }
            }

            for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
            {
                std::vector<int> equipmentSlots = it->getClass().getEquipmentSlots(*it).first;
                if (std::find(equipmentSlots.begin(), equipmentSlots.end(), (int)MWWorld::InventoryStore::Slot_CarriedRight)
                        == equipmentSlots.end())
                    continue;

                float rating = rateWeapon(*it, actor, enemy, -1, bestArrowRating, bestBoltRating);
                if (rating > bestActionRating)
                {
                    const ESM::Weapon* weapon = it->get<ESM::Weapon>()->mBase;

                    MWWorld::Ptr ammo;
                    if (weapon->mData.mType == ESM::Weapon::MarksmanBow)
                        ammo = bestArrow;
                    else if (weapon->mData.mType == ESM::Weapon::MarksmanCrossbow)
                        ammo = bestBolt;

                    bestActionRating = rating;
                    bestAction.reset(new ActionWeapon(*it, ammo));
                    antiFleeRating = vanillaRateWeaponAndAmmo(*it, ammo, actor, enemy);
                }
            }
        }

        for (Spells::TIterator it = spells.begin(); it != spells.end(); ++it)
        {
            const ESM::Spell* spell = it->first;

            float rating = rateSpell(spell, actor, enemy);
            if (rating > bestActionRating)
            {
                bestActionRating = rating;
                bestAction.reset(new ActionSpell(spell->mId));
                antiFleeRating = vanillaRateSpell(spell, actor, enemy);
            }
        }

        if (makeFleeDecision(actor, enemy, antiFleeRating))
            bestAction.reset(new ActionFlee());

        if (bestAction.get())
            bestAction->prepare(actor);

        return bestAction;
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
            static const float fHandToHandReach = gmst.find("fHandToHandReach")->getFloat();
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
                    if (effectIt->mArea == ESM::RT_Target)
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
                        if (effectIt->mArea == ESM::RT_Target)
                        {
                            const ESM::MagicEffect* effect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effectIt->mEffectID);
                            dist = effect->mData.mSpeed;
                            break;
                        }
                    }
                }
            }

            static const float fTargetSpellMaxSpeed = gmst.find("fTargetSpellMaxSpeed")->getFloat();
            dist *= std::max(1000.0f, fTargetSpellMaxSpeed);
        }
        else if (!activeWeapon.isEmpty())
        {
            const ESM::Weapon* esmWeap = activeWeapon.get<ESM::Weapon>()->mBase;
            if (esmWeap->mData.mType >= ESM::Weapon::MarksmanBow)
            {
                static const float fTargetSpellMaxSpeed = gmst.find("fProjectileMaxSpeed")->getFloat();
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

        static const float fCombatDistance = gmst.find("fCombatDistance")->getFloat();
        static const float fCombatDistanceWerewolfMod = gmst.find("fCombatDistanceWerewolfMod")->getFloat();

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

        const CreatureStats& enemyStats = enemy.getClass().getCreatureStats(enemy);
        if (enemyStats.getMagicEffects().get(ESM::MagicEffect::Invisibility).getMagnitude() > 0
                || enemyStats.getMagicEffects().get(ESM::MagicEffect::Chameleon).getMagnitude() > 0)
        {
            if (!MWBase::Environment::get().getMechanicsManager()->awarenessCheck(enemy, actor))
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

    float vanillaRateSpell(const ESM::Spell* spell, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy)
    {
        const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        static const float fAIMagicSpellMult = gmst.find("fAIMagicSpellMult")->getFloat();
        static const float fAIRangeMagicSpellMult = gmst.find("fAIRangeMagicSpellMult")->getFloat();

        float mult = fAIMagicSpellMult;

        for (std::vector<ESM::ENAMstruct>::const_iterator effectIt =
             spell->mEffects.mList.begin(); effectIt != spell->mEffects.mList.end(); ++effectIt)
        {
            if (effectIt->mArea == ESM::RT_Target)
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

    float vanillaRateWeaponAndAmmo(const MWWorld::Ptr& weapon, const MWWorld::Ptr& ammo, const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy)
    {
        const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        static const float fAIMeleeWeaponMult = gmst.find("fAIMeleeWeaponMult")->getFloat();
        static const float fAIMeleeArmorMult = gmst.find("fAIMeleeArmorMult")->getFloat();
        static const float fAIRangeMeleeWeaponMult = gmst.find("fAIRangeMeleeWeaponMult")->getFloat();

        if (weapon.isEmpty())
            return 0.f;

        float skillMult = actor.getClass().getSkill(actor, weapon.getClass().getEquipmentSkill(weapon)) * 0.01f;
        float chopMult = fAIMeleeWeaponMult;
        float bonusDamage = 0.f;

        const ESM::Weapon* esmWeap = weapon.get<ESM::Weapon>()->mBase;

        if (esmWeap->mData.mType >= ESM::Weapon::MarksmanBow)
        {
            if (!ammo.isEmpty() && !MWBase::Environment::get().getWorld()->isSwimming(enemy))
            {
                bonusDamage = ammo.get<ESM::Weapon>()->mBase->mData.mChop[1];
                chopMult = fAIRangeMeleeWeaponMult;
            }
            else
                chopMult = 0.f;
        }

        float chopRating = (esmWeap->mData.mChop[1] + bonusDamage) * skillMult * chopMult;
        float slashRating = esmWeap->mData.mSlash[1] * skillMult * fAIMeleeWeaponMult;
        float thrustRating = esmWeap->mData.mThrust[1] * skillMult * fAIMeleeWeaponMult;

        return actor.getClass().getArmorRating(actor) * fAIMeleeArmorMult
                    + std::max(std::max(chopRating, slashRating), thrustRating);
    }

    float vanillaRateFlee(const MWWorld::Ptr& actor, const MWWorld::Ptr& enemy)
    {
        const CreatureStats& stats = actor.getClass().getCreatureStats(actor);
        const MWWorld::Store<ESM::GameSetting>& gmst = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        int flee = stats.getAiSetting(CreatureStats::AI_Flee).getModified();
        if (flee >= 100)
            return flee;

        static const float fAIFleeHealthMult = gmst.find("fAIFleeHealthMult")->getFloat();
        static const float fAIFleeFleeMult = gmst.find("fAIFleeFleeMult")->getFloat();

        float healthPercentage = (stats.getHealth().getModified() == 0.0f)
                                    ? 1.0f : stats.getHealth().getCurrent() / stats.getHealth().getModified();
        float rating = (1.0f - healthPercentage) * fAIFleeHealthMult + flee * fAIFleeFleeMult;

        static const int iWereWolfLevelToAttack = gmst.find("iWereWolfLevelToAttack")->getInt();

        if (enemy.getClass().isNpc() && enemy.getClass().getNpcStats(enemy).isWerewolf() && stats.getLevel() < iWereWolfLevelToAttack)
        {
            static const int iWereWolfFleeMod = gmst.find("iWereWolfFleeMod")->getInt();
            rating = iWereWolfFleeMod;
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
