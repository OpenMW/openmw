#include "aicombataction.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/actionequip.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/spellcasting.hpp"

#include <components/esm/loadench.hpp>
#include <components/esm/loadmgef.hpp>

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

void suggestCombatRange(int rangeTypes, float& rangeAttack, float& rangeFollow)
{
    if (rangeTypes & Touch)
    {
        rangeAttack = 100.f;
        rangeFollow = 300.f;
    }
    else if (rangeTypes & Target)
    {
        rangeAttack = 1000.f;
        rangeFollow = 0.f;
    }
    else
    {
        // For Self spells, distance doesn't matter, so back away slightly to avoid enemy hits
        rangeAttack = 600.f;
        rangeFollow = 0.f;
    }
}

int numEffectsToCure (const MWWorld::Ptr& actor, int effectFilter=-1)
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
            if (magicEffect->mData.mFlags & ESM::MagicEffect::Harmful
                    && effectIt->mDuration > 3 // Don't attempt to cure if effect runs out shortly anyway
                    )
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

    float rateWeapon (const MWWorld::Ptr &item, const MWWorld::Ptr& actor, const MWWorld::Ptr& target, int type,
                      float arrowRating, float boltRating)
    {
        if (item.getTypeName() != typeid(ESM::Weapon).name())
            return 0.f;

        const ESM::Weapon* weapon = item.get<ESM::Weapon>()->mBase;

        if (type != -1 && weapon->mData.mType != type)
            return 0.f;

        float rating=0.f;

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
                rating += rateEffects(enchantment->mEffects, actor, target);
        }

        int skill = item.getClass().getEquipmentSkill(item);
        if (skill != -1)
            rating *= actor.getClass().getSkill(actor, skill) / 100.f;

        return rating;
    }

    float rateSpell(const ESM::Spell *spell, const MWWorld::Ptr &actor, const MWWorld::Ptr& target)
    {
        const CreatureStats& stats = actor.getClass().getCreatureStats(actor);

        if (MWMechanics::getSpellSuccessChance(spell, actor) == 0)
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
        if ( ((types & Touch) || (types & Target)) && target.getClass().getCreatureStats(target).getActiveSpells().isSpellActive(spell->mId))
            return 0.f;

        return rateEffects(spell->mEffects, actor, target);
    }

    float rateMagicItem(const MWWorld::Ptr &ptr, const MWWorld::Ptr &actor, const MWWorld::Ptr& target)
    {
        if (ptr.getClass().getEnchantment(ptr).empty())
            return 0.f;

        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(ptr.getClass().getEnchantment(ptr));

        if (enchantment->mData.mType == ESM::Enchantment::CastOnce)
        {
            return rateEffects(enchantment->mEffects, actor, target);
        }
        else
        {
            //if (!ptr.getClass().canBeEquipped(ptr, actor))
            return 0.f;
        }
    }

    float rateEffect(const ESM::ENAMstruct &effect, const MWWorld::Ptr &actor, const MWWorld::Ptr &target)
    {
        // NOTE: target may be empty

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
        case ESM::MagicEffect::Invisibility:
            return 0.f;
        case ESM::MagicEffect::Feather:
            if (actor.getClass().getEncumbrance(actor) - actor.getClass().getCapacity(actor) >= 0)
                return 100.f;
            else
                return 0.f;
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

        // Prefer Cure effects over Dispel, because Dispel also removes positive effects
        case ESM::MagicEffect::Dispel:
            return 1000.f * numEffectsToCure(actor);
        case ESM::MagicEffect::CureParalyzation:
            return 1001.f * numEffectsToCure(actor, ESM::MagicEffect::Paralyze);
        case ESM::MagicEffect::CurePoison:
            return 1001.f * numEffectsToCure(actor, ESM::MagicEffect::Poison);

        case ESM::MagicEffect::DisintegrateArmor: // TODO: check if actor is wearing armor
        case ESM::MagicEffect::DisintegrateWeapon: // TODO: check if actor is wearing weapon
            break;

        case ESM::MagicEffect::DamageAttribute:
        case ESM::MagicEffect::DrainAttribute:
            if (!target.isEmpty() && target.getClass().getCreatureStats(target).getAttribute(effect.mAttribute).getModified() <= 0)
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
            if (target.isEmpty() || !target.getClass().isNpc())
                return 0.f;
            if (target.getClass().getNpcStats(target).getSkill(effect.mSkill).getModified() <= 0)
                return 0.f;
            break;

        default:
            break;
        }

        // TODO: for non-cumulative effects (e.g. paralyze), check if the target is already suffering from them

        // TODO: could take into account target's resistance/weakness against the effect

        const ESM::MagicEffect* magicEffect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effect.mEffectID);

        rating *= magicEffect->mData.mBaseCost;

        if (!(magicEffect->mData.mFlags & ESM::MagicEffect::NoMagnitude))
            rating *= (effect.mMagnMin + effect.mMagnMax)/2.f;
        if (!(magicEffect->mData.mFlags & ESM::MagicEffect::NoDuration))
            rating *= effect.mDuration;

        if (magicEffect->mData.mFlags & ESM::MagicEffect::Harmful)
            rating *= -1.f;

        // Currently treating all "on target" or "on touch" effects to target the enemy actor.
        // Combat AI is egoistic, so doesn't consider applying positive effects to friendly actors.
        if (effect.mRange != ESM::RT_Self)
            rating *= -1.f;
        return rating;
    }

    float rateEffects(const ESM::EffectList &list, const MWWorld::Ptr& actor, const MWWorld::Ptr& target)
    {
        // NOTE: target may be empty
        float rating = 0.f;
        for (std::vector<ESM::ENAMstruct>::const_iterator it = list.mList.begin(); it != list.mList.end(); ++it)
        {
            rating += rateEffect(*it, actor, target);
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
    }

    void ActionSpell::getCombatRange(float& rangeAttack, float& rangeFollow)
    {
        const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(mSpellId);
        int types = getRangeTypes(spell->mEffects);
        suggestCombatRange(types, rangeAttack, rangeFollow);
    }

    void ActionEnchantedItem::prepare(const MWWorld::Ptr &actor)
    {
        actor.getClass().getCreatureStats(actor).getSpells().setSelectedSpell(std::string());
        actor.getClass().getInventoryStore(actor).setSelectedEnchantItem(mItem);
        actor.getClass().getCreatureStats(actor).setDrawState(DrawState_Spell);
    }

    void ActionEnchantedItem::getCombatRange(float& rangeAttack, float& rangeFollow)
    {
        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(mItem->getClass().getEnchantment(*mItem));
        int types = getRangeTypes(enchantment->mEffects);
        suggestCombatRange(types, rangeAttack, rangeFollow);
    }

    void ActionPotion::getCombatRange(float& rangeAttack, float& rangeFollow)
    {
        // distance doesn't matter, so back away slightly to avoid enemy hits
        rangeAttack = 600.f;
        rangeFollow = 0.f;
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

    void ActionWeapon::getCombatRange(float& rangeAttack, float& rangeFollow)
    {
        // Already done in AiCombat itself
    }

    boost::shared_ptr<Action> prepareNextAction(const MWWorld::Ptr &actor, const MWWorld::Ptr &target)
    {
        Spells& spells = actor.getClass().getCreatureStats(actor).getSpells();

        float bestActionRating = 0.f;
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
                }
            }

            for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
            {
                float rating = rateMagicItem(*it, actor, target);
                if (rating > bestActionRating)
                {
                    bestActionRating = rating;
                    bestAction.reset(new ActionEnchantedItem(it));
                }
            }

            float bestArrowRating = 0;
            MWWorld::Ptr bestArrow;
            for (MWWorld::ContainerStoreIterator it = store.begin(); it != store.end(); ++it)
            {
                float rating = rateWeapon(*it, actor, target, ESM::Weapon::Arrow);
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
                float rating = rateWeapon(*it, actor, target, ESM::Weapon::Bolt);
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

                float rating = rateWeapon(*it, actor, target, -1, bestArrowRating, bestBoltRating);
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
                }
            }
        }

        for (Spells::TIterator it = spells.begin(); it != spells.end(); ++it)
        {
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(it->first);

            float rating = rateSpell(spell, actor, target);
            if (rating > bestActionRating)
            {
                bestActionRating = rating;
                bestAction.reset(new ActionSpell(spell->mId));
            }
        }

        if (bestAction.get())
            bestAction->prepare(actor);

        return bestAction;
    }

}
